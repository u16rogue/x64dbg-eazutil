#pragma once

#include <memory>
#include "msdnlib.hpp"

#define XSFD_PLUG_NAME "eazutil"
#define PLUG_EXPORT extern "C" __attribute__((dllexport))

// TODO: namespace this
constexpr int menuid_initalize = 1;

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
	// Non volatile (instance can be kept throughout lifetime)
	inline ICLRMetaHost    * meta_host     = nullptr;
	inline ICLRDebugging   * clr_debugging = nullptr;

	// Volatile (instance is recreated and is only placed here for global access)
	// inline ICLRRuntimeInfo * runtime_info  = nullptr;
	inline std::unique_ptr<xsfd::debug_lib_provider> dlp_instance = nullptr;
	inline std::unique_ptr<xsfd::debug_data_target>  ddt_instance = nullptr;
	inline ICorDebugProcess  * cor_debug_process  = nullptr;
	inline ICorDebugProcess5 * cor_debug_process5 = nullptr;
}