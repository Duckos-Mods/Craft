#pragma once
#include <cstdint>
#include <array>
#include <span>
#include <vector>
#include <Internal/CraftASM.hpp>
#include <Internal/CraftRegisterCalculations.hpp>
#include <functional>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/raw_ostream.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Driver/Options.h>
#include <clang/Driver/Driver.h>
#include <clang/Driver/Compilation.h>
#include <clang/Driver/ToolChain.h>
#include <clang/Driver/Types.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Target/TargetMachine.h>
#include <Internal/CraftContext.hpp>
#include <variant>
#include <expected>

namespace Craft
{

    enum class HookType : u8 // No need for more than 255 types
    {
        PreHook,
        PostHook,
        ReplaceHook,

        Inline = 0x20, // Experimental Will Not Work Most Likely
    };

    enum class HookError : u8
    {
        DecideHookAlreadySet,
        ReplaceHookAlreadySet,
        AllocationError
    };

    namespace Internal
    {
        struct InternalHookInfo
		{
            InternalHookInfo(NeededHookInfo& hookInfo) : mHookInfo(hookInfo), mCompilerInstance() {}
            NeededHookInfo& mHookInfo;
            CraftContext* mContext = nullptr;
            clang::CompilerInstance mCompilerInstance;
            std::string mSource;
		};

        //using JmpInterface = std::variant<jmp64, jmp32>;
        class alignas(8) JmpInterface : private std::variant<jmp64, jmp32>
        {
        public:
            JmpInterface() : std::variant<jmp64, jmp32>(jmp32(0)) {}
            JmpInterface(jmp64 jmp) : std::variant<jmp64, jmp32>(jmp) {}
            JmpInterface(jmp32 jmp) : std::variant<jmp64, jmp32>(jmp) {}
            template<typename T> requires std::is_same_v<T, jmp64> || std::is_same_v<T, jmp32>
            JmpInterface(T jmp) : std::variant<jmp64, jmp32>(jmp) {}
            bool Is64() const { return std::holds_alternative<jmp64>(*this); }
            bool Is32() const { return std::holds_alternative<jmp32>(*this); }
            uSize GetHookSize() const { return std::holds_alternative<jmp64>(*this) ? 14 : 5; }
            static uSize Get32Size() { return 5; }
            static uSize Get64Size() { return 14; }
            u8* GetASM() { return std::holds_alternative<jmp64>(*this) ? std::get<jmp64>(*this).GetASM() : std::get<jmp32>(*this).GetASM(); }
        };
    }

    constexpr uSize cAllocSize = CRAFT_ALLOCATION_SIZE;
    class ManagerHook
    {
    protected: // Marked as protected so if someone inherits from this class to add extra stuff they can modify them. We dont make functions virtual cuz of overhead
        TPointerWrapper<std::vector<UnkFunc>> mPreHooks{};
        TPointerWrapper<std::vector<UnkFunc>> mPostHooks{};
        TPointerWrapper<UnkFunc> mReplaceCallback;
        TPointerWrapper<UnkFunc> mDecideCallback;

        struct OriginalFunc
        {
            u8* mOriginalBytes{};
            uSize mSize{};
            UnkFunc mCallback{};
            UnkFunc mOldFunc{}; // This is the old function that was replaced
        };
        struct Trampoline
        {
			u8* mTrampoline{};
			uSize mSize{};
		};

        struct CompileResult {
            std::unique_ptr<llvm::LLVMContext> C;
            std::unique_ptr<llvm::Module> M;
        };
        OriginalFunc mOriginalFunc{};
		Trampoline mTrampoline{};
        Internal::JmpInterface mJmpInterface{};
        bool mCanShortJump = false;
    protected:
        jmp64 createThunk64(UnkFunc targetAddress);  
        jmp32 createThunk32(i32 targetOffset);
        std::expected<TPointerWrapper<u8>, HookError> allocateMemory(UnkFunc originalFunc);
        void setupMemory(UnkFunc originalFunc);
        void installThunk();

        void generateASM(NeededHookInfo& hookInfo, CraftContext* context);
        void generateSource(Internal::InternalHookInfo&);
        llvm::Expected<std::unique_ptr<llvm::TargetMachine>> createTargetMachine(CompileResult& compResult, CraftContext* ctx);
        llvm::Expected<CompileResult> compileSource(Internal::InternalHookInfo&);
        void emitMachineCode(CompileResult& compResult, llvm::TargetMachine& target);



    public:
        ManagerHook() {}
        ManagerHook(ManagerHook&) = delete;
        ManagerHook& operator=(ManagerHook&) = delete; // Disallow copying of this class
        ManagerHook(ManagerHook&&) = default;
        ManagerHook& operator=(ManagerHook&&) = default;

        void CreateManagerHook(UnkFunc originalFunc, UnkFunc targetFunc, NeededHookInfo& hookInfo, HookType hookType, CraftContext* ctx);
        

        HookError AddHook(UnkFunc hookFunc, HookType hookType);
        void RemoveHook(UnkFunc targetFunc, HookType hookType); // This force pauses all threads to prevent any issues
        UnkFunc GetOriginalFunc() { return this->mOriginalFunc.mCallback; }


        void AddNewFuncToHooksWithNoProcess(UnkFunc ptr)
        {
            this->mPreHooks->push_back(ptr);
        }
        void AddPreNewFuncToHooksWithNoProcess(UnkFunc ptr)
        {
            this->mPostHooks->push_back(ptr);
        }
        void WriteReplaceBypass(UnkFunc ptr)
		{
            auto bp = this->mReplaceCallback.as<u64>();
            *bp = reinterpret_cast<u64>(ptr);
		}

        UnkFunc GetTrampoline() { return this->mTrampoline.mTrampoline; }
        

    };

    

}