#include "CraftHook.hpp"
#include <Internal/CraftThreads.hpp>
#include <zasm/zasm.hpp>
#include <Internal/CraftMemory.hpp>

namespace Craft
{
	using namespace zasm;
	using namespace zasm::x86;


	constexpr uSize vecSize = sizeof(std::vector<UnkFunc>);
	constexpr uSize xmmSize = sizeof(details::XMM0Backup);

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
		this->mStackBackups = new(memory + cAllocSize - vecSize - stackBackupSize) std::array<UnkData, stackBackupCount>();
		DEBUG_ONLY(
			memset(this->mStackBackups.as<void>(), 0, stackBackupSize);
		);
		this->mXMM0Backup = new(memory + cAllocSize - vecSize - stackBackupSize - xmmSize) details::XMM0Backup;
		this->mStackOffset = new(memory + cAllocSize - vecSize - stackBackupSize - xmmSize - sizeof(u64)) u64(0xDEADC0DE);

		DEBUG_ONLY(
			PointerWrapper xmm0Wrapper{ this->mXMM0Backup };
		auto alignment = xmm0Wrapper.value % alignof(decltype(*this->mXMM0Backup));
		if (alignment != 0)
			CRAFT_THROW("XMM0Backup is not aligned correctly");
		);
		

		this->mPreHooks->reserve(16);
		this->mPostHooks->reserve(16);


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

		details::InternalNeededHookInfo hookInfoInternal{ hookInfo, (u8)stackSize, u8((u8)ALI.size() - (u8)stackSize), returnDataNeedsStack };

		createFunctionHeadASM(hookInfoInternal, a);
		createRegisterBackupASM(hookInfoInternal, a);
		createReadOffsetASM(hookInfoInternal, a);
		createStackBackupASM(hookInfoInternal, a); // This comes after so we have 5 registers to use for handling stack backup
		createPreHookCallingASM(hookInfoInternal, a);
		a.bind(l);

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
		a.mov(rax, rsp);
		a.and_(rax, 31);
		// If rax is 16 then we need to align the stack
		Label l = a.createLabel("StackAlignRax8");
		Label ll = a.createLabel("StackAlignRax0");
		a.cmp(rax, 16);
		a.jz(l);
		a.jnz(ll);
		a.bind(l);
		a.mov(rax, (u8)-8);
		a.bind(ll);
		a.xor_(rax, rax);
		a.push(r9);
		a.mov(r9, this->mStackOffset.value());
		a.mov(qword_ptr(r9), rax);
		a.pop(r9);

		TEST_ONLY(a.int3());
		a.push(rbp);
		hookInfo.mStackOffset += 8; // just the ammount of data pushed so far
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
		u32 startRBPOffset = 0;
		constexpr u16 stackOffsetPerArg = 16;
		a.mov(rax, this->mStackBackups.value());
		a.mov(r9, 0xFFFFFFFF);
		for (i16 i = 4; i < ALI.size(); i++)
		{
			DEBUG_ONLY(a.nop());
			auto& arg = ALI[i];
			CRAFT_THROW_IF_NOT(arg.location == VariableLocations::Stack, "Invalid location for stack argument");
			u16 flooredIdx = this->getStackOffset(i) * 8 + STACK_MAGIC_NUMBER;
			DEBUG_ONLY(a.nop());
			
			TEST_ONLY(a.int3());

			if (arg.shouldTakeAddress)
			{
				a.mov(rcx, qword_ptr(rbp, rbx, 1, flooredIdx));
				if (arg.size == 4)
					a.and_(rcx, r9);
				a.mov(qword_ptr(rax), rcx);
				a.mov(qword_ptr(rax, 8), rax);
			}
			else
			{
				// Only in debug due to the fact its more instructions
				// Just makes debugging easier :3
				TEST_ONLY(a.int3());
				DEBUG_ONLY(a.mov(rcx, STACK_MAGIC_DEBUG_NUMBER));
				a.mov(qword_ptr(rax), rcx);
				a.mov(rcx, qword_ptr(rbp, rbx, 1, flooredIdx));
				a.mov(qword_ptr(rax, 8), rcx);
			}		
			a.add(rax, 16);
		}


	}
	void ManagerHook::createRegisterBackupASM(details::InternalNeededHookInfo& hookInfo, zasm::x86::Assembler& a)
	{
		auto& nhi = hookInfo.mNeededHookInfo;
		auto& ALI = nhi.argumentLocationInfo;
		auto regArgCount = hookInfo.mRegArgCount;
		u16 rbpOffset = 16; // This skips the return address

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

	void ManagerHook::createPreHookCallingASM(details::InternalNeededHookInfo& hookInfo, zasm::x86::Assembler& a)
	{
		DEBUG_ONLY(a.nop());
		a.push(r15);
		a.push(r14);
		hookInfo.mStackOffset += 16; // just the ammount of data pushed so far
		constexpr u16 stackOffset = 16; // This is the offset from the stack pointer to the start of the stack arguments
		a.mov(r14, (u64)this->mPreHooks);
		a.mov(r15, qword_ptr(r14, 16));
		a.mov(r14, qword_ptr(r14, 8));
		//a.int3();

		Label l = a.createLabel("PreHookLoop");
		Label ll = a.createLabel("PreHookEnd");
		a.bind(l); // This isnt actual assembly its just semantics for zasm
		a.cmp(r14, r15);
		a.jz(ll); // If we are at the end of the vector jump to the end
		createFunctionCallASM(hookInfo, a);
		a.call(qword_ptr(r14)); // This is the actual call
		a.add(rsp, (hookInfo.mStackOffset + 8) + hookInfo.mStackArgCount * 8);
		a.add(r14, 8); // Move to the next function in the vector
		a.jmp(l); // Jump back to the start of the loop
		a.bind(ll);

		a.pop(r14);
		a.pop(r15);
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
	void ManagerHook::createReadOffsetASM(details::InternalNeededHookInfo& hookInfo, zasm::x86::Assembler& a)
	{
		a.push(rax);
		a.mov(rax, this->mStackOffset.value());
		a.mov(rbx, qword_ptr(rax));
		a.pop(rax);

		// This loads the offset into rbx
	}
	void ManagerHook::AddHook(UnkFunc hookFunc, HookType hookType)
	{
	}
	void ManagerHook::createFunctionCallASM(details::InternalNeededHookInfo& hookInfo, zasm::x86::Assembler& a, bool appendRax)
	{
		if (appendRax)
		{
			a.mov(rcx, this->mXMM0Backup.value());
			if (hookInfo.mNeededHookInfo.ShouldUseWideRegisters)
				// This means the return data is in xmm0
				a.movdqa(xmmword_ptr(rcx), xmm0);
			else
				// This means the return data is in rax
				a.mov(qword_ptr(rcx), rax);
		}
		u16 currentAlignmentOffset = hookInfo.mStackOffset;

		u16 tempAlignmentOffset = currentAlignmentOffset + hookInfo.mStackArgCount * 8;
		if (tempAlignmentOffset % 16 == 0)
		{
			// We need to align the stack
			a.sub(rsp, 8);
			currentAlignmentOffset += 8;
			// This forces us to push so we can use the same alignment code later
		}
		u16 addedToStack = 0;
		auto& ALI = hookInfo.mNeededHookInfo.argumentLocationInfo;
		auto stackArgCount = hookInfo.mStackArgCount;
		if (stackArgCount != 0)
			a.mov(rax, this->mStackBackups.value() + 8);
		for (u16 i = 0; i < stackArgCount; i++)
		{
			DEBUG_ONLY(a.nop());
			a.mov(rcx, qword_ptr(rax));
			a.push(rcx);
			a.add(rax, 16);
			addedToStack += 8;
		}
		TEST_ONLY(a.int3());

		DEBUG_ONLY(
			if (addedToStack != hookInfo.mStackArgCount * 8)
				CRAFT_THROW("Stack arguments not added correctly");
		);

		a.sub(rsp, 32); // Allocate shadow space and 8 padding
		addedToStack += 32;
		auto regArgCount = hookInfo.mRegArgCount;
		u16 rbpOffset = 16; // This skips the return address
		for (u16 i = 0; i < regArgCount; i++)
		{
			if (ALI[i].location == VariableLocations::Stack) [[unlikely]]
				CRAFT_THROW("Invalid location for argument this early in the array");
			auto reg = VariableLocationToGp64(TO_ENUM(VariableLocations, i+1));
			if (ALI[i].shouldTakeAddress)
				a.lea(reg, qword_ptr(rbp, rbpOffset));
			else
				a.mov(reg, qword_ptr(rbp, rbpOffset));
			rbpOffset += 8;
		}
	}
}


