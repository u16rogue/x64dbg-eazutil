#pragma once

#include <Windows.h>
#include <_plugins.h>
#include "global.hpp"
#include <string>
#include <format>
#include <array>

#ifdef XSFDEU_DEBUG
	#include <stdio.h>

	#define XSFD_DEBUG_LOG(x, ...) \
		xsfd::log(x, __VA_ARGS__)
#else
	#define XSFD_DEBUG_LOG(x, ...)
#endif


namespace xsfd::details
{
	template <typename T>
	struct _defer
	{
		_defer(T && v_)
			: v(v_)
		{}

		~_defer() { v(); }

		const T v;
	};
}

#define XSFD_GLUE2(m1, m2) m1##m2
#define XSFD_GLUE(m1, m2) XSFD_GLUE2(m1, m2)

#define XSFD_DEFER \
	xsfd::details::_defer XSFD_GLUE(___DEFER, __LINE__) = [&]()

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

	template <int len> requires (len % 3 == 0)
	struct ida_pattern_literal
	{
		struct fragment
		{
			unsigned char byte;
			bool masked;
		};

		consteval ida_pattern_literal(const char (& pattern)[len])
		{
			for (int i = 0; i < len / 3; ++i)
			{
				auto chunk = &pattern[i * 3];
				auto char_to_nibble = [](unsigned char c) -> unsigned char
				{
					if (c == '?')
						return 0;

					if (c >= '0' && c <= '9')
						return c - '0';
					else if (c >= 'A' && c <= 'F')
						return c - 'A' + 0xA;
					else if (c >= 'a' && c <= 'f')
						return c - 'a' + 0xa;
					
					return 0;
				};

				unsigned char byte = char_to_nibble(chunk[0]) << 4 | char_to_nibble(chunk[1]);
				fragments[i] = {
					.byte = static_cast<unsigned char>(byte),
					.masked = chunk[0] != '?'
				};
			}
		}

		std::array<fragment, len / 3> fragments {};
	};

	constexpr std::ptrdiff_t INVALID_PATTERN_RVA = -1;

	template <ida_pattern_literal pattern>
	auto pattern_scan_rva(void * _buffer, std::size_t size) -> std::ptrdiff_t
	{
		constexpr auto pattern_length = pattern.fragments.size();
		unsigned char * buffer = (decltype(buffer))_buffer;
		for (int i_buffer = 0; i_buffer < size; ++i_buffer)
		{
			for (int i_pattern = 0; i_pattern < pattern_length; ++i_pattern)
			{
				const auto & fragment = pattern.fragments[i_pattern];
				if (!fragment.masked)
					continue;
				if (buffer[i_buffer + i_pattern] != fragment.byte)
					break;
				if (pattern_length - 1 == i_pattern)
					return i_buffer;
			}
		}

		return INVALID_PATTERN_RVA;
	}
}