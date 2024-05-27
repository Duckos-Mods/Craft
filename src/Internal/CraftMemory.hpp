#pragma once
#include <Internal/CraftUtils.hpp>
#include <expected>
#include <Internal/OS.hpp>

namespace Craft
{
	u32 GetOsPageSize();


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
		MemInfo() = default;
		MemInfo(u32 pageSize, uSize minAddress, uSize maxAddress) : pageSize(pageSize), minAddress(minAddress), maxAddress(maxAddress) {}
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