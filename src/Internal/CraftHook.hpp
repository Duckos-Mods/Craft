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
        struct DataSync
        {
            UnkIntegral mArg1Value{};
            UnkData mArg1Pointer{};
            UnkIntegral mArg2Value{};
            UnkData mArg2Pointer{};
            UnkIntegral mArg3Value{};
            UnkData mArg3Pointer{};
            UnkIntegral mArg4Value{};
            UnkData mArg4Pointer{};
            DataSync() = default;
        };
        std::vector<UnkFunc>* mPreHooks{};
        std::vector<UnkFunc>* mPostHooks{};
        DataSync* mDataSync{};
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
    private:
        jmp64 createThunk(UnkFunc targetAddress);  
        void setupMemory(UnkFunc originalFunc);
        void installThunk();
        void generateASM(NeededHookInfo& hookInfo);
        PointerWrapper VariableLocationToDataSyncLocation(VariableLocations location, bool isBackup);

    public:
        ManagerHook() {}
        ManagerHook(ManagerHook&) = delete;
        ManagerHook& operator=(ManagerHook&) = delete; // Disallow copying of this class
        ManagerHook(ManagerHook&&) = default;
        ManagerHook& operator=(ManagerHook&&) = default;

        void CreateManagerHook(UnkFunc originalFunc, UnkFunc targetFunc, NeededHookInfo& hookInfo, HookType hookType, bool pauseThreads = true);
        

        void AddHook(UnkFunc hookFunc, HookType hookType, bool pauseThreads = true);


        TEST_ONLY(UnkFunc GetTrampoline() { return this->mTrampoline.mTrampoline; })
        

    };

    

}