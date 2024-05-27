#include "CraftHook.hpp"
#include <Internal/CraftThreads.hpp>
#include <zasm/zasm.hpp>
#include <Internal/CraftMemory.hpp>

namespace Craft
{
	using namespace zasm;
	using namespace zasm::x86;


	constexpr uSize vecSize = sizeof(std::vector<UnkFunc>);

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

		this->mPreHooks = new(memory + cAllocSize - vecSize) std::vector<UnkFunc>;
		this->mPostHooks = new(memory + cAllocSize - vecSize*2) std::vector<UnkFunc>;


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
		memcpy(this->mOriginalFunc.mOriginalFunc, thunk.GetASM(), 14);
	}

	static Gp64 VariableLocationToGp64(VariableLocations location)
	{
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
		Label l = a.createLabel("MainFunction");
		a.bind(l);
		auto& ALI = hookInfo.argumentLocationInfo;
		i32 stackSize = (i32)ALI.size() - 4;
		if (stackSize < 0)
			stackSize = 0;
		bool returnDataNeedsStack = (ALI.size() >= 4) ? true : false;

		auto getStackIndex = [&](i16 idx) -> u16
			{
				static const i16 stackMax = (i16)ALI.size();
				return abs((idx - stackMax)) - 4; // I might have cooked here but idk lol
			};

		details::InternalNeededHookInfo hookInfoInternal{ hookInfo, (u8)stackSize, (u8)ALI.size() - stackSize, returnDataNeedsStack };

		createFunctionHeadASM(hookInfoInternal, a);
		createRegisterBackupASM(hookInfoInternal, a);
		createStackBackupASM(hookInfoInternal, a); // This comes after so we have 5 registers to use for handling stack backup

		createFunctionTailASM(hookInfoInternal, a);
		

		Serializer s{};
		auto res = s.serialize(program, (uint64_t)this->mTrampoline.mTrampoline);
		CRAFT_THROW_IF_NOT(res == Error::None, "Error in generating ASM");
		const auto* sect = s.getSectionInfo(0);
		const auto* data = s.getCode() + sect->offset;
		CRAFT_THROW_IF(sect->physicalSize > cAllocSize, "To much ASM generated");
		memcpy(this->mTrampoline.mTrampoline, data, sect->physicalSize);

	}
	void ManagerHook::createFunctionHeadASM(details::InternalNeededHookInfo& hookInfo, zasm::x86::Assembler& a)
	{
		// Performs backup of registers required to be backed up
		a.push(rbp);
		a.mov(rbp, rsp);
	}
	void ManagerHook::createFunctionTailASM(details::InternalNeededHookInfo& hookInfo, zasm::x86::Assembler& a)
	{
		// Restores registers that were backed up
		a.mov(rsp, rbp);
		a.pop(rbp);
		a.ret();
	}
	void ManagerHook::createStackBackupASM(details::InternalNeededHookInfo& hookInfo, zasm::x86::Assembler& a)
	{
		auto& nhi = hookInfo.mNeededHookInfo;
		auto& ALI = nhi.argumentLocationInfo;
		auto stackArgCount = hookInfo.mStackArgCount;
		auto regArgCount = hookInfo.mRegArgCount;
		if (stackArgCount == 0)
			return; // Just return if we dont have any stack arguments also means no alignment is needed later in the function
		uSize startRBPOffset = 0;
		constexpr u16 stackOffsetPerArg = 16;
		for (i16 i = 4; i < ALI.size(); i++)
		{
			auto& arg = ALI[i];
			CRAFT_THROW_IF_NOT(arg.location == VariableLocations::Stack, "Invalid location for stack argument");
			u16 flooredIdx = (this->getStackOffset(ALI.size(), i - 4) * 8) + STACK_MAGIC_NUMBER;
			DEBUG_ONLY(a.nop());
			if (startRBPOffset == 0) [[unlikely]]
				startRBPOffset = flooredIdx;
			if (arg.shouldTakeAddress)
			{
				a.push(qword_ptr(rbp, flooredIdx));
				a.push(rsp);
			}
			else
			{
				// Only in debug due to the fact its more instructions
				// Just makes debugging easier :3
				DEBUG_ONLY(
					a.mov(rax, 0);
					a.push(rax);
				);
				a.mov(rax, qword_ptr(rbp, flooredIdx));
				a.push(rax);
				RELEASE_ONLY(
					a.push(rax);
				);
				/*
				* This does work but its actually slower than using a mov instruction due to CPU cache
				RELEASE_ONLY(
					a.push(qword_ptr(rbp, flooredIdx));
				);
				a.push(qword_ptr(rbp, flooredIdx));*/
			}
			
		}

	}
	void ManagerHook::createRegisterBackupASM(details::InternalNeededHookInfo& hookInfo, zasm::x86::Assembler& a)
	{
		auto& nhi = hookInfo.mNeededHookInfo;
		auto& ALI = nhi.argumentLocationInfo;
		auto regArgCount = hookInfo.mRegArgCount;
		u16 rbpOffset = 16; // This skips the return address and the 8 byte alignmet that is pushed by the compiler

		for (u16 i = 0; i < regArgCount; i++)
		{
			DEBUG_ONLY(a.nop());
			auto& ali = ALI[i];
			if (ali.location == VariableLocations::Stack) [[unlikely]]
				CRAFT_THROW("Invalid location for argument this early in the array");
			if (ali.location >= VariableLocations::RCX && ali.location <= VariableLocations::R9)
			{
				auto reg = VariableLocationToGp64(ali.location);
				a.mov(qword_ptr(rbp, rbpOffset), reg);
			}
			else
			{
				auto reg = VariableLocationToXmm(ali.location);
				// Waiiiiiiit this is a function????????
				a.movsd(qword_ptr(rbp, rbpOffset), reg);
			}
			rbpOffset += 8;
		}

	}
	void ManagerHook::CreateManagerHook(UnkFunc originalFunc, UnkFunc targetFunc, NeededHookInfo& hookInfo, HookType hookType, bool pauseThreads)
	{
		if (pauseThreads)
			AllOSThreads pauseThreads{}; // This auto releases when it goes out of scope so we dont have to worry about it
		setupMemory(originalFunc);
		generateASM(hookInfo);

		// installThunk(); // Do this last so if threads are not paused we hopefully dont jump while building the hook

	}
	void ManagerHook::RemoveHook(UnkFunc targetFunc, HookType hookType)
	{
	}
	void ManagerHook::AddHook(UnkFunc hookFunc, HookType hookType)
	{
	}
}


