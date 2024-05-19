#include "CraftHook.hpp"
#include <Internal/CraftThreads.hpp>
#include <zasm/zasm.hpp>
#include <Internal/CraftMemory.hpp>

namespace Craft
{
	using namespace zasm;
	using namespace zasm::x86;
	constexpr u32 cAllocSize = 4096;

	jmp64 ManagerHook::createThunk(UnkFunc targetAddress)
	{
		return jmp64(reinterpret_cast<u64>(targetAddress));
	}
	void ManagerHook::setupMemory(UnkFunc originalFunc)
	{
		u8* memory = (u8*)UNROLL_EXPECTED(OSAlloc(
			cAllocSize,
			{ true, true, true }));
		// "Failed to allocate memory for hooking function"

		OSErr err = OSProtect(
			originalFunc,
			14,
			{ true, true, true }
		);
		CRAFT_THROW_IF_NOT(err == OSErr::NONE, "Failed to protect memory for original function");
		constexpr uSize vecSize = sizeof(std::vector<UnkFunc>);
		constexpr uSize ptrSize = sizeof(UnkIntegral);
		this->mPreHooks = new(memory + cAllocSize - vecSize) std::vector<UnkFunc>;
		this->mPostHooks = new(memory + cAllocSize - vecSize*2) std::vector<UnkFunc>;
		this->mArg1 = new(memory + cAllocSize - vecSize*2+ptrSize) UnkIntegral;
		this->mArg2 = new(memory + cAllocSize - vecSize*2+ptrSize*2) UnkIntegral;
		this->mArg3 = new(memory + cAllocSize - vecSize*2+ptrSize*3) UnkIntegral;
		this->mArg4 = new(memory + cAllocSize - vecSize*2+ptrSize*4) UnkIntegral;
		// I am not sure if this is the best way to do this but it works

		this->mOriginalFunc.mOriginalFunc = originalFunc;
		this->mOriginalFunc.mSize = (u32)ASMUtil::GetMinBytesForNeededSize(originalFunc, 14);
		this->mOriginalFunc.mOriginalBytes = new u8[this->mOriginalFunc.mSize];
		memcpy(this->mOriginalFunc.mOriginalBytes, originalFunc, this->mOriginalFunc.mSize);

		this->mTrampoline.mSize = cAllocSize;
		this->mTrampoline.mTrampoline = memory;
	}
	void ManagerHook::installThunk()
	{
		jmp64 thunk = createThunk(this->mOriginalFunc.mOriginalFunc);
		memcpy(this->mTrampoline.mTrampoline, thunk.GetASM(), 14);
	}

	static Gp64 VariableLocationToGp64(VariableLocations location)
	{
		/*if (TO_UNDERLYING(location) >= TO_UNDERLYING(VariableLocations::RCX) && TO_UNDERLYING(location) <= TO_UNDERLYING(VariableLocations::R9))
			return Gp64(static_cast<Reg::Id>(TO_UNDERLYING(location) + ZYDIS_REGISTER_RAX));*/
		switch (location)
		{
			case VariableLocations::RCX:
				return rcx;
			case VariableLocations::RDX:
				return rdx;
			case VariableLocations::R8:
				return r8;
			case VariableLocations::R9:
				return r9;
			default:
				CRAFT_THROW("Invalid location for argument");
		}
	}

	static Xmm VariableLocationToXmm(VariableLocations location)
	{
		switch (location)
		{
			case VariableLocations::XMM0:
				return xmm0;
			case VariableLocations::XMM1:
				return xmm1;
			case VariableLocations::XMM2:
				return xmm2;
			case VariableLocations::XMM3:
				return xmm3;
			default:
				CRAFT_THROW("Invalid location for argument");
		}
	}

	void ManagerHook::generateASM(NeededHookInfo& hookInfo)
	{
		Program program(MachineMode::AMD64); 
		Assembler a(program);
		Label l = a.createLabel();
		auto& ALI = hookInfo.argumentLocationInfo;
		i32 stackSize = ALI.size() - 4;
		if (stackSize < 0)
			stackSize = 0;
		bool returnDataNeedsStack = (ALI.size() > 4) ? true : false;

		auto getStackIndex = [&](i16 idx) -> u16
			{
				static const i16 stackMax = ALI.size();
				return abs((idx - stackMax)) - 4; // I might have cooked here but idk lol
			};

		a.push(rbp); // push rbp // Save old base pointer
		a.mov(rbp, rsp); // mov rbp, rsp // Set new base pointer
		u16 stackModificationSize = 0;
		constexpr u16 stackOffsetPerArg = 16;

		// Handles the stack arguments
		for (i16 i = 4; i < ALI.size(); i++)
		{
			auto& arg = ALI[i];
			CRAFT_THROW_IF_NOT(arg.location == VariableLocations::Stack, "Invalid location for argument this far into the array");
			u16 flooredIndexOffset = getStackIndex(i - 4) * 8; // Offset for reading arguments from the stack
			if (arg.shouldTakeAddress)
			{
				a.mov(rax, qword_ptr(rbp, flooredIndexOffset)); // mov rax, qword ptr [rbp + flooredIndexOffset] // Load the argument
				a.push(rax); // push rax // Push the argument
				a.push(rsp); // push rsp // Push the stack pointer this can be used to get the address of the argument
			}
			else
			{
				DEBUG_ONLY(
					a.mov(rax, 0);
					a.push(rax);
				);

				a.mov(rax, qword_ptr(rbp, flooredIndexOffset)); // mov rax, qword ptr [rbp + flooredIndexOffset] // Load the argument
				RELEASE_ONLY(
					a.push(rax); // push rax // Push the argument
					// This double push is to make sure that we keep our ABI aligned to our standard
					// It is also less instructions then moving 0 into rax and then pushing it
				    // It is harder to debug but it is faster
					// This is only done in release mode
				);
				a.push(rax); // push rax // Push the argument

			}

			stackModificationSize += 16;
		}
		

		// Restore everything i hope
		a.mov(rsp, rbp); // mov rsp, rbp // Restore stack pointer
		a.pop(rbp); // pop rbp // Restore base pointer

		Serializer s{};
		auto res = s.serialize(program, (uint64_t)this->mTrampoline.mTrampoline);
		CRAFT_THROW_IF_NOT(res == Error::None, "Error in generating ASM");
		const auto* sect = s.getSectionInfo(0);
		const auto* data = s.getCode() + sect->offset;
		CRAFT_THROW_IF(sect->physicalSize > cAllocSize, "To much ASM generated");
		memcpy(this->mTrampoline.mTrampoline, data, sect->physicalSize);

	}
	void ManagerHook::CreateManagerHook(UnkFunc originalFunc, UnkFunc targetFunc, NeededHookInfo& hookInfo, HookType hookType, bool pauseThreads)
	{
		if (pauseThreads)
			AllOSThreads pauseThreads{};
		setupMemory(originalFunc);
		generateASM(hookInfo);

		installThunk(); // Do this last so if threads are not paused we hopefully dont jump while building the hook

	}
}
