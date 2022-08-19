#include <Windows.h>
#include <_plugins.h>
#include "xsfd_utils.hpp"
#include "global.hpp"
#include "callbacks/callbacks.hpp"
#include "menu.hpp"

static auto plug_cb_menuentry(CBTYPE btype, void * _s) -> void
{
	PLUG_CB_MENUENTRY * s = reinterpret_cast<decltype(s)>(_s);

	switch (s->hEntry)
	{
		case menuid_initalize:
			callbacks::initialize();
			break;
		case menuid_menuvis:
			if (menu::valid())
				menu::toggle();
			break;
		default:
			xsfd::log("!WARNING: Received menuentry callback but was not handled");
			break;
	}

	return;
}

static auto plug_cb_detach_and_stop(CBTYPE btype, void * _s) -> void
{
	callbacks::uninitialize(callbacks::uninit::PARTIAL);
}

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

PLUG_EXPORT auto plugsetup(PLUG_SETUPSTRUCT * s) -> void
{
	xsfd::init();

	#ifdef XSFDEU_DEBUG
		constexpr char btype[] = { "DEBUG" };
	#else
		constexpr char btype[] = { "RELEASE" };
	#endif

	xsfd::log("!Initializing plugin ( %s | version: %d | build: " __DATE__ " " __TIME__ " )...\n", btype, global::plug_version);

	global::hwndDlg     = s->hwndDlg;
	global::hMenu       = s->hMenu;
	global::hMenuDisasm = s->hMenuDisasm;
	global::hMenuDump   = s->hMenuDump;
	global::hMenuStack  = s->hMenuStack;

	global::h_menu_entry = _plugin_menuadd(global::hMenu, global::plug_name); 
	if (!global::h_menu_entry)
	{
		xsfd::log("!Failed to add a menu entry!\n");
		return;
	}

	if (!_plugin_menuaddentry(global::hMenu, menuid_initalize, "Initialize")
	||  !_plugin_menuaddentry(global::hMenu, menuid_menuvis, "Toggle menu")
	) {
		xsfd::log("!Failed to add initialization menu entry!\n");
		return;
	}

	_plugin_registercallback(global::pluginHandle, CB_MENUENTRY, plug_cb_menuentry);
	_plugin_registercallback(global::pluginHandle, CB_DETACH,    plug_cb_detach_and_stop);
	_plugin_registercallback(global::pluginHandle, CB_STOPDEBUG, plug_cb_detach_and_stop);
}

PLUG_EXPORT auto plugstop() -> void
{	
	xsfd::log("!Unloading " XSFD_PLUG_NAME "...\n");
	callbacks::uninitialize(callbacks::uninit::FULL);
}