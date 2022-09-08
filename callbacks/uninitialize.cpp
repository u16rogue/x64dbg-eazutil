#include "callbacks.hpp"

#include "../global.hpp"
#include "../xsfd_utils.hpp"
#include "../menu.hpp"
#include "../dotnet.hpp"

auto callbacks::uninitialize(uninit mode) -> void
{
	if (mode == uninit::FULL)
	{
		menu::dispose();
		dotnet::destroy();
	}
	else
	{
		dotnet::uninitialize();
	}

	global::initialized = false;
	_plugin_menuentrysetname(global::pluginHandle, menuid_initalize, "Initialize");
}