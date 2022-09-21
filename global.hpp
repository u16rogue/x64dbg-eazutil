#pragma once

#include <memory>
#include "msdnlib.hpp"

#define XSFD_PLUG_NAME "eazutil"

#if defined(__clang__)
	#define PLUG_EXPORT extern "C" __attribute__((dllexport))
#elif defined(_MSC_VER)
	#define PLUG_EXPORT extern "C" __declspec(dllexport)
#else
	#error "Unsupported compiler - PLUG_EXPORT definition"
#endif

// TODO: namespace this
constexpr int menuid_initalize = 1;
constexpr int menuid_menuvis   = 2;

namespace global
{
	inline bool initialized = false;

	inline const char    plug_name[]  = { XSFD_PLUG_NAME };
	inline constexpr int plug_version = 1;

	inline void * hmod         = nullptr;
	inline int    pluginHandle = 0;
	inline void * hwndDlg      = nullptr;

	inline int    hMenu        = 0;
	inline int    h_menu_entry = 0;

	inline int    hMenuDisasm  = 0;
	inline int    hMenuDump    = 0;
	inline int    hMenuStack   = 0;
}
