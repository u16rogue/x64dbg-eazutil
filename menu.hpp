#pragma once

#include <kita/kita.hpp>

namespace menu
{
	auto valid() -> bool;
	auto toggle() -> bool;
	auto show() -> bool;
	auto hide() -> bool;
	auto dispose() -> bool;

	auto add_render(kita::events::details::on_render_cb_t f, const char * title) -> bool;
}