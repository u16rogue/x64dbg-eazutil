#include "callbacks.hpp"
#include <string_view>
#include <filesystem>
#include <string>
#include "../xsfd_utils.hpp"
#include <optional>

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
	const auto db_path = get_dotnet_path();
	if (!db_path)
	{
		xsfd::log("!ERROR: No valid dotNet found.\n");
		return;
	}
	
	if (!std::filesystem::exists(*db_path/L"dbgshim.dll"))
	{
		xsfd::log("!ERROR: dbgshim.dll was not found on the selected dotNet directory.");
		return;
	}

	

	global::initialized = true;
}