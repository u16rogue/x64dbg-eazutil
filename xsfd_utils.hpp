#pragma once

#include <Windows.h>
#include <_plugins.h>
#include "global.hpp"
#include <string>
#include <format>

#ifdef XSFDEU_DEBUG
	#include <stdio.h>

	#define XSFD_DEBUG_LOG(x, ...) \
		xsfd::log(x, __VA_ARGS__)
#else
	#define XSFD_DEBUG_LOG(x, ...)
#endif


namespace xsfd
{
	auto init() -> bool;

	template <typename... vargs_t>
	auto log(const std::string_view & fmt, vargs_t... vargs) -> void
	{
		char buffer[512] = {};

		const char * _fmt = fmt.data();
		if (fmt.starts_with("!"))
		{
			#ifdef XSFDEU_DEBUG
				printf("[" XSFD_PLUG_NAME "] ");
			#endif
			GuiAddLogMessage("[" XSFD_PLUG_NAME "] ");
			++_fmt;
		}

		sprintf_s(buffer, _fmt, vargs...);
		#ifdef XSFDEU_DEBUG
			printf("%s", buffer);
		#endif
		GuiAddLogMessage(buffer);

		//GuiAddLogMessage(std::format(fmt.data(), vargs...).c_str()); [10/05/2022] clang 14 doesn't support cxx20's format library apparently
	}	

	auto wc2u8(const std::wstring_view & str) -> std::string;
}