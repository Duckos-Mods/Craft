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
#error "Unsupported platform"
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
#define UNROLL_EXPECTED_EX(name, exp)\
if(!exp)\
	return std::unexpected(exp.error());\
auto name = exp.value()
#endif


#ifndef STATIC_ASSERT
#define STATIC_ASSERT(condition, message) static_assert(condition, message)
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

	template<typename T>
	class TPointerWrapper
	{
	private:
		PointerWrapper m_pointer;
	public:
		TPointerWrapper(T* ptr) : m_pointer(ptr) {}
		TPointerWrapper() = default;
		TPointerWrapper(u64 val) : m_pointer(val) {}

		u64 value() const { return m_pointer.value; }

		operator T* () const { return m_pointer.as<T>(); }
		operator u64() const { return m_pointer; }
		T* operator->() const { return m_pointer.as<T>(); }
		T& operator*() const { return *m_pointer.as<T>(); }
		template<typename U>
		operator U* () const { return m_pointer.as<U>(); }
		template<typename U>
		U* as() const { return m_pointer.as<U>(); }
	};

	static inline UnkFunc NonStaticLocalFunctionToRealAddress(PointerWrapper pointer)
	{

		void* pFunction = pointer;

		char* ptr = reinterpret_cast<char*>(pFunction);
		ptr++;
		int32_t offset = *reinterpret_cast<int32_t*>(ptr);
		ptr--;
		uint64_t target = ((uint64_t)ptr + offset);
		while (target % 16 != 0) {
			target++;
		}
		return (void*)target;
	}
}