#include <Windows.h>
#include <_plugins.h>
#include "xsfd_utils.hpp"
#include "global.hpp"

PLUG_EXPORT auto APIENTRY DllMain(HMODULE hmod, DWORD reason, LPVOID reserved) -> BOOL
{
	if (reason == DLL_PROCESS_ATTACH)
		global::hmod = hmod;

	return TRUE;
}

PLUG_EXPORT auto pluginit(PLUG_INITSTRUCT * s) -> bool
{
	global::pluginHandle = s->pluginHandle;	

	for (const char & n : global::plug_name)
		s->pluginName[&n - global::plug_name] = n;

	s->pluginVersion = global::plug_version;
	s->sdkVersion    = PLUG_SDKVERSION;

	return true;
}

PLUG_EXPORT auto plugsetup(PLUG_SETUPSTRUCT* s) -> void
{
	xsfd::log("!Initializing plugin ( version: %d | build: " __DATE__ " " __TIME__ " )...\n", global::plug_version);

	global::hwndDlg     = s->hwndDlg;
	global::hMenu       = s->hMenu;
	global::hMenuDisasm = s->hMenuDisasm;
	global::hMenuDump   = s->hMenuDump;
	global::hMenuStack  = s->hMenuStack;

	if (!_plugin_menuadd(global::hMenu, global::plug_name))
	{
		xsfd::log("!Failed to add a menu entry!");
		return;
	}
}

PLUG_EXPORT auto plugstop() -> void
{	
	xsfd::log("!Unloading " XSFD_PLUG_NAME "...\n");
}
