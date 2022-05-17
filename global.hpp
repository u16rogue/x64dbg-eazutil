#pragma once

#include <dncomlib/dncomlib.hpp>

#define XSFD_PLUG_NAME "eazutil"
#define PLUG_EXPORT extern "C" __attribute__((dllexport))

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

namespace dotnet
{
	inline dncomlib::meta_host meta_host;
	inline void * h_dbgshim = nullptr;
}