#include "dotnet.hpp"
#include <Windows.h>
#include <TlHelp32.h>
#include <_plugins.h>
#include <filesystem>
#include <string>
#include <memory>
#include <algorithm>
#include <fstream>

// Non volatile (instance can be kept throughout lifetime)
static ICLRMetaHost    * meta_host       = nullptr;
static ICLRDebugging   * clr_debugging   = nullptr;
static ICorRuntimeHost * runtime_host    = nullptr;
static IUnknown        * host_app_domain = nullptr;

// Volatile (instance is recreated and is only placed here for global access)
// inline ICLRRuntimeInfo * runtime_info  = nullptr;
static std::unique_ptr<xsfd::debug_lib_provider> dlp_instance = nullptr;
static std::unique_ptr<xsfd::debug_data_target>  ddt_instance = nullptr;
static ICorDebugProcess  * cor_debug_process  = nullptr;
static ICorDebugProcess5 * cor_debug_process5 = nullptr;
void                     * prepare_method = nullptr;

static HMODULE h_mscoree = nullptr;
static HRESULT(*_CLRCreateInstance)(REFCLSID, REFIID, LPVOID *) = nullptr;

static auto get_dotnet_path(std::filesystem::path & path_out, int (&ver_out)[3]) -> bool
{
	// Find runtime to be loaded
	#if XSFDEU_ARCH32
		std::filesystem::path dn_ver_dir = "C:/Program Files (x86)/dotnet/shared/Microsoft.NETCore.App";
	#elif XSFDEU_ARCH64
		std::filesystem::path dn_ver_dir = "C:/Program Files/dotnet/shared/Microsoft.NETCore.App";
	#else
		#error Target architechture was not defined. Please make sure the project is properly generated and is built for an appropriate target.
	#endif

	if (!std::filesystem::exists(dn_ver_dir))
		return false;

	bool has_path = false;
	for (const auto & ver_path : std::filesystem::directory_iterator(dn_ver_dir))
	{
		if (!ver_path.is_directory())
			continue;

		const std::wstring & path_str = ver_path.path().native();	

		const auto ver_index = path_str.find_last_of(L"/\\");
		if (ver_index == std::wstring::npos)
			continue;

		std::wstring ver_str = path_str.substr(ver_index + 1); 
		
		XSFD_DEBUG_LOG("!Found installed dotNet version: %s\n", xsfd::wc2u8(ver_str).c_str());

		int nvers[3] = {};
		if (swscanf_s(ver_str.c_str(), L"%d.%d.%d", &nvers[0], &nvers[1], &nvers[2]) != 3)
			continue;

		for (int i = 0; i < 3; ++i)
		{
			if (nvers[i] > ver_out[i])
			{
				has_path = true;
				path_out = ver_path;

				for (int vi = 0; vi < 3; ++vi)
					ver_out[vi] = nvers[vi];

				break;
			}
		}
	}

	if (!has_path)
		return false;

	return true;
}

/*
* RuntimeHelpers.PrepareMethod() - v4.0.30319
* 89 8D ? ? ? ? 33 DB 85 C9 75 22 68 ? ? ? ? 53 6A 04 - \x89\x8D\x00\x00\x00\x00\x33\xDB\x85\xC9\x75\x22\x68\x00\x00\x00\x00\x53\x6A\x04 xx????xxxxxxx????xxx @ clr.dll
* 68 98 00 00 00                                  push    98h ; '˜'
* B8 94 3F 6E 10                                  mov     eax, offset sub_106E3F94
* E8 96 CE B6 FF                                  call    sub_10010C95
* 8B F2                                           mov     esi, edx
* 89 B5 E0 FF FF FF                               mov     [ebp-20h], esi
* B8 F0 3D 4A 10                                  mov     eax, offset RuntimeHelpers_PrepareMethod
* 89 85 D0 FF FF FF                               mov     [ebp-30h], eax
* 89 8D CC FF FF FF                               mov     [ebp-34h], ecx                            <<<
* 33 DB                                           xor     ebx, ebx
* 85 C9                                           test    ecx, ecx
* 75 22                                           jnz     short loc_104A3E40
*/
static auto dotnet_get_preparemethod_adr(const MODULEENTRY32 & clr) -> void *
{
	if (!clr.modBaseAddr || !clr.modBaseSize)
	{
		xsfd::log("!ERROR: Invalid clr module entry.\n");
		return nullptr;
	}

	auto clr_buff = std::make_unique<std::uint8_t[]>(clr.modBaseSize);
	if (!clr_buff)
	{
		xsfd::log("!ERROR: Failed to allocate for clr buffer.\n");
		return nullptr;
	}

	if (!DbgMemRead((duint)clr.modBaseAddr, clr_buff.get(), clr.modBaseSize))
	{
		xsfd::log("!ERROR: Failed to read clr module from remote process.\n");
		return nullptr;
	}

	auto rva = xsfd::pattern_scan_rva<"89 8D ?? ?? ?? ?? 33 DB 85 C9 75 22 68 ?? ?? ?? ?? 53 6A 04">(clr_buff.get(), clr.modBaseSize);
	if (rva == xsfd::INVALID_PATTERN_RVA)
	{
		xsfd::log("!ERROR: Pattern for RuntimeHelpers.PrepareMethod was not found.\n");
		return nullptr;
	}

	void * result = *(void **)((clr_buff.get() + rva) - 0xA);
	XSFD_DEBUG_LOG("!Found RuntimeHelpers.PrepareMethod RVA at 0x%x and VA at 0x%p\n", rva, result);
	return result;
}

auto dotnet::initialize() -> bool
{	
	if (!h_mscoree)
	{
		h_mscoree = LoadLibraryW(L"mscoree.dll");
		if (!h_mscoree)
		{
			xsfd::log("!ERROR: Failed to load mscoree module.\n");
			return false;
		}
		XSFD_DEBUG_LOG("!Loaded mscoree module @ 0x%p\n", h_mscoree);
	}

	if (!_CLRCreateInstance)
	{
		_CLRCreateInstance = reinterpret_cast<decltype(_CLRCreateInstance)>(GetProcAddress(h_mscoree, "CLRCreateInstance"));
		if (!_CLRCreateInstance)
		{
			xsfd::log("!ERROR: Failed to load CLRCreateInstance function pointer.\n");
			return false;
		}
		XSFD_DEBUG_LOG("!Loaded CLRCreateInstance @ 0x%p\n", _CLRCreateInstance);
	}

	HANDLE phnd = DbgGetProcessHandle();
	DWORD pid   = DbgGetProcessId();
	if (!phnd || !pid)
	{
		xsfd::log("!ERROR: No process handle.\n");
		return false;
	}
	XSFD_DEBUG_LOG("!Process handle: 0x%p (ID: %d)\n", phnd, pid);

	// ------------------------------------------------------------------------------------------------------------------------------------------------------ 
	// Meta host
	// ------------------------------------------------------------------------------------------------------------------------------------------------------ 
	if (!meta_host)
	{
		if (_CLRCreateInstance(CLSID_CLRMetaHost, IID_ICLRMetaHost, reinterpret_cast<LPVOID *>(&meta_host)) != S_OK)
		{
			xsfd::log("!ERROR: Failed to create meta host instance.\n");
			return false;
		}
		XSFD_DEBUG_LOG("!Created meta host instance @ 0x%p\n", meta_host);
	}

	// ------------------------------------------------------------------------------------------------------------------------------------------------------ 
	// Enumerate loaded runtimes
	// ------------------------------------------------------------------------------------------------------------------------------------------------------ 
	IEnumUnknown * enum_runtime = nullptr;
	if (meta_host->EnumerateLoadedRuntimes(phnd, &enum_runtime) != S_OK)
	{
		xsfd::log("!ERROR: Failed to create an enumerator for loaded runtimes.\n");
		return false;
	}

	XSFD_DEFER {
		XSFD_DEBUG_LOG("!Releasing runtime enumerator.\n");
		enum_runtime->Release();
	};
	

	XSFD_DEBUG_LOG("!Created runtime enumerator @ 0x%p\n", enum_runtime);

	ICLRRuntimeInfo * rinfo = nullptr;
	ULONG count = 0;
	if (enum_runtime->Next(1, reinterpret_cast<IUnknown **>(&rinfo), &count) != S_OK || count != 1)
	{
		xsfd::log("!ERROR: No runtime information could be found.\n");
		return false;
	}

	#if XSFDEU_DEBUG
	{
		wchar_t bver[24] = { L'\0' };
		DWORD bsize = sizeof(bver) / sizeof(bver[0]);
		rinfo->GetVersionString(bver, &bsize);
		xsfd::log("!Obtained runtime information %s @ 0x%p\n", xsfd::wc2u8(bver).c_str(), rinfo);
	}
	#endif

	dlp_instance = std::make_unique<xsfd::debug_lib_provider>(rinfo);
	if (!dlp_instance)
	{
		rinfo->Release();
		xsfd::log("!ERROR: Failed to create debug library provider");
		return false;
	}
	XSFD_DEBUG_LOG("!Created debug library provider @ 0x%p\n", dlp_instance.get());

	// ------------------------------------------------------------------------------------------------------------------------------------------------------ 
	// CLR Debugging setup
	// ------------------------------------------------------------------------------------------------------------------------------------------------------ 
	if (!clr_debugging)
	{
		if (_CLRCreateInstance(CLSID_CLRDebugging, IID_ICLRDebugging, reinterpret_cast<LPVOID *>(&clr_debugging)) != S_OK)
		{
			xsfd::log("!ERROR: Could not create clr debugging instance.\n");
			return false;
		}
		XSFD_DEBUG_LOG("!Created CLR debugging instance @ 0x%p\n", clr_debugging);
	}

	HANDLE mod_snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
	if (mod_snap == INVALID_HANDLE_VALUE)
	{
		xsfd::log("!Could not create module snapshot of target process. GetLastError: %d\n", GetLastError());
		return false;
	}

	XSFD_DEFER {
		XSFD_DEBUG_LOG("!Releasing module snapshot of target process.\n");
		CloseHandle(mod_snap);
	};

	XSFD_DEBUG_LOG("!Module snapshot 0x%p\n", mod_snap);

	MODULEENTRY32 me32 = { .dwSize = sizeof(me32) };
	if (!Module32First(mod_snap, &me32))
	{
		xsfd::log("!ERROR: No module entry on module snapshot.\n");
		return false;
	}

	CLR_DEBUGGING_VERSION cdv = {
		.wStructVersion = 0,
		.wMajor         = 4,
		.wMinor         = 0,
		.wBuild         = 65535,
		.wRevision      = 30319,
	};
	CLR_DEBUGGING_PROCESS_FLAGS cdpf {};

	ddt_instance = std::make_unique<xsfd::debug_data_target>(phnd);
	if (!ddt_instance)
	{
		xsfd::log("!ERROR: Failed to create debug data target instance.\n");
		return false;
	}
	XSFD_DEBUG_LOG("!Created debug data target @ 0x%p\n", ddt_instance.get());

	bool clr_found = false;
	MODULEENTRY32 me_clr {};

	XSFD_DEBUG_LOG("!Checking debuggable modules:\n");
	do
	{
		static constexpr IID IID_ICorDebugProcess = { 0x3d6f5f64, 0x7538, 0x11d3, 0x8d, 0x5b, 0x00, 0x10, 0x4b, 0x35, 0xe7, 0xef };
		XSFD_DEBUG_LOG("\n    %s", me32.szModule);
		if (!clr_found && strcmp(me32.szModule, "clr.dll") == 0)
		{
			XSFD_DEBUG_LOG("\n        -- CLR MODULE FOUND @ 0x%p", me32.modBaseAddr);
			me_clr = me32;
			clr_found = true;
		}

		if (!cor_debug_process && clr_debugging->OpenVirtualProcess(ULONG64(me32.hModule), ddt_instance.get(), dlp_instance.get(), &cdv, IID_ICorDebugProcess, reinterpret_cast<IUnknown **>(&cor_debug_process), &cdv, &cdpf) == S_OK)
		{
			XSFD_DEBUG_LOG("\n        -- DEBUG TARGET FOUND @ 0x%p", me32.modBaseAddr);
		}

		if (clr_found && cor_debug_process)
			break;

	} while (Module32Next(mod_snap, &me32));

	XSFD_DEBUG_LOG("\n");

	if (!cor_debug_process)
	{
		xsfd::log("!ERROR: Failed to open debug handle to virtual process.\n");
		return false;
	}

	if (!clr_found)
	{
		xsfd::log("!ERROR: Failed to locate clr.dll from the target process.\n");
		return false;
	}

	XSFD_DEBUG_LOG("!Open virtual process for debugging @ 0x%p\n", cor_debug_process);

	static constexpr IID IID_ICorDebugProcess5 = { 0x21e9d9c0, 0xfcb8, 0x11df, 0x8c, 0xff, 0x08, 0x00, 0x20, 0x0c ,0x9a, 0x66 };
	if (cor_debug_process->QueryInterface(IID_ICorDebugProcess5, reinterpret_cast<void **>(&cor_debug_process5)) != S_OK)
	{
		xsfd::log("!ERROR: Failed to create COR debug process 5 instance.\n");
		return false;
	}
	XSFD_DEBUG_LOG("!Created COR debug process 5 instance @ 0x%p\n", cor_debug_process5);

	#if 0
	prepare_method = dotnet_get_preparemethod_adr(me_clr);
	if (!prepare_method)
		return false;
	#endif

	return true;
}

auto dotnet::uninitialize() -> bool
{
	if (cor_debug_process5)
	{
		XSFD_DEBUG_LOG("!Releasing COR Debug Process instance 5.\n");
		cor_debug_process5->Release();
		cor_debug_process5 = nullptr;
	}

	if (cor_debug_process)
	{
		XSFD_DEBUG_LOG("!Releasing COR Debug Process instance.\n");
		cor_debug_process->Release();
		cor_debug_process = nullptr;
	}

	if (ddt_instance)
	{
		XSFD_DEBUG_LOG("!Releasing debug data target instance.\n");
		ddt_instance = nullptr;
	}

	if (dlp_instance)
	{
		XSFD_DEBUG_LOG("!Releasing data library provider instance.\n");
		dlp_instance = nullptr;
	}

	return true;
}

auto dotnet::destroy() -> bool
{
	if (host_running())
		host_end();

	if (!uninitialize())
		return false;

	if (clr_debugging)
	{
		XSFD_DEBUG_LOG("!Releasing CLR Debugging instance.\n");
		clr_debugging->Release();
		clr_debugging = nullptr;
	}

	if (meta_host)
	{
		XSFD_DEBUG_LOG("!Releasing Meta host instance.\n");
		meta_host->Release();
		meta_host = nullptr;
	}

	return true;
}

struct dn_ver_info_t
{
	ICLRRuntimeInfo * info;
	int v[3];
	ICorRuntimeHost * host;
};

static auto dotnet_enumerate_and_run_installed_hosts(ICLRMetaHost * mhost, dn_ver_info_t & out_runtimehost) -> bool
{
	out_runtimehost = {
		.info = nullptr,
		.host = nullptr
	};

	IEnumUnknown * installed_runtimes = nullptr;
	if (mhost->EnumerateInstalledRuntimes(&installed_runtimes) != S_OK)
	{
		xsfd::log("!ERROR: Could not enumerate installed runtimes.\n");
		return false;
	}

	XSFD_DEFER {
		XSFD_DEBUG_LOG("!Releasing installed_runtimes enumerator...\n");
		installed_runtimes->Release();
	};

	XSFD_DEBUG_LOG("!Installed hosts enumerator @ 0x%p\n", installed_runtimes);

	std::vector<dn_ver_info_t> runtime_infos;

	ICLRRuntimeInfo * current_info = nullptr;
	ULONG count = 0;
	while (installed_runtimes->Next(1, (IUnknown **)&current_info, &count) == S_OK && count == 1)
	{
		// !IMPORTANT: Null current_info if it was successfully added to prevent it from being automatically released.
		XSFD_DEFER {
			if (current_info)
				current_info->Release();
		};

		dn_ver_info_t current_dn = {};
		current_dn.info = current_info;

		wchar_t v_str[32] = {};
		DWORD v_sz = 32;
		if (current_info->GetVersionString(v_str, &v_sz) != S_OK)
		{
			xsfd::log("!Version parsing failed for installed runtime 0x%p\n", current_info);
			continue;
		}

		XSFD_DEBUG_LOG("!Installed runtime 0x%p is running %s\n", current_info, xsfd::wc2u8(v_str).c_str());

		if (swscanf_s(v_str, L"v%d.%d.%d", &current_dn.v[0], &current_dn.v[1], &current_dn.v[2]) != 3)
		{
			xsfd::log("!Numeric version parsing failed.\n");
			continue;
		}

		if (BOOL is_loadable = FALSE; current_info->IsLoadable(&is_loadable) != S_OK || !is_loadable)
		{
			xsfd::log("!Installed runtime 0x%p is not loadable.", current_info);
			continue;
		}

		runtime_infos.emplace_back(current_dn);
		current_info = nullptr;
	}

	if (runtime_infos.empty())
	{
		xsfd::log("!ERROR: No suitable runtime info found.\n");
		return false;
	}

	std::sort(runtime_infos.begin(), runtime_infos.end(), [](const dn_ver_info_t & lhs, const dn_ver_info_t & rhs) -> bool {
		return lhs.v[0] > rhs.v[0] || lhs.v[1] > rhs.v[1]; //|| lhs.v[2] > rhs.v[2];
	});

	#ifdef XSFDEU_DEBUG
	{
		xsfd::log("!Version sorted to:\n");
		for (const auto & v : runtime_infos)
			xsfd::log("!    %d.%d.%d\n", v.v[0], v.v[1], v.v[2]);
	}
	#endif

	// Look for a loadable version
	for (const auto & v : runtime_infos)
	{
		if (BOOL is_loadable = FALSE; v.info->IsLoadable(&is_loadable) != S_OK || !is_loadable)
			continue;

		// NOTE: Null rtime_host if its successful to prevent it from being released
		ICorRuntimeHost * rtime_host = nullptr;
		if (v.info->GetInterface(CLSID_CorRuntimeHost, IID_ICorRuntimeHost, (LPVOID *)&rtime_host) != S_OK)
			continue;

		XSFD_DEFER {
			if (rtime_host)
				rtime_host->Release();
		};

		if (rtime_host->Start() != S_OK)
			continue;

		out_runtimehost.info = v.info;
		out_runtimehost.host = rtime_host;
		for (int i = 0; i < 3; ++i)
			out_runtimehost.v[i] = v.v[i];

		// Prevent release
		rtime_host = nullptr;

		break;
	}

	// Release runtime information
	for (const auto & v : runtime_infos)
		if (v.info != out_runtimehost.info) // Except if its going out
			v.info->Release();

	return out_runtimehost.host; // Returns true or false depending if there's a resulting host.
}

auto dotnet::host_running() -> bool
{
	return runtime_host;
}

auto dotnet::host_start() -> bool
{
	const char * _einfo = "This isn't supposed to happen as the dotnet component should be initialized before this is called.";
	if (!h_mscoree)
	{
		xsfd::log("!ERROR: mscoree is not loaded. %s\n", _einfo);
		return false;
	}

	if (!meta_host)
	{
		xsfd::log("!ERROR: Meta host interface is not available. %s\n", _einfo);
		return false;
	}

	ICorRuntimeHost * _runtime_host = nullptr; // DEV: Upon success null this pointer to prevent it from being released
	if (dn_ver_info_t result; dotnet_enumerate_and_run_installed_hosts(meta_host, result))
	{
		XSFD_DEBUG_LOG("!Hosting dotnet version %d.%d.%d @ 0x%p\n", result.v[0], result.v[1], result.v[2], result.host);
		result.info->Release();
		_runtime_host = result.host;
	}
	else
	{
		xsfd::log("!ERROR: No available runtime.\n");
		return false;
	}

	XSFD_DEFER {
		if (_runtime_host)
		{
			XSFD_DEBUG_LOG("!Stopping and Releasing temporary _runtime_host...\n");
			_runtime_host->Stop();
			_runtime_host->Release();
		}
	};

	IUnknown * default_domain = nullptr;
	if (_runtime_host->GetDefaultDomain(&default_domain) != S_OK || !default_domain)
	{
		xsfd::log("!ERROR: Could not obtain default domain.\n");
		return false;
	}
	XSFD_DEFER {
		if (default_domain)
		{
			XSFD_DEBUG_LOG("!Releasing default_domain...\n");
			default_domain->Release();
		}
	};
	XSFD_DEBUG_LOG("!Default domain @ 0x%p\n", default_domain);
	
	constexpr GUID app_domain_guid = { 0x05f696dc, 0x2b29, 0x3663, { 0xad, 0x8b, 0xc4, 0x38, 0x9c, 0xf2, 0xa7, 0x13 } };
	IUnknown * app_domain = nullptr;
	if (default_domain->QueryInterface(app_domain_guid, (void **)&app_domain) != S_OK)
	{
		xsfd::log("!ERROR: Failed to query for app domain.\n");
		return false;
	}
	XSFD_DEFER {
		if (app_domain)
		{
			XSFD_DEBUG_LOG("!Releasing app_domain...\n");
			app_domain->Release();
		}
	};
	XSFD_DEBUG_LOG("!App domain @ 0x%p\n", app_domain);

	runtime_host = _runtime_host;
	_runtime_host = nullptr;

	host_app_domain = app_domain;
	app_domain = nullptr;

	return true;
}

auto dotnet::host_end() -> bool
{
	if (!runtime_host)
	{
		xsfd::log("ERROR: No existing host to end in the first place.\n");
		return false;
	}

	if (host_app_domain)
	{
		XSFD_DEBUG_LOG("!Releasing host_app_domain...\n");
		host_app_domain->Release();
		host_app_domain = nullptr;
	}

	if (runtime_host->Stop() != S_OK)
	{
		xsfd::log("ERROR: Failed to stop runtime host.\n");
		return false;
	}

	XSFD_DEBUG_LOG("!Releasing runtime_host...\n");
	runtime_host->Release();
	runtime_host = nullptr;
	return true;
}

auto dotnet::host_load_library(const char * lib_name) -> std::optional<wrap_Assembly>
{
	if (!host_app_domain || !lib_name)
		return std::nullopt;

	auto _path = std::filesystem::path("plugins") / lib_name;
	#if 0
	if (std::filesystem::exists(_path))
	{
		xsfd::log("!ERROR: Library file not found.\n");
		return std::nullopt;
	}
	#endif

	std::ifstream f;
	if (f.open(_path, std::ios::binary); !f.is_open())
		return std::nullopt;

	XSFD_DEFER {
		f.close();
	};

	auto _sz = 0;
	{
		auto a = f.tellg();
		f.seekg(0, std::ios::end);
		auto b = f.tellg();
		f.clear();
		f.seekg(0);
		_sz = b - a;
	}

	XSFD_DEBUG_LOG("!Loading %d bytes of binary...", _sz);

	auto _bin_tmp = std::make_unique<std::uint8_t[]>(_sz);
	f.read((char *)_bin_tmp.get(), _sz);

	if (*(std::uint16_t *)_bin_tmp.get() != *(std::uint16_t *)"MZ")
	{
		xsfd::log("!ERROR: Invalid PE magic!\n");
		return std::nullopt;
	}

	SAFEARRAYBOUND sab = {
		.cElements = (ULONG)_sz,
		.lLbound   = 0
	};

	SAFEARRAY * sa_bin = SafeArrayCreate(VT_UI1, 1, &sab);
	if (!sa_bin)
	{
		xsfd::log("!ERROR: SafeArrayCreate Failed.\n");
		return std::nullopt;
	}

	std::uint8_t * sa_access = nullptr;
	if (SafeArrayAccessData(sa_bin, (void **)&sa_access) != S_OK)
	{
		xsfd::log("!ERROR: SafeArrayAccessData Failed.\n");
		return std::nullopt;
	}
	XSFD_DEBUG_LOG("!SafeArrayAccess to library binary @ 0x%p\n", sa_access);

	std::memcpy(sa_access, _bin_tmp.get(), _sz);

	if (SafeArrayUnaccessData(sa_bin) != S_OK)
	{
		xsfd::log("!ERROR: SafeArrayUnaccessData Failed.\n");
		return std::nullopt;
	}

	void * out_assembly = nullptr;
	if ((*(HRESULT(__stdcall***)(void *, void *))host_app_domain)[0x0b4](sa_bin, &out_assembly) != S_OK) // Load_3 - loads the instance
	{
		xsfd::log("!ERROR: Failed to load assembly (Load_3).\n");
		return std::nullopt;
	}

	XSFD_DEBUG_LOG("!Assembly library loaded @ 0x%p\n", out_assembly);

	return std::nullopt;
}

auto dotnet::dump() -> std::optional<std::vector<domain_info_t>>
{
	std::vector<domain_info_t> v_domains;

	// Domains
	ICorDebugAppDomainEnum * appdomain = nullptr;
	if (cor_debug_process->EnumerateAppDomains(&appdomain) != S_OK)
		return std::nullopt;

	XSFD_DEFER { appdomain->Release(); };

	ULONG domain_count = 0;
	appdomain->GetCount(&domain_count);
	xsfd::log("!Domain count: %d\n", domain_count);
	auto domains = std::make_unique<ICorDebugAppDomain * []>(domain_count);
	if (!domains)
	{
		xsfd::log("!Domain container alloc failed.\n");
		return std::nullopt;
	}

	XSFD_DEFER {
		for (int i = 0; i < domain_count; ++i)
			domains[i]->Release();
	};

	ULONG domain_count_fetched = 0;
	if (appdomain->Next(domain_count, domains.get(), &domain_count_fetched) != S_OK || domain_count != domain_count_fetched)
	{
		xsfd::log("!Domain fetch failed (%d:%d)\n", domain_count, domain_count_fetched);
		return std::nullopt;
	}

	for (int i_domain = 0; i_domain < domain_count; ++i_domain)
	{
		ICorDebugAppDomain * domain = domains[i_domain];
		WCHAR nbuff[128] = {};
		ULONG32 nlen = 0;

		if (domain->GetName(127, &nlen, nbuff) != S_OK)
		{
			xsfd::log("!ICorDebugAppDomain::GetName failed.\n");
			continue;
		}

		auto & v_domain = v_domains.emplace_back(domain_info_t { xsfd::wc2u8(nbuff), {} });

		// Assembly
		ICorDebugAssemblyEnum * assembly_enum = nullptr;
		if (domain->EnumerateAssemblies(&assembly_enum) != S_OK)
		{
			xsfd::log("!ICorDebugAppDomain::EnumerateAssemblies failed.\n");
			continue;
		}
		XSFD_DEFER { assembly_enum->Release(); };

		ULONG assembly_fetched = 0;
		ICorDebugAssembly * assembly = nullptr;
		while (assembly_enum->Next(1, &assembly, &assembly_fetched) == S_OK && assembly_fetched == 1)
		{
			XSFD_DEFER { assembly->Release(); };
			if (assembly->GetName(127, &nlen, nbuff) != S_OK)
			{
				xsfd::log("!ICorDebugAssembly::GetName failed.\n");
				continue;
			}

			auto s_assembly = xsfd::wc2u8(nbuff);
			auto & v_assembly = v_domain.assemblies.emplace_back(assembly_info_t { s_assembly.substr(s_assembly.find_last_of('\\') + 1), {} });

			// Modules
			ICorDebugModuleEnum * module_enum = nullptr;
			if (assembly->EnumerateModules(&module_enum) != S_OK)
			{
				xsfd::log("!ICorDebugAssembly::EnumerateModules failed.\n");
				continue;
			}
			XSFD_DEFER { module_enum->Release(); };

			ICorDebugModule * mod = nullptr;
			ULONG mod_fetched = 0;
			while (module_enum->Next(1, &mod, &mod_fetched) == S_OK && mod_fetched == 1)
			{
				XSFD_DEFER { mod->Release(); };
				CORDB_ADDRESS address = 0;
				if (mod->GetName(127, &nlen, nbuff) != S_OK || mod->GetBaseAddress(&address) != S_OK)
				{
					xsfd::log("!ICorDebugModule::GetName failed.\n");
					continue;
				}

				auto & v_modules = v_assembly.modules.emplace_back(modules_info_t { xsfd::wc2u8(nbuff), {} });

				// Meta data
				IMetaDataImport * metadata = nullptr;
				if (mod->GetMetaDataInterface(IID_IMetaDataImport, reinterpret_cast<IUnknown **>(&metadata)) != S_OK)
				{
					xsfd::log("!ICorDebugModule::GetMetaDataInterface failed.\n");
					continue;
				}
				XSFD_DEFER { metadata->Release(); };

				// Type definitions
				HCORENUM td_hce = 0;
				ULONG td_count = 0;

				metadata->EnumTypeDefs(&td_hce, nullptr, 0, nullptr);
				if (metadata->CountEnum(td_hce, &td_count) != S_OK)
				{
					xsfd::log("!IMetaDataImport::CountEnum failed.\n");
					continue;
				}

				auto typedefs = std::make_unique<mdTypeDef[]>(td_count);
				if (!typedefs || metadata->EnumTypeDefs(&td_hce, typedefs.get(), td_count, &td_count) != S_OK)
				{
					xsfd::log("!IMetaDataImport::EnumTypeDefs failed.\n");
					continue;
				}

				for (int i_tds = 0; i_tds < td_count; ++i_tds)
				{
					// Type definition props
					ULONG td_pch = 0;
					DWORD td_flags = 0;
					mdToken ext;
					if (metadata->GetTypeDefProps(typedefs[i_tds], nbuff, sizeof(nbuff) / sizeof(nbuff[0]), &td_pch, &td_flags, &ext) != S_OK)
						continue;

					auto & v_typedefs = v_modules.typedefs.emplace_back(typedef_info_t { xsfd::wc2u8(nbuff) });

					// Methods
					HCORENUM mth_hce   = 0;
					ULONG    mth_count = 0;
					metadata->EnumMethods(&mth_hce, typedefs[i_tds], nullptr, 0, nullptr);
					if (metadata->CountEnum(mth_hce, &mth_count) != S_OK)
						continue;

					auto methods = std::make_unique<mdMethodDef[]>(mth_count);
					if (!methods || metadata->EnumMethods(&mth_hce, typedefs[i_tds], methods.get(), mth_count, &mth_count) != S_OK)
						continue;

					for (int i_mth = 0; i_mth < mth_count; ++i_mth)
					{
						PCCOR_SIGNATURE sig     = 0;
						mdTypeDef       mdtd    = 0;
						ULONG           pchmth  = 0;
						DWORD           attr    = 0;
						ULONG           sigblob = 0;
						ULONG           rva     = 0;
						DWORD           flags   = 0;

						if (metadata->GetMethodProps(methods[i_mth], &mdtd, nbuff, 128, &pchmth, &attr, &sig, &sigblob, &rva, &flags) != S_OK)
							continue;

						ICorDebugFunction * mth_fn = nullptr;
						if (mod->GetFunctionFromToken(methods[i_mth], &mth_fn) != S_OK)
							continue;
						XSFD_DEFER { mth_fn->Release(); };

						ICorDebugCode * mth_ilcode = nullptr;
						if (mth_fn->GetILCode(&mth_ilcode) != S_OK)
							continue;
						XSFD_DEFER { mth_ilcode->Release(); };

						CORDB_ADDRESS il_address = 0;
						if (mth_ilcode->GetAddress(&il_address) != S_OK)
							continue;
						
						ICorDebugCode * mth_natcode = nullptr;
						CORDB_ADDRESS native_address = 0;
						if (mth_fn->GetNativeCode(&mth_natcode) == S_OK)
							mth_natcode->GetAddress(&native_address);
						XSFD_DEFER { if (mth_natcode) mth_natcode->Release(); };

						methods_info_t mi = {};
						mi.md_methoddef = methods[i_mth];
						mi.name = xsfd::wc2u8(nbuff);
						mi.rva  = rva;
						mi.sig  = sig;
						mi.il_address = (void*)il_address;
						mi.native_address = (void*)native_address;
						v_typedefs.methods.emplace_back(mi);
					}
				}
			}
		}
	}

	return v_domains;
}

dotnet::wrap_Assembly::wrap_Assembly(void * assembly)
	: wrap_base(assembly)
{
}

dotnet::wrap_Assembly::~wrap_Assembly()
{
}

dotnet::wrap_base::wrap_base()
	: instance(nullptr)
{
}

dotnet::wrap_base::wrap_base(void * i)
	: instance(i)
{
}

auto dotnet::wrap_base::get_name() -> std::wstring 
{
	BSTR n = nullptr;
	if (((HRESULT(__stdcall***)(BSTR *))(instance))[0][0x30](&n) != S_OK)
		return std::wstring();
	XSFD_DEFER {
		SysFreeString(n);
	};
	return std::wstring(n);
}

dotnet::wrap_base::operator bool() const noexcept
{
	return instance;
}
