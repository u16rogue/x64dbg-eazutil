#pragma once

#include <Windows.h>
#include <_plugins.h>
#include "global.hpp"
#include <string>
#include <format>

namespace xsfd
{
	template <typename... vargs_t>
	auto log(const std::string_view fmt, vargs_t... vargs) -> void
	{
		char buffer[512] = {};
		volatile char * cb = buffer;

		const char * _fmt = fmt.data();
		if (fmt.starts_with("!"))
		{
			GuiAddLogMessage("[" XSFD_PLUG_NAME "] ");
			++_fmt;
		}

		sprintf_s(buffer, _fmt, vargs...);
		GuiAddLogMessage(buffer);

		//GuiAddLogMessage(std::format(fmt.data(), vargs...).c_str());	
	}	
}