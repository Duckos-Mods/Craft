#include "CraftHook.hpp"
#include <Internal/CraftThreads.hpp>
#include <zasm/zasm.hpp>
#include <Internal/CraftMemory.hpp>

namespace Craft
{
	using namespace zasm;
	using namespace zasm::x86;

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
		constexpr uSize dataSyncSize = sizeof(DataSync);

		this->mPreHooks = new(memory + cAllocSize - vecSize) std::vector<UnkFunc>;
		this->mPostHooks = new(memory + cAllocSize - vecSize*2) std::vector<UnkFunc>;

		this->mDataSync = new(memory + cAllocSize - vecSize*2 - dataSyncSize) DataSync;

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
		Label l = a.createLabel("MainFunction");
		a.bind(l);
		auto& ALI = hookInfo.argumentLocationInfo;
		i32 stackSize = (i32)ALI.size() - 4;
		if (stackSize < 0)
			stackSize = 0;
		bool returnDataNeedsStack = (ALI.size() > 4) ? true : false;

		auto getStackIndex = [&](i16 idx) -> u16
			{
				static const i16 stackMax = (i16)ALI.size();
				return abs((idx - stackMax)) - 4; // I might have cooked here but idk lol
			};

		a.push(rbp); // Save old base pointer
		a.mov(rbp, rsp); // Set new base pointer

		u16 stackModificationSize = 0;
		constexpr u16 stackOffsetPerArg = 16;
		u64 startRBPOffset = 0;

		// Handles the stack arguments
		for (i16 i = 4; i < ALI.size(); i++)
		{
			auto& arg = ALI[i];
			CRAFT_THROW_IF_NOT(arg.location == VariableLocations::Stack, "Invalid location for argument this far into the array");
			u16 flooredIndexOffset = (getStackIndex(i - 4) * 8) + STACK_MAGIC_NUMBER; // Offset for reading arguments from the stack
			if (startRBPOffset == 0) [[unlikely]]
				startRBPOffset = flooredIndexOffset;

			DEBUG_ONLY(
				a.nop(); // Breaks Up Code For easier debugging
			);
			if (arg.shouldTakeAddress)
			{
				a.mov(rax, qword_ptr(rbp, flooredIndexOffset)); // Load the argument
				a.push(rax); // Push the argument
				a.push(rsp); // Push stack pointer for address
			}
			else
			{
				DEBUG_ONLY(
					a.mov(rax, 0);
					a.push(rax);
				);

				a.mov(rax, qword_ptr(rbp, flooredIndexOffset)); // mov rax, qword ptr [rbp + flooredIndexOffset] // Load the argument
				RELEASE_ONLY(
					a.push(rax); / Push the argument
					// This double push is to make sure that we keep our ABI aligned to our standard
					// It is also less instructions then moving 0 into rax and then pushing it
				    // It is harder to debug but it is faster
				);
				a.push(rax); // Push the argument
			}
			stackModificationSize += 16;
		}
		u16 regLoopCount = (ALI.size() > 4) ? 4 : (u16)ALI.size();
		// Handles the register arguments
		// This might suck due to float registers existing :sob:
		a.push(r10);
		for (i16 i = 0; i < regLoopCount; i++)
		{
			auto& arg = ALI[i];
			if (arg.location == VariableLocations::Stack)
				CRAFT_THROW("Invalid location for argument this soon in the array!");

			DEBUG_ONLY(
				a.nop(); // Breaks Up Code For easier debugging
			);
			if (arg.shouldTakeAddress)
			{
				if (arg.location >= VariableLocations::RCX && arg.location <= VariableLocations::R9)
					a.mov(rax, VariableLocationToGp64(arg.location)); // Load the argument
				else
				{
					a.sub(rsp, 8);
					a.movsd(qword_ptr(rsp), VariableLocationToXmm(arg.location));
					a.pop(rax);
					// This is a bit of a hack but it works
					// Performs a push of a float to the stack then pops into a none float register for easier backup
				}
				PointerWrapper data = VariableLocationToDataSyncLocation(arg.location, true);
				a.mov(r10, data.value);
				a.mov(qword_ptr(r10), rax);
				// This loads the argument into the data sync location
				a.mov(qword_ptr(r10, 8), r10); // Writes the address of the argument.
			}
			else
			{
				if (arg.location < VariableLocations::RCX || arg.location > VariableLocations::R9)
					CRAFT_THROW("A Non Main Register Has Been Passed By Value");

				// We know the register is now RCX, RDX, R8, or R9
				PointerWrapper data = VariableLocationToDataSyncLocation(arg.location, false);
				a.mov(r10, data.value);
				a.mov(rax, VariableLocationToGp64(arg.location));
				a.mov(qword_ptr(r10), rax);
			}

		}
		a.pop(r10);
		// Setup Registerts
		for (i16 i = 0; i < regLoopCount; i++)
		{
			auto& arg = ALI[i];
			DEBUG_ONLY(
				a.nop(); // Breaks Up Code For easier debugging
			);
			auto loc = TO_ENUM(VariableLocations, TO_UNDERLYING(VariableLocations::RCX) + i);

			PointerWrapper data = VariableLocationToDataSyncLocation(loc, false); // Obtains a pointer to the register data
			a.mov(VariableLocationToGp64(loc), data.value);
		}
		a.push(r10);
		a.mov(r10, rsp);
		a.add(r10, 10);
		a.push(r15);
		a.push(r14);
		// Call the pre hooks
		DEBUG_ONLY(
			a.nop(); // Breaks Up Code For easier debugging
		);
		a.mov(r15, (uint64_t)this->mPreHooks); // The array of funcs to call
		a.mov(r14, qword_ptr(r15, 8)); // Last Element In The Array Pointer
		a.mov(r15, qword_ptr(r15)); // Start Pointer

		TEST_ONLY(a.int3());
		Label ll = a.createLabel("preHooksLoopStart");
		a.bind(ll);
		DEBUG_ONLY(
			a.nop(); // Breaks Up Code For easier debugging
		);
		a.add(r15, 8); // Go To Next Function Pointer
		a.cmp(r15, r14); // Make sure R15 and R14 are not the same if they are we are at th end of the calls 
		a.jnz(ll); // If r15 and r14 are not the same jump back to do another call
		DEBUG_ONLY(
			a.nop(); // Breaks Up Code For easier debugging
		);
		a.bind(l);
		a.pop(r14);
		a.pop(r15);
		a.pop(r10);

		// Restore everything i hope
		a.mov(rsp, rbp); // mov rsp, rbp // Restore stack pointer
		a.pop(rbp); // pop rbp // Restore base pointer
		a.ret(); // ret // Return to the original function

		Serializer s{};
		auto res = s.serialize(program, (uint64_t)this->mTrampoline.mTrampoline);
		CRAFT_THROW_IF_NOT(res == Error::None, "Error in generating ASM");
		const auto* sect = s.getSectionInfo(0);
		const auto* data = s.getCode() + sect->offset;
		CRAFT_THROW_IF(sect->physicalSize > cAllocSize, "To much ASM generated");
		memcpy(this->mTrampoline.mTrampoline, data, sect->physicalSize);

	}
	PointerWrapper ManagerHook::VariableLocationToDataSyncLocation(VariableLocations location, bool isBackup)
	{
		switch (location)
		{
		case Craft::VariableLocations::RCX:
		case Craft::VariableLocations::XMM0:
			if (isBackup)
				return &this->mDataSync->mArg1Value;
			else
				return &this->mDataSync->mArg1Pointer;
			break;
		case Craft::VariableLocations::RDX:
		case Craft::VariableLocations::XMM1:
			if (isBackup)
				return &this->mDataSync->mArg2Value;
			else
				return &this->mDataSync->mArg2Pointer;
			break;
		case Craft::VariableLocations::R8:
		case Craft::VariableLocations::XMM2:
			if (isBackup)
				return &this->mDataSync->mArg3Value;
			else
				return &this->mDataSync->mArg3Pointer;
			break;
		case Craft::VariableLocations::R9:
		case Craft::VariableLocations::XMM3:
			if (isBackup)
				return &this->mDataSync->mArg4Value;
			else
				return &this->mDataSync->mArg4Pointer;
			break;
		default:
			CRAFT_THROW("Invalid location for argument");
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
}


