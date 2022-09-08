#pragma once

#include "global.hpp"

namespace dotnet
{
	auto initialize() -> bool;
	auto uninitialize() -> bool;
	auto destroy() -> bool;

	auto dump() -> bool;
}