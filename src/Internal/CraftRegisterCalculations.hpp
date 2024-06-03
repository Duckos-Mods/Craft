#pragma once
#include <Internal/CraftUtils.hpp>
#include <type_traits>
#include <vector>

namespace Craft
{
	enum class VariableLocations : u8
	{
		Stack = 0, 
		RCX,
		RDX,
		R8,
		R9,
		XMM0,
		XMM1,
		XMM2,
		XMM3,
	};

	struct TypeInformation
	{
		bool isFloat = false;
		bool isPointer = false;
		u32 size = 0; // No class should be above 4GB in size if it is then you are doing something wrong lol
	};

	struct NeededTypeHookInfo
	{
		VariableLocations location = VariableLocations::Stack;
		bool shouldTakeAddress = false;
		u32 size = 0;
	};
	struct NeededHookInfo
	{
		std::vector<NeededTypeHookInfo> argumentLocationInfo{};
		bool ShouldPassReturnValue = false;
		bool ShouldUseWideRegisters = false;
	};

	template<typename T>
	constexpr TypeInformation GetTypeInformation()
	{
		TypeInformation info;

		if constexpr (std::is_reference_v<T>)
		{
			info.size = 8;
			info.isPointer = true;
			info.isFloat = false;
			return info;
		}
		info.size = sizeof(T);
		info.isFloat = std::is_floating_point_v<T>;
		info.isPointer = std::is_pointer_v<T>;
		return info;
	}

	template<>
	constexpr TypeInformation GetTypeInformation<void>()
	{
		TypeInformation info;
		info.size = 0;
		info.isFloat = false;
		info.isPointer = false;
		return info;
	}

	namespace InternalCalculations
	{
		inline bool GetPassReturn(TypeInformation returnTypeInfo){return returnTypeInfo.size > 0;} // If void then return false else 
		inline bool GetShouldUseWideRgister(TypeInformation returnTypeInfo) { return returnTypeInfo.size > 8 && returnTypeInfo.size <= 16; }

		NeededTypeHookInfo ComputeTypeHookInfo(TypeInformation typeInfo, u16 index);

	}

	NeededHookInfo GetNeededHookInfo(TypeInformation returnTypeInfo, const std::vector<TypeInformation>& functionArguments);



}