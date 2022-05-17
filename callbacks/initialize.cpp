#include "callbacks.hpp"
#include <string_view>
#include <filesystem>
#include <Windows.h>
#include <string>
#include "../xsfd_utils.hpp"
#include <optional>

#include <mscoree.h>

#ifdef XSFDEU_DEBUG
	#include <codecvt>
#endif

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

		#ifdef XSFDEU_DEBUG
		{
			std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
			xsfd::log("!Found dotNet version: %s\n", conv.to_bytes(ver_str).c_str());
		}
		#endif

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
	if (!dotnet::meta_host)
	{
		// CLSID_CLRMetaHost
		// IID_ICLRMetaHost

		constexpr dncomlib::GUID CLSID_CLRMetaHost = {
			.Data1 = 0x9280188d,
			.Data2 = 0x0E8E,
			.Data3 = 0x4867,
			.Data4 = { 0xB3, 0x0C, 0x7F, 0xA8, 0x38, 0x84, 0xE8, 0xDE }
		};

		constexpr dncomlib::GUID IID_ICLRMetaHost = {
			.Data1 = 0xD332DB9E,
			.Data2 = 0xB9B3,
			.Data3 = 0x4125,
			.Data4 = { 0x82, 0x07, 0xA1, 0x48, 0x84, 0xF5, 0x32, 0x16 }
		};

		dotnet::meta_host = dncomlib::clr_create_instance(CLSID_CLRMetaHost, IID_ICLRMetaHost);
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

		dotnet::h_dbgshim = LoadLibraryW(dbgshim_path.c_str());
		if (!dotnet::h_dbgshim)
		{
			xsfd::log("!ERROR: Failed to load dbgshim.dll\n");
			return;
		}

		xsfd::log("!dbgshim.dll loaded @ 0x%p\n", dotnet::h_dbgshim);
	}

	global::initialized = true;
	xsfd::log("!Successfuly initialized!\n");
}