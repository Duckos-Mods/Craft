#include "CraftMemory.hpp"
#if CRAFT_PLATFORM_WINDOWS == 1
#define NOMINMAX
#include <Windows.h>
#elif CRAFT_PLATFORM_LINUX == 1
#endif


namespace Craft
{
	static std::expected<void*, OSErr> RawOsAlloc(void* addrss, uSize size, u32 protection)
	{
		void* ptr = VirtualAlloc(addrss, size, MEM_RESERVE | MEM_COMMIT, protection);
		if (!ptr)
			return std::unexpected(OSErr::ALLOCATE);
		return ptr;

	}

	u32 GetOsPageSize()
	{
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		return sysInfo.dwPageSize;
	}

	MemInfo GetMemInfo()
	{
		static MemInfo info;
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		info.pageSize = sysInfo.dwPageSize;
		info.minAddress = (uSize)sysInfo.lpMinimumApplicationAddress;
		info.maxAddress = (uSize)sysInfo.lpMaximumApplicationAddress;
		info.allocationGranularity = sysInfo.dwAllocationGranularity;
		return info;


	}

	/*
	* Aligns the given address to the next multiple of the given alignment.
	* @param address The address to align.
	* @param alignment The alignment to use.
	* @return The aligned address.
	*
	*/
	uSize AlignUp(uSize address, uSize alignment)
    {
		uintptr_t ptr = (uintptr_t)address;
		uintptr_t alignedPtr = (ptr + alignment - 1) & ~(alignment - 1);
		return (uSize)alignedPtr;
    }

	uSize AlignDown(uSize size, uSize alignment)
	{
		uintptr_t ptr = (uintptr_t)size;
		uintptr_t alignedPtr = ptr & ~(alignment - 1);
		return (uSize)alignedPtr;
	}

    std::expected<void*, OSErr> OSAlloc(uSize Count, MemAccess access, void* address, uSize maxDistance, u32 alignment)
    {
		if (address == nullptr)
		{
			maxDistance = 0;
			address = (void*)GetMemInfo().minAddress;
		}
		if (maxDistance == (size_t)-1)
			maxDistance = Sizes::GiB * 10;

		PointerWrapper baseAddress = (u64)address - maxDistance;
		PointerWrapper maxAddress = (u64)address + maxDistance;
		if (maxAddress == baseAddress)
			maxAddress = (u64)GetMemInfo().maxAddress;

		protType prot;
		auto acc = access.ToOsProtection();
		if (!acc.has_value())
			return std::unexpected(OSErr::UNKNOWN_PROTECTION);
		prot = acc.value();
		PointerWrapper currentAddress = baseAddress;
		while (currentAddress < maxAddress)
		{
			PointerWrapper alignedUp = AlignUp(currentAddress, alignment);

			auto allocated = RawOsAlloc(alignedUp, Count, prot);
			if (allocated.has_value())
				return allocated;
			currentAddress = currentAddress.value + GetMemInfo().allocationGranularity;
		}
		return std::unexpected(OSErr::NO_MEMORY_IN_RANGE);
    }
	OSErr OSProtect(void* address, uSize size, MemAccess access)
	{
		protType prot;
		if (!access.ToOsProtection().has_value())
			return OSErr::UNKNOWN_PROTECTION;
		prot = access.ToOsProtection().value();
		DWORD oldProt;
		if (!VirtualProtect(address, size, prot, &oldProt))
			return OSErr::PROTECT;
		return OSErr::NONE;

	}
}
