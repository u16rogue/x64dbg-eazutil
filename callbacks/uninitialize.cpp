#include "callbacks.hpp"

#include "../global.hpp"
#include "../xsfd_utils.hpp"
#include "../menu.hpp"

auto callbacks::uninitialize(uninit mode) -> void
{
	if (mode == uninit::FULL)
		menu::dispose();

	if (dotnet::cor_debug_process5)
	{
		XSFD_DEBUG_LOG("!Releasing COR Debug Process instance 5.\n");
		dotnet::cor_debug_process5->Release();
		dotnet::cor_debug_process5 = nullptr;
	}

	if (dotnet::cor_debug_process)
	{
		XSFD_DEBUG_LOG("!Releasing COR Debug Process instance.\n");
		dotnet::cor_debug_process->Release();
		dotnet::cor_debug_process = nullptr;
	}

	if (dotnet::ddt_instance)
	{
		XSFD_DEBUG_LOG("!Releasing debug data target instance.\n");
		dotnet::ddt_instance = nullptr;
	}

	if (dotnet::dlp_instance)
	{
		XSFD_DEBUG_LOG("!Releasing data library provider instance.\n");
		dotnet::dlp_instance = nullptr;
	}

	if (mode == uninit::FULL)
	{
		if (dotnet::clr_debugging)
		{
			XSFD_DEBUG_LOG("!Releasing CLR Debugging instance.\n");
			dotnet::clr_debugging->Release();
			dotnet::clr_debugging = nullptr;
		}

		if (dotnet::meta_host)
		{
			XSFD_DEBUG_LOG("!Releasing Meta host instance.\n");
			dotnet::meta_host->Release();
			dotnet::meta_host = nullptr;
		}
	}

	global::initialized = false;
	_plugin_menuentrysetname(global::pluginHandle, menuid_initalize, "Initialize");
}