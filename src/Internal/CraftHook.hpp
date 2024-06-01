#pragma once
#include <cstdint>
#include <array>
#include <span>
#include <vector>
#include <Internal/CraftASM.hpp>
#include <Internal/CraftRegisterCalculations.hpp>
#include <zasm/zasm.hpp>
#include <functional>

namespace Craft
{
    constexpr uSize stackBackupCount = CRAFT_MAX_EXPECTED_STACK_ARGS * 2;
    constexpr uSize stackBackupSize = sizeof(std::array<UnkData, stackBackupCount>);
    namespace details
    {
        struct InternalNeededHookInfo
        {
            NeededHookInfo& mNeededHookInfo; 
            u8 mStackArgCount{};
            u8 mRegArgCount{};
            bool mReturnDataNeedsStack : 1 {0};
            bool mNonReturnNeeds8Pad : 1 {0};
            bool mReturnNeeds8Pad : 1 {0};
            u32 mStackOffset{};
        };

        union alignas(16) XMM0Backup
        {
            std::array<f32, 4> mFloats;
            std::array<f64, 2> mDoubles;
            std::array<u8, 16> mBytes;
            std::array<u64, 2> mInts;
            std::array<u32, 4> mInts32;
            std::array<u16, 8> mInts16;
            std::array<i8, 16> mSignedInts8;
            std::array<i16, 8> mSignedInts16;
            std::array<i32, 4> mSignedInts32;
            std::array<i64, 2> mSignedInts64;
        };


    }

    enum class HookType : u8 // No need for more than 255 types
    {
        PreHook,
        PostHook,
        ReplaceHook,

        Inline = 0x20, // Experimental Will Not Work Most Likely
    };

    class ManagerHook
    {
    private:
        TPointerWrapper<std::vector<UnkFunc>> mPreHooks{};
        TPointerWrapper<std::vector<UnkFunc>> mPostHooks{};
        TPointerWrapper<std::array<UnkData, stackBackupCount>> mStackBackups{};
        TPointerWrapper<details::XMM0Backup> mXMM0Backup{};
        TPointerWrapper<u64> mStackOffset{};
        UnkFunc ReplaceHook{};
        struct OriginalFunc
        {
            u8* mOriginalBytes{};
            uSize mSize{};
            UnkFunc mOriginalFunc{};
        };
        struct Trampoline
        {
			u8* mTrampoline{};
			uSize mSize{};
		};
        OriginalFunc mOriginalFunc{};
		Trampoline mTrampoline{};
        constexpr static std::array<u16, 4> mRegisterOffsets = { 8, 16, 24, 32 };
    private:
        jmp64 createThunk(UnkFunc targetAddress);  
        void setupMemory(UnkFunc originalFunc);
        void installThunk();
        u16 getStackOffset(i16 index) {return index - 3;} // Ok so. I might have cooked? I am not sure but I think this is right

        void generateASM(NeededHookInfo& hookInfo);

        void createRegisterBackupASM(details::InternalNeededHookInfo& hookInfo, zasm::x86::Assembler& a);
        void createStackBackupASM(details::InternalNeededHookInfo& hookInfo, zasm::x86::Assembler& a);
        void createPreHookCallingASM(details::InternalNeededHookInfo& hookInfo, zasm::x86::Assembler& a);
        void createFunctionCallASM(details::InternalNeededHookInfo& hookInfo, zasm::x86::Assembler& a, bool appendRax = false);

        void createReadOffsetASM(details::InternalNeededHookInfo& hookInfo, zasm::x86::Assembler& a);

        void createFunctionHeadASM(details::InternalNeededHookInfo& hookInfo, zasm::x86::Assembler& a);
        void createFunctionTailASM(details::InternalNeededHookInfo& hookInfo, zasm::x86::Assembler& a);



    public:
        ManagerHook() {}
        ManagerHook(ManagerHook&) = delete;
        ManagerHook& operator=(ManagerHook&) = delete; // Disallow copying of this class
        ManagerHook(ManagerHook&&) = default;
        ManagerHook& operator=(ManagerHook&&) = default;

        void CreateManagerHook(UnkFunc originalFunc, UnkFunc targetFunc, NeededHookInfo& hookInfo, HookType hookType, bool pauseThreads = true);
        

        void AddHook(UnkFunc hookFunc, HookType hookType); 
        void RemoveHook(UnkFunc targetFunc, HookType hookType); // This force pauses all threads to prevent any issues


        void AddNewFuncToHooksWithNoProcess(UnkFunc ptr) 
        { 
            this->mPreHooks->push_back(ptr);
        }
        UnkFunc GetTrampoline() { return this->mTrampoline.mTrampoline; }
        

    };

    

}