#include "xsfd_utils.hpp"

auto xsfd::init() -> bool
{
	#ifdef XSFDEU_DEBUG
		AllocConsole();
		FILE * dummy;
		freopen_s(&dummy, "CONOUT$", "w", stdout);
	#endif

	return true;
}

auto xsfd::wc2u8(const std::wstring_view & str) -> std::string
{
	char out[512] = {};
	WideCharToMultiByte(CP_UTF8, NULL, str.data(), -1, out, sizeof(out), NULL, nullptr);
	return std::string(out);
}
