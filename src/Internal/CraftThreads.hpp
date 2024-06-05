#pragma once

#include <Internal/CraftUtils.hpp>

namespace Craft
{
	/*
	* RAII class to pause all OS threads that isnt the current thread. 
	* Resumes all threads when the object goes out of scope.
	*/
	class AllOSThreads
	{
	public:
		AllOSThreads() noexcept(false);
		~AllOSThreads() noexcept(false);

		AllOSThreads(const AllOSThreads&) = delete;
		AllOSThreads& operator=(const AllOSThreads&) = delete;

		AllOSThreads(AllOSThreads&&) = delete;
		AllOSThreads& operator=(AllOSThreads&&) = delete;

	private:
	};

}