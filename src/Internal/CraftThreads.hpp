#pragma once

#include <Internal/CraftUtils.hpp>

namespace Craft
{
	/*
	* On construction locks all threads but the current one
	* On destruction unlocks all threads
	*/
	class AllOSThreads
	{
	public:
		AllOSThreads();
		~AllOSThreads() noexcept(false);

		AllOSThreads(const AllOSThreads&) = delete;
		AllOSThreads& operator=(const AllOSThreads&) = delete;

		AllOSThreads(AllOSThreads&&) = delete;
		AllOSThreads& operator=(AllOSThreads&&) = delete;

	private:
	};

}