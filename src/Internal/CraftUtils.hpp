#pragma once
#include <type_traits>
#include <array>

#ifndef TO_UNDERLYING
#define TO_UNDERLYING(x) static_cast<std::underlying_type_t<decltype(x)>>(x)
#endif

#ifndef TO_ENUM
#define TO_ENUM(x, y) static_cast<x>(y)
#endif

#if !defined(__linux__)
#define CRAFT_PLATFORM_WINDOWS 1 
#define NOMINMAX
#include <Windows.h>
using protType = DWORD;
#elif defined(__linux__)
#define CRAFT_PLATFORM_LINUX 1
#include <unistd.h>
#include <sys/mman.h>
#include <unistd.h>
using protType = int;
#else
#error "Unsupported platform"
#endif

#ifndef CRAFT_NO_DEBUG && _DEBUG
#define CRAFT_DEBUG 1
#ifndef DEBUG_ONLY
#define DEBUG_ONLY(x) x
#endif
#ifndef RELEASE_ONLY
#define RELEASE_ONLY(x)
#endif
#else
#define CRAFT_DEBUG 0
#ifndef DEBUG_ONLY
#define DEBUG_ONLY(x)
#endif
#ifndef RELEASE_ONLY
#define RELEASE_ONLY(x) x
#endif
#endif
#define CRAFT_TESTS 
#ifdef CRAFT_TESTS
#include <print>
#define TEST_ONLY(x) x
#define TEST_LOG(FORMAT, ...) std::println(FORMAT, __VA_ARGS__)
#else
#define TEST_ONLY(x)
#define TEST_LOG(FORMAT, ...)
#endif


#if CRAFT_NO_THROW == 0 && CRAFT_CUSTOM_THROW == 0
#include <exception>
#define CRAFT_THROW(exception) throw exception
#define CRAFT_THROW_IF(condition, exception) if(condition) CRAFT_THROW(exception)
#define CRAFT_THROW_IF_NOT(condition, exception) if(!(condition)) CRAFT_THROW(exception)
#elif CRAFT_NO_THROW == 1 && CRAFT_CUSTOM_THROW == 0
#define CRAFT_THROW(exception) exit(1)
#define CRAFT_THROW_IF(condition, exception) exit(1)
#define CRAFT_THROW_IF_NOT(condition, exception) exit(1)
#elif CRAFT_CUSTOM_THROW == 1
#ifndef CRAFT_THROW
#error "CRAFT_THROW is not defined"
#endif
#ifndef CRAFT_THROW_IF 
#error "CRAFT_THROW_IF is not defined"
#endif
#ifndef CRAFT_THROW_IF_NOT
#error "CRAFT_THROW_IF_NOT is not defined"
#endif
#endif

#ifndef UNROLL_EXPECTED
#define UNROLL_EXPECTED(exp) ((exp.has_value()) ? exp.value() : CRAFT_THROW(exp.error()))
#endif
#ifndef CRAFT_CUSTOM_ALLOCATION_SIZE
namespace Craft {
	constexpr uint32_t cAllocSize = 4096;
}
#else
namespace Craft {
	constexpr uint32_t cAllocSize = CRAFT_CUSTOM_ALLOCATION_SIZE;
}
#endif
#ifndef SHADOW_SPACE_ALLOCATION_SIZE
#define SHADOW_SPACE_ALLOCATION_SIZE 32
#endif
#ifndef RETURN_ADDRESS_SIZE
#define RETURN_ADDRESS_SIZE 8 
// This isnt actually the size of the return address. This handles alignment due to shadow space being 32 bytes 
// The return address is 8 bytes but since the stack pointer needs to be 16 aligned there is an extra 8 bytes pushed 
#endif
#ifndef STACK_MAGIC_NUMBER
#define STACK_MAGIC_NUMBER (SHADOW_SPACE_ALLOCATION_SIZE + RETURN_ADDRESS_SIZE)
#endif
namespace Craft
{
	using u8 = unsigned char;
	using u16 = unsigned short;
	using u32 = unsigned int;
	using u64 = unsigned long long;
	using i8 = signed char;
	using i16 = signed short;
	using i32 = signed int;
	using i64 = signed long long;
	using f32 = float;
	using f64 = double;
	using uSize = size_t;
	using UnkFunc = void*;
	template<size_t size>
	using UnkPad = std::array<u8, size>;
	using UnkIntegral = UnkPad<8>;
	using UnkData = void*;

	union PointerWrapper
	{
		UnkData pointer = nullptr;
		u64 value;

		template<typename T>
		PointerWrapper(T* ptr) : pointer(reinterpret_cast<UnkData>(ptr)) {}
		PointerWrapper() = default;
		PointerWrapper(u64 val) : value(val) {}

		template<typename T>
		operator T* () const { return reinterpret_cast<T*>(pointer); }
		operator u64() const { return value; }

		template<typename T>
		T* as() const { return reinterpret_cast<T*>(pointer); }
	};


}