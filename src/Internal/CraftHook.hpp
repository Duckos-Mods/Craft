#pragma once
#include <cstdint>
#include <array>
#include <span>
#include <vector>
#include <Internal/CraftASM.hpp>
#include <Internal/CraftRegisterCalculations.hpp>

namespace Craft
{

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
        UnkIntegral* mArg1{};
        UnkIntegral* mArg2{};
        UnkIntegral* mArg3{};
        UnkIntegral* mArg4{}; // Storage for register based arguments that require pointers to them
        UnkFunc ReplaceHook{};
        struct OriginalFunc
        {
            u8* mOriginalBytes{};
            u32 mSize{};
            UnkFunc mOriginalFunc{};
        };
        struct Trampoline
        {
			u8* mTrampoline{};
			u32 mSize{};
		};
        OriginalFunc mOriginalFunc{};
		Trampoline mTrampoline{};
    private:
        jmp64 createThunk(UnkFunc targetAddress);  
        void setupMemory(UnkFunc originalFunc);
        void installThunk();
        void generateASM(NeededHookInfo& hookInfo);

    public:
        ManagerHook() {}

        void CreateManagerHook(UnkFunc originalFunc, UnkFunc targetFunc, NeededHookInfo& hookInfo, HookType hookType, bool pauseThreads = true);
        

        void AddHook(UnkFunc hookFunc, HookType hookType, bool pauseThreads = true);



        

    };

    

}