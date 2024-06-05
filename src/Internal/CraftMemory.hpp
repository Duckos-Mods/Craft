#pragma once
#include <Internal/CraftUtils.hpp>
#include <expected>
#include <Internal/OS.hpp>

namespace Craft
{
	u32 GetOsPageSize();
	namespace Sizes
	{
		constexpr u64 byte = 1;
		constexpr u64 KiB = 1024 * byte;
		constexpr u64 MiB = 1024 * KiB;
		constexpr u64 GiB = 1024 * MiB;
		constexpr u64 TiB = 1024 * GiB;
		constexpr u64 PiB = 1024 * TiB;
		constexpr u64 EiB = 1024 * PiB;
		constexpr u64 ZiB = 1024 * EiB;
		constexpr u64 YiB = 1024 * ZiB;
		constexpr u64 KB = 1000 * byte;
		constexpr u64 MB = 1000 * KB;
		constexpr u64 GB = 1000 * MB;
		constexpr u64 TB = 1000 * GB;
		constexpr u64 PB = 1000 * TB;
		constexpr u64 EB = 1000 * PB;
		constexpr u64 ZB = 1000 * EB;
		constexpr u64 YB = 1000 * ZB;


	}


	struct MemAccess
	{
		bool read : 1;
		bool write : 1;
		bool execute : 1;

		bool operator==(const MemAccess other) const {return read == other.read && write == other.write && execute == other.execute;}

		std::expected<protType, OSErr> ToOsProtection() const
		{
			DWORD Protection = 0;

			if (*this == MemAccess{ true, false, false })
				Protection = PAGE_READONLY;
			else if (*this == MemAccess{ true, false, true })
				Protection = PAGE_EXECUTE_READ;
			else if (*this == MemAccess{ true, true, false })
				Protection = PAGE_READWRITE;
			else if (*this == MemAccess{ true, true, true })
				Protection = PAGE_EXECUTE_READWRITE;
			else
				return std::unexpected(OSErr::UNKNOWN_PROTECTION);
			return Protection;
		}
	};
	struct MemInfo
	{
		u32 pageSize = GetOsPageSize();
		uSize minAddress = 0;
		uSize maxAddress = 0;
		u64 allocationGranularity = 0;
		MemInfo() = default;
		MemInfo(u32 pageSize, uSize minAddress, uSize maxAddress, u64 allocationGranularity) : pageSize(pageSize), minAddress(minAddress), maxAddress(maxAddress), allocationGranularity(allocationGranularity) {}
		static u32 GetPageSize() { return GetOsPageSize(); }
	};

	struct MemRegion
	{
		void* address = nullptr;
		uSize size = 0;
		MemAccess access = { false, false, false };
		bool isFree = false;

	};

	MemInfo GetMemInfo();

	uSize AlignUp(uSize size, uSize alignment);
	uSize AlignDown(uSize size, uSize alignment);
	std::expected<void*, OSErr> OSAlloc(uSize Count, MemAccess access, void* address = nullptr, uSize maxDistance = (uSize)-1, u32 alignment = MemInfo::GetPageSize());
	OSErr OSProtect(void* address, uSize size, MemAccess access);

	template<typename T>
	T* Talloc(size_t size, size_t alignment = 0x10)
	{
		return (T*)OSAlloc(size * sizeof(T), MemAccess{ true, true, true }, nullptr, (uSize)-1, alignment).value();

	}

	template<typename T>
	void Tfree(T* ptr)
	{
		_aligned_free(ptr);
	}

}