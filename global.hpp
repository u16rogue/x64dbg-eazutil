#pragma once

#define XSFD_PLUG_NAME "eazutil"
#define PLUG_EXPORT extern "C" __attribute__((dllexport))

namespace global
{
	inline bool initialized = false;

	inline const char    plug_name[]  = { XSFD_PLUG_NAME };
	inline constexpr int plug_version = 1;

	inline void * hmod;
	inline int    pluginHandle;
	inline void * hwndDlg;
	inline int    hMenu;
	inline int    hMenuDisasm;
	inline int    hMenuDump;
	inline int    hMenuStack;
	inline int    h_menu_entry;
}