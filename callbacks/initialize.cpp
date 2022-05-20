#include "callbacks.hpp"
#include <string_view>
#include <filesystem>
#include <Windows.h>
#include <string>
#include "../xsfd_utils.hpp"
#include <optional>

static auto get_dotnet_path() -> std::optional<std::filesystem::path> 
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
		return std::nullopt;

	bool has_path = false;
	std::filesystem::path best_path;
	int best_ver[3] = { 0, 0, 0 };

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
		
		XSFD_DEBUG_LOG("!Found dotNet version: %s\n", xsfd::wc2u8(ver_str).c_str());

		int nvers[3] = {};
		if (swscanf_s(ver_str.c_str(), L"%d.%d.%d", &nvers[0], &nvers[1], &nvers[2]) != 3)
			continue;

		for (int i = 0; i < 3; ++i)
		{
			if (nvers[i] > best_ver[i])
			{
				has_path = true;
				best_path = ver_path;

				for (int vi = 0; vi < 3; ++vi)
					best_ver[vi] = nvers[vi];

				break;
			}
		}
	}

	if (!has_path)
		return std::nullopt;

	return std::make_optional(best_path);
}

auto callbacks::initialize() -> void
{	
	HANDLE proc = DbgGetProcessHandle();
	if (!proc)
	{
		xsfd::log("!ERROR: Invalid process handle!\n");
		return;
	}

	xsfd::log("!Handle: 0x%p", proc);

	if (!dotnet::meta_host)
	{	
		dotnet::meta_host = dncomlib::clr_create_meta_host_instance();
		if (!dotnet::meta_host)
		{
			xsfd::log("!ERROR: Unable to create a CLR instance -> (CLSID_CLRMetaHost, IID_ICLRMetaHost)\n");	
			return;
		}

		xsfd::log("!Created CLR instance (CLSID_CLRMetaHost, IID_ICLRMetaHost) @ 0x%p\n", *dotnet::meta_host.ppinstance());
	}

	if (!dotnet::h_dbgshim)
	{
		const auto db_path = get_dotnet_path();
		if (!db_path)
		{
			xsfd::log("!ERROR: No valid dotNet found.\n");
			return;
		}

		const auto & dbgshim_path = *db_path/L"dbgshim.dll";
		if (!std::filesystem::exists(dbgshim_path))
		{
			xsfd::log("!ERROR: dbgshim.dll was not found on the selected dotNet directory.\n");
			return;
		}

		xsfd::log("!Using dotNet: %s\n", xsfd::wc2u8(db_path->c_str()).c_str());

		dotnet::h_dbgshim = LoadLibraryW(dbgshim_path.c_str());
		if (!dotnet::h_dbgshim)
		{
			xsfd::log("!ERROR: Failed to load dbgshim.dll\n");
			return;
		}

		xsfd::log("!dbgshim.dll loaded @ 0x%p\n", dotnet::h_dbgshim);
	}

	for (const auto & runtime : dotnet::meta_host.enumerate_loaded_runtimes(proc))
	{
		auto runtime_info = dncomlib::runtime_info::from_unknown(runtime);
		if (!runtime_info)
		{
			xsfd::log("!ERROR: Failed to obtain runtime from enumerator!\n");
			return;
		}

		XSFD_DEBUG_LOG("!Found loaded runtime version: %s\n", xsfd::wc2u8(runtime_info.get_version_string()).c_str());

		auto host = runtime_info.get_host();
		if (!host)
		{
			xsfd::log("!ERROR: Invalid host\n");
			continue;
		}

		auto thunk = host.get_default_domain_thunk();
		if (!thunk)
		{
			xsfd::log("!ERROR: Invalid thunk\n");
			continue;
		}

		auto domain = dncomlib::app_domain::from_unknown(thunk);
		if (!domain)
		{
			xsfd::log("!ERROR: Invalid domain\n");
			continue;
		}

		xsfd::log("!Domain: %s\n", xsfd::wc2u8(domain.get_friendly_name()).c_str());
	}

	global::initialized = true;
	xsfd::log("!Successfuly initialized!\n");
}