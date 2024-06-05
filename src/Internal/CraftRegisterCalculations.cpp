#include "CraftRegisterCalculations.hpp"

namespace Craft
{
	NeededHookInfo GetNeededHookInfo(TypeInformation returnTypeInfo, const std::vector<TypeInformation>& functionArguments)
	{
		NeededHookInfo inf;
		inf.argumentLocationInfo.reserve(functionArguments.size()); 
		inf.ShouldPassReturnValue = InternalCalculations::GetPassReturn(returnTypeInfo);
		inf.ShouldUseWideRegisters = InternalCalculations::GetShouldUseWideRgister(returnTypeInfo);
		inf.ReturnArgSize = returnTypeInfo.size;
		u16 loopCount = ((u16)functionArguments.size() > 4) ? 4 : (u16)functionArguments.size();

		for (u16 i = 0; i < loopCount; i++)
			inf.argumentLocationInfo.push_back(InternalCalculations::ComputeTypeHookInfo(functionArguments[i], i));
		if (loopCount < 4)
			return inf; 


		u16 remainingArgs = (u16)functionArguments.size() - 4;
		u16 startingIndex = 4;

		for (u16 i = 4; i < functionArguments.size(); i++)
			inf.argumentLocationInfo.push_back(InternalCalculations::ComputeTypeHookInfo(functionArguments[i], i));

		return inf;
	}

	// Index can be u16 due to no function having more than 65535 args. I could prolly do a u8 lol
	NeededTypeHookInfo InternalCalculations::ComputeTypeHookInfo(TypeInformation typeInfo, u16 index) 
	{
		NeededTypeHookInfo inf;
		inf.size = typeInfo.size;
		if (typeInfo.size > 8)
			inf.shouldTakeAddress = true; 
		else
		{
			if (typeInfo.isPointer)
				inf.shouldTakeAddress = false;
			else
				inf.shouldTakeAddress = true;

		}
			
		// If its smaller than 8 bytes then its passed as value so we need to tell all functions reading it that its a pointer so they can modify it 
		if (typeInfo.isFloat)
		{
			if (index < 4)
				inf.location = TO_ENUM(VariableLocations, TO_UNDERLYING(VariableLocations::XMM0) + index);
			else
				inf.location = VariableLocations::Stack;
		}
		else
		{
			if (index < 4)
				inf.location = TO_ENUM(VariableLocations, TO_UNDERLYING(VariableLocations::RCX) + index);
			else
				inf.location = VariableLocations::Stack;
		}

		return inf;
	}
}

