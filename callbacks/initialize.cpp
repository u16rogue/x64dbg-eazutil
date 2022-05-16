#include "callbacks.hpp"
#include <filesystem>
#include <string>
#include "../xsfd_utils.hpp"
#include <codecvt>

auto callbacks::initialize() -> void
{	
	xsfd::log("!Loading appropriate dotNet runtime...\n");

	// Find runtime to be loaded
	#if XSFDEU_ARCH32
		std::filesystem::path dn_ver_dir = "C:/Program Files (x86)/dotnet/shared/Microsoft.NETCore.App";
	#elif XSFDEU_ARCH64
		std::filesystem::path dn_ver_dir = "C:/Program Files/dotnet/shared/Microsoft.NETCore.App";
	#else
		#error Target architechture was not defined. Please make sure the project is properly generated and is built for an appropriate target
	#endif

	if (!std::filesystem::exists(dn_ver_dir))
	{
		xsfd::log("!Invalid dotNet version directory");
		return;
	}

	bool has_path = false;
	std::filesystem::path best_path;
	for (const auto & vers : std::filesystem::directory_iterator(dn_ver_dir))
	{
		if (!vers.is_directory())
			continue;

		const std::wstring & path_str = vers.path().native();
		const auto ver_index = path_str.find_last_of(L"/\\");
		if (ver_index == std::wstring::npos)
		{
			xsfd::log("!WARNING: dotNet version folder qualified but was not properly parsed for its version. Skipped.\n");
			continue;
		}

		std::wstring ver_str = path_str.substr(ver_index + 1); 

		#if XSFDEU_DEBUG
		{
			std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
			xsfd::log("!Found dotNet version: %s\n", conv.to_bytes(ver_str).c_str());
		}
		#endif
		
	}

	if (!has_path)
	{
		xsfd::log("!No available dotNet version found!");
		return;
	}
}