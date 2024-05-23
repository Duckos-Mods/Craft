#include "CraftMemory.hpp"
#if CRAFT_PLATFORM_WINDOWS == 1
#define NOMINMAX
#include <Windows.h>
#elif CRAFT_PLATFORM_LINUX == 1
#include <sys/mman.h>
#endif


namespace Craft
{
	static std::expected<void*, OSErr> RawOsAlloc(void* addrss, uSize size, u32 protection)
	{
		#if CRAFT_PLATFORM_WINDOWS == 1
		void* ptr = VirtualAlloc(addrss, size, protection, MEM_RESERVE | MEM_COMMIT);
		if (!ptr)
			return std::unexpected(OSErr::ALLOCATE);
		#elif CRAFT_PLATFORM_LINUX == 1
		void* ptr = mmap(addrss, size, protection, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (ptr == MAP_FAILED)
			return std::unexpected(OSErr::ALLOCATE);
		#endif
		return ptr;

	}

	u32 GetOsPageSize()
	{
        #if CRAFT_PLATFORM_WINDOWS == 1
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		return sysInfo.dwPageSize;
		#elif CRAFT_PLATFORM_LINUX == 1
		return static_cast<uint32_t>(sysconf(_SC_PAGESIZE));
		#endif
	}

	MemInfo GetMemInfo()
	{
		static MemInfo info;
#if CRAFT_PLATFORM_WINDOWS == 1
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		info.pageSize = sysInfo.dwPageSize;
		info.minAddress = (uSize)sysInfo.lpMinimumApplicationAddress;
		info.maxAddress = (uSize)sysInfo.lpMaximumApplicationAddress;
		#elif CRAFT_PLATFORM_LINUX == 1
		info.pageSize = static_cast<uint32_t>(sysconf(_SC_PAGESIZE));
		info.minAddress = 0x10000;
		info.maxAddress = 1ull << 47;
		#endif
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

		if (!access.ToOsProtection().has_value())
			return std::unexpected(access.ToOsProtection().error());
		prot = access.ToOsProtection().value();

		if (!address)
			if (auto res = RawOsAlloc(nullptr, allocationSize, prot))
				return res.value();
			else 
				return std::unexpected(res.error());

		MemInfo info = GetMemInfo();
		auto start = info.minAddress;
		auto end = info.maxAddress;
		auto desiredAddress = (uSize)address;
		if ((uSize)(desiredAddress - start) > maxDistance)
			start = desiredAddress - maxDistance;
		if ((end - desiredAddress) > maxDistance)
			end = desiredAddress + maxDistance;

		return nullptr;

    }
	OSErr OSProtect(void* address, uSize size, MemAccess access)
	{
		protType prot;
		if (!access.ToOsProtection().has_value())
			return OSErr::UNKNOWN_PROTECTION;
		prot = access.ToOsProtection().value();
		#if CRAFT_PLATFORM_WINDOWS == 1
		DWORD oldProt;
		if (!VirtualProtect(address, size, prot, &oldProt))
			return OSErr::PROTECT;
		#elif CRAFT_PLATFORM_LINUX == 1
		if (mprotect(address, size, prot))
			return OSErr::PROTECT;
		#endif
		return OSErr::NONE;

	}
}
