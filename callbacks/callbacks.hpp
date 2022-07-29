#pragma once

namespace callbacks
{
	auto initialize() -> void;

	enum class uninit { PARTIAL, FULL };
	auto uninitialize(uninit mode) -> void;
}