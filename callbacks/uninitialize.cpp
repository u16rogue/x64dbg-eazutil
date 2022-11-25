#include "callbacks.hpp"

#include "../global.hpp"
#include "../xsfd_utils.hpp"
#include "../menu.hpp"
#include "../dotnet.hpp"
#include "../eazlib.hpp"

auto callbacks::uninitialize(uninit mode) -> void
{
	if (mode == uninit::FULL)
	{
		menu::dispose();
		if (eaz::decode_fn)
			eaz::decode_fn = dotnet::wrap_MethodInfo();
		// no idea how to unload the assembly lol
		if (eaz::eazlib_module)
			eaz::eazlib_module = dotnet::wrap_Assembly();

		dotnet::destroy();
	}
	else
	{
		dotnet::uninitialize();
	}

	global::initialized = false;
	_plugin_menuentrysetname(global::pluginHandle, menuid_initalize, "Initialize");
}