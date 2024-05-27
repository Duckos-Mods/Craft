#pragma once
#include <Internal/CraftUtils.hpp>

namespace Craft
{
	enum class OSErr : u8
	{
		NONE,
		ALLOCATE,
		PROTECT,
		QUERY,
		UNKNOWN_PROTECTION,
		NO_MEMORY_IN_RANGE,
	};
} 