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
    namespace details
    {
        struct InternalNeededHookInfo
        {
            NeededHookInfo& mNeededHookInfo; 
            u8 mStackArgCount{};
            u8 mRegArgCount{};
            bool mReturnDataNeedsStack : 1 {0};


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
        std::vector<UnkFunc>* mPreHooks{};
        std::vector<UnkFunc>* mPostHooks{};
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
        constexpr static std::array<u16, 4> mRegisterOffsets = { 16, 24, 32, 40 };
    private:
        jmp64 createThunk(UnkFunc targetAddress);  
        void setupMemory(UnkFunc originalFunc);
        void installThunk();
        u16 getStackOffset(i16 stackMax, i16 index) {return abs(index - stackMax) - 4;} // Ok so. I might have cooked? I am not sure but I think this is right

        void generateASM(NeededHookInfo& hookInfo);

        void createRegisterBackupASM(details::InternalNeededHookInfo& hookInfo, zasm::x86::Assembler& a);
        void createStackBackupASM(details::InternalNeededHookInfo& hookInfo, zasm::x86::Assembler& a);
        
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
        TEST_ONLY(UnkFunc GetTrampoline() { return this->mTrampoline.mTrampoline; })
        

    };

    

}