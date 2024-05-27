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
		void* ptr = VirtualAlloc(addrss, size, protection, MEM_RESERVE | MEM_COMMIT);
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

	bool inRange(void* address, void* desired, uSize maxDistance)
	{
		const uSize addr = (uSize)address;
		const uSize desiredAddr = (uSize)desired;

		uSize delta = (addr > desiredAddr) ? addr - desiredAddr : desiredAddr - addr;
		return delta < maxDistance; 

	}
	static void* tryAllocate(void* address, uSize maxDistance, void* desiredAddress, uSize size, u32 alignment)
	{
		if (!inRange(address, desiredAddress, maxDistance))
			return nullptr;

		if (auto res = RawOsAlloc(address, size, PAGE_EXECUTE_READWRITE))
			return res.value();
		return nullptr;
	}

    std::expected<void*, OSErr> OSAlloc(uSize Count, MemAccess access, void* address, uSize maxDistance, u32 alignment)
    {
		uSize allocationSize = AlignUp(Count, alignment);
		//TODO: Implement properly!
		void* data = _aligned_malloc(Count, alignment);
		if (!data)
			return std::unexpected(OSErr::ALLOCATE);
		protType prot;
		if (!access.ToOsProtection().has_value())
			return std::unexpected(access.ToOsProtection().error());
		prot = access.ToOsProtection().value();
		protType oldProt;
		VirtualProtect(data, Count, prot, &oldProt);
		return data;
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
