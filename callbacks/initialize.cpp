#include "callbacks.hpp"
#include <string_view>
#include <filesystem>
#include <string>
#include <memory>

#include "../msdnlib.hpp"
#include <TlHelp32.h>

#include "../global.hpp"
#include "../xsfd_utils.hpp"
#include "../menu.hpp"

auto dump_test(kita::events::on_render * e) -> void;

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

		/*
		if (!std::filesystem::exists(ver_path.path()/L"dbgshim.dll"))
		{
			xsfd::log("!WARNING: dotNet version does not contain dbgshim. Skipping!\n");
			continue;
		}
		*/

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

static HMODULE h_mscoree = nullptr;
static HRESULT(*_CLRCreateInstance)(REFCLSID, REFIID, LPVOID *) = nullptr;

auto callbacks::initialize() -> void
{	
	uninitialize(uninit::PARTIAL);

	if (!h_mscoree)
	{
		h_mscoree = LoadLibraryW(L"mscoree.dll");
		if (!h_mscoree)
		{
			xsfd::log("!ERROR: Failed to load mscoree module.\n");
			return;
		}
		XSFD_DEBUG_LOG("!Loaded mscoree module @ 0x%p\n", h_mscoree);
	}

	if (!_CLRCreateInstance)
	{
		_CLRCreateInstance = reinterpret_cast<decltype(_CLRCreateInstance)>(GetProcAddress(h_mscoree, "CLRCreateInstance"));
		if (!_CLRCreateInstance)
		{
			xsfd::log("!ERROR: Failed to load CLRCreateInstance function pointer.\n");
			return;
		}
		XSFD_DEBUG_LOG("!Loaded CLRCreateInstance @ 0x%p\n", _CLRCreateInstance);
	}

	HANDLE phnd = DbgGetProcessHandle();
	DWORD pid   = DbgGetProcessId();
	if (!phnd || !pid)
	{
		xsfd::log("!ERROR: No process handle.\n");
		return;
	}
	XSFD_DEBUG_LOG("!Process handle: 0x%p (ID: %d)\n", phnd, pid);

	// ------------------------------------------------------------------------------------------------------------------------------------------------------ 
	// Meta host
	// ------------------------------------------------------------------------------------------------------------------------------------------------------ 
	
	if (!dotnet::meta_host)
	{
		if (_CLRCreateInstance(CLSID_CLRMetaHost, IID_ICLRMetaHost, reinterpret_cast<LPVOID *>(&dotnet::meta_host)) != S_OK)
		{
			xsfd::log("!ERROR: Failed to create meta host instance.\n");
			return;
		}
		XSFD_DEBUG_LOG("!Created meta host instance @ 0x%p\n", dotnet::meta_host);
	}

	// ------------------------------------------------------------------------------------------------------------------------------------------------------ 
	// Enumerate loaded runtimes
	// ------------------------------------------------------------------------------------------------------------------------------------------------------ 

	IEnumUnknown * enum_runtime = nullptr;
	if (dotnet::meta_host->EnumerateLoadedRuntimes(phnd, &enum_runtime) != S_OK)
	{
		xsfd::log("!ERROR: Failed to create an enumerator for loaded runtimes.\n");
		return;
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
		return;
	}

	#if XSFDEU_DEBUG
	{
		wchar_t bver[24] = { L'\0' };
		DWORD bsize = sizeof(bver) / sizeof(bver[0]);
		rinfo->GetVersionString(bver, &bsize);
		xsfd::log("!Obtained runtime information %s @ 0x%p\n", xsfd::wc2u8(bver).c_str(), rinfo);
	}
	#endif

	dotnet::dlp_instance = std::make_unique<xsfd::debug_lib_provider>(rinfo);
	if (!dotnet::dlp_instance)
	{
		rinfo->Release();
		xsfd::log("!ERROR: Failed to create debug library provider");
		return;
	}
	XSFD_DEBUG_LOG("!Created debug library provider @ 0x%p\n", dotnet::dlp_instance.get());

	// ------------------------------------------------------------------------------------------------------------------------------------------------------ 
	// CLR Debugging setup
	// ------------------------------------------------------------------------------------------------------------------------------------------------------ 
	if (!dotnet::clr_debugging)
	{
		if (_CLRCreateInstance(CLSID_CLRDebugging, IID_ICLRDebugging, reinterpret_cast<LPVOID *>(&dotnet::clr_debugging)) != S_OK)
		{
			xsfd::log("!ERROR: Could not create clr debugging instance.\n");
			return;
		}
		XSFD_DEBUG_LOG("!Created CLR debugging instance @ 0x%p\n", dotnet::clr_debugging);
	}

	HANDLE mod_snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
	if (mod_snap == INVALID_HANDLE_VALUE)
	{
		xsfd::log("!Could not create module snapshot of target process. GetLastError: %d\n", GetLastError());
		return;
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
		return;
	}

	CLR_DEBUGGING_VERSION cdv = {
		.wStructVersion = 0,
		.wMajor         = 4,
		.wMinor         = 0,
		.wBuild         = 65535,
		.wRevision      = 30319,
	};
	CLR_DEBUGGING_PROCESS_FLAGS cdpf {};

	dotnet::ddt_instance = std::make_unique<xsfd::debug_data_target>(phnd);
	if (!dotnet::ddt_instance)
	{
		xsfd::log("!ERROR: Failed to create debug data target instance.\n");
		return;
	}
	XSFD_DEBUG_LOG("!Created debug data target @ 0x%p\n", dotnet::ddt_instance.get());
	XSFD_DEBUG_LOG("!Checking debuggable modules:\n");
	do
	{
		static constexpr IID IID_ICorDebugProcess = { 0x3d6f5f64, 0x7538, 0x11d3, 0x8d, 0x5b, 0x00, 0x10, 0x4b, 0x35, 0xe7, 0xef };
		XSFD_DEBUG_LOG("    %s", me32.szModule);
		if (dotnet::clr_debugging->OpenVirtualProcess(ULONG64(me32.hModule), dotnet::ddt_instance.get(), dotnet::dlp_instance.get(), &cdv, IID_ICorDebugProcess, reinterpret_cast<IUnknown **>(&dotnet::cor_debug_process), &cdv, &cdpf) == S_OK)
		{
			XSFD_DEBUG_LOG(" -- OK @ 0x%p\n", me32.szModule);
			break;
		}
		dotnet::cor_debug_process = nullptr; // Assurance :)
		XSFD_DEBUG_LOG(" -- nope\n");
	} while (Module32Next(mod_snap, &me32));

	if (!dotnet::cor_debug_process)
	{
		xsfd::log("!ERROR: Failed to open debug handle to virtual process.\n");
		return;
	}

	XSFD_DEBUG_LOG("!Open virtual process for debugging @ 0x%p\n", dotnet::cor_debug_process);

	static constexpr IID IID_ICorDebugProcess5 = { 0x21e9d9c0, 0xfcb8, 0x11df, 0x8c, 0xff, 0x08, 0x00, 0x20, 0x0c ,0x9a, 0x66 };
	if (dotnet::cor_debug_process->QueryInterface(IID_ICorDebugProcess5, reinterpret_cast<void **>(&dotnet::cor_debug_process5)) != S_OK)
	{
		xsfd::log("!ERROR: Failed to create COR debug process 5 instance.\n");
		return;
	}
	XSFD_DEBUG_LOG("!Created COR debug process 5 instance @ 0x%p\n", dotnet::cor_debug_process5);

	global::initialized = true;
	_plugin_menuentrysetname(global::pluginHandle, menuid_initalize, "Re-Initialize");

	// Test

	#if (1)
		menu::add_render(dump_test, "Test dump");
		menu::show();
	#endif
}

struct methods_info_t
{
	std::string name;
	PCCOR_SIGNATURE sig;
	ULONG rva;

	void * il_address;
	void * native_address;
};

struct typedef_info_t
{
	std::string name;
	std::vector<methods_info_t> methods;
};

struct modules_info_t
{
	std::string name;
	std::vector<typedef_info_t> typedefs;
};

struct assembly_info_t
{
	std::string name;
	std::vector<modules_info_t> modules;
};

struct domain_info_t
{
	std::string name;
	std::vector<assembly_info_t> assemblies;
};

auto dump_test(kita::events::on_render * e) -> void
{
	static std::vector<domain_info_t> v_domains;

	ImGui::Text("Domains:");
	for (const auto & domains : v_domains)
	{
		if (!ImGui::TreeNode(domains.name.c_str()))
			continue;

		ImGui::Text("Assemblies:");
		for (const auto & assemblies : domains.assemblies)
		{
			if (!ImGui::TreeNode(assemblies.name.c_str()))
				continue;

			ImGui::Text("Modules:");
			for (const auto & modules : assemblies.modules)
			{
				if (!ImGui::TreeNode(modules.name.c_str()))
					continue;

				ImGui::Text("Type definitions:");
				for (const auto & td : modules.typedefs)
				{
					if (!ImGui::TreeNode(td.name.c_str()))
						continue;

					ImGui::BeginChild("Methods:");
					for (const auto & mth : td.methods)
					{
						ImGui::Text("%s - RVA: 0x%p - IL: 0x%p - Native:", mth.name.c_str(), mth.rva, mth.il_address);
						ImGui::SameLine();
						ImGui::Text("0x%p", mth.native_address);
						if (mth.native_address)
						{
							if (ImGui::IsItemClicked())
								GuiDisasmAt((duint)mth.native_address, 0);
							if (ImGui::IsItemHovered())
								ImGui::SetTooltip("Go to address in disassembler");
						}
					}
					ImGui::EndChild();

					ImGui::TreePop();
				}
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}
		ImGui::TreePop();
	}

	if (ImGui::Button("Clear"))
		v_domains.clear();

	ImGui::SameLine();

	if (!ImGui::Button("Dump"))
		return;

	v_domains.clear();

	xsfd::log("!Dumping...\n");
	// Domains
	ICorDebugAppDomainEnum * appdomain = nullptr;
	if (dotnet::cor_debug_process->EnumerateAppDomains(&appdomain) != S_OK)
		return;

	XSFD_DEFER { appdomain->Release(); };

	ULONG domain_count = 0;
	appdomain->GetCount(&domain_count);
	xsfd::log("!Domain count: %d\n", domain_count);
	auto domains = std::make_unique<ICorDebugAppDomain * []>(domain_count);
	if (!domains)
	{
		xsfd::log("!Domain container alloc failed.\n");
		return;
	}

	XSFD_DEFER {
		for (int i = 0; i < domain_count; ++i)
			domains[i]->Release();
	};

	ULONG domain_count_fetched = 0;
	if (appdomain->Next(domain_count, domains.get(), &domain_count_fetched) != S_OK || domain_count != domain_count_fetched)
	{
		xsfd::log("!Domain fetch failed (%d:%d)\n", domain_count, domain_count_fetched);
		return;
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
					{
						xsfd::log("!IMetaDataImport::GetTypeDefProps failed.\n");
						continue;
					}

					auto & v_typedefs = v_modules.typedefs.emplace_back(typedef_info_t { xsfd::wc2u8(nbuff) });

					// Methods
					HCORENUM mth_hce = 0;
					ULONG mth_count = 0;
					metadata->EnumMethods(&mth_hce, typedefs[i_tds], nullptr, 0, nullptr);
					if (metadata->CountEnum(mth_hce, &mth_count) != S_OK)
					{
						xsfd::log("!IMetaDataImport::CountEnum failed.\n");
						continue;
					}

					auto methods = std::make_unique<mdMethodDef[]>(mth_count);
					if (!methods || metadata->EnumMethods(&mth_hce, typedefs[i_tds], methods.get(), mth_count, &mth_count) != S_OK)
					{
						//xsfd::log("!IMetaDataImport::EnumMethods failed.\n");
						continue;
					}

					for (int i_mth = 0; i_mth < mth_count; ++i_mth)
					{
						mdTypeDef mdtd = 0;
						ULONG pchmth = 0;
						DWORD attr = 0;
						PCCOR_SIGNATURE sig = 0;
						ULONG sigblob = 0;
						ULONG rva = 0;
						DWORD flags = 0;
						if (metadata->GetMethodProps(methods[i_mth], &mdtd, nbuff, 128, &pchmth, &attr, &sig, &sigblob, &rva, &flags) != S_OK)
						{
							//xsfd::log("!IMetaDataImport::GetMethodProps failed.\n");
							continue;
						}

						ICorDebugFunction * mth_fn = nullptr;
						if (mod->GetFunctionFromToken(methods[i_mth], &mth_fn) != S_OK)
						{
							//xsfd::log("!IMetaDataImport::GetFunctionFromToken failed.\n");
							continue;
						}
						XSFD_DEFER { mth_fn->Release(); };

						ICorDebugCode * mth_ilcode = nullptr;
						if (mth_fn->GetILCode(&mth_ilcode) != S_OK)
						{
							//xsfd::log("!ICorDebugFunction::GetILCode failed.\n");
							continue;
						}
						XSFD_DEFER { mth_ilcode->Release(); };

						CORDB_ADDRESS il_address = 0;
						if (mth_ilcode->GetAddress(&il_address) != S_OK)
						{
							//xsfd::log("!ICorDebugCode::GetAddress failed.\n");
							continue;
						}
						
						ICorDebugCode * mth_natcode = nullptr;
						if (mth_fn->GetNativeCode(&mth_natcode) != S_OK)
						{
							//xsfd::log("!ICorDebugFunction::GetNativeCode failed.\n");
						}
						XSFD_DEFER { if (mth_natcode) mth_natcode->Release(); };

						CORDB_ADDRESS native_address = 0;
						if (mth_natcode && mth_natcode->GetAddress(&native_address) != S_OK)
						{
							//xsfd::log("!ICorDebugCode::GetAddress failed.\n");
						}

						methods_info_t mi = {};
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

	xsfd::log("!Dump completed.\n");
}
