#include "CraftHook.hpp"
#include <Internal/CraftThreads.hpp>
#include <Internal/CraftMemory.hpp>

#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/TargetParser.h>
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
#include <clang/Basic/TargetInfo.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/MemoryBuffer.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/FrontendOptions.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/ADT/Triple.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Basic/Diagnostic.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/MC/MCContext.h>

#ifndef CODE_LINE
#define CODE_LINE(x) x"\n"
#endif
namespace Craft
{


	constexpr uSize vecSize = sizeof(std::vector<UnkFunc>);
	constexpr uSize UnkFuncSize = sizeof(UnkFunc);
	jmp64 ManagerHook::createThunk(UnkFunc targetAddress)
	{
		return jmp64(reinterpret_cast<u64>(targetAddress));
	}
	void ManagerHook::setupMemory(UnkFunc originalFunc)
	{
		u8* memory = (u8*)UNROLL_EXPECTED(OSAlloc(
			cAllocSize,
			{ true, true, true },
			originalFunc,
			Sizes::GiB * 2) // 2 Gib in each direction from code
		);
		// "Failed to allocate memory for hooking function"

		OSErr err = OSProtect(
			originalFunc,
			512,
			{ true, true, true }
		);
		CRAFT_THROW_IF_NOT(err == OSErr::NONE, "Failed to protect memory for original function");

		this->mPreHooks = new(memory + cAllocSize - vecSize) std::vector<UnkFunc>;
		this->mPostHooks = new(memory + cAllocSize - vecSize*2) std::vector<UnkFunc>;
		this->mReplaceCallback = new(memory + cAllocSize - vecSize * 2 - UnkFuncSize) UnkFunc();
		this->mDecideCallback = new(memory + cAllocSize - vecSize * 2 - UnkFuncSize * 2) UnkFunc();
		auto* val = (u64*)this->mReplaceCallback;
		*val = (u64)0xDEADBEEF;
		auto currentAllocateHead = memory + cAllocSize - vecSize * 2 - UnkFuncSize * 2;

		this->mPreHooks->reserve(16);
		this->mPostHooks->reserve(16);


		// I am not sure if this is the best way to do this but it works
		this->mOriginalFunc.mSize = (u32)ASMUtil::GetMinBytesForNeededSize(originalFunc, 14);
		uSize allocFirstSize = this->mOriginalFunc.mSize;
		currentAllocateHead -= allocFirstSize;
		this->mOriginalFunc.mOriginalBytes = new(currentAllocateHead) u8[allocFirstSize];
		allocFirstSize += 14;
		currentAllocateHead -= allocFirstSize;
		this->mOriginalFunc.mCallback = new(currentAllocateHead) u8[allocFirstSize];


		//this->mOriginalFunc.mOriginalBytes = new u8[this->mOriginalFunc.mSize];
		/*auto memAlloc = OSAlloc(this->mOriginalFunc.mSize + 14, { true, true, true }, originalFunc);
		if (!memAlloc.has_value())
			CRAFT_THROW("Failed to allocate memory for original function");*/

			//new u8[this->mOriginalFunc.mSize + 14]; 
		memcpy(this->mOriginalFunc.mOriginalBytes, originalFunc, this->mOriginalFunc.mSize);

		this->mTrampoline.mSize = cAllocSize;
		this->mTrampoline.mTrampoline = memory;
		TEST_LOG("Trampoline: {}", (u64)this->mTrampoline.mTrampoline);
	}
	void ManagerHook::installThunk()
	{
		jmp64 jmpToTrampoline = createThunk(this->mTrampoline.mTrampoline);
		jmp64 jmpBackToOgFunction = createThunk((u8*)this->mOriginalFunc.mOldFunc + this->mOriginalFunc.mSize);
		UnkFunc callback = this->mOriginalFunc.mCallback;
		memcpy(callback, this->mOriginalFunc.mOriginalBytes, this->mOriginalFunc.mSize);
		memcpy((u8*)callback + this->mOriginalFunc.mSize, jmpBackToOgFunction.GetASM(), 14);
		memcpy(this->mOriginalFunc.mOldFunc, jmpToTrampoline.GetASM(), 14);



	}

	void ManagerHook::generateASM(NeededHookInfo& hookInfo, CraftContext* context)
	{
		Internal::InternalHookInfo ihi{ hookInfo };
		ihi.mContext = context;

		generateSource(ihi);
		auto returnData = compileSource(ihi);
		if (!returnData)
		{
			TEST_LOG("Failed to compile source: {}", toString(returnData.takeError()));
			return;
		}
		auto targetMachine = createTargetMachine(returnData.get(), context);
		if (!targetMachine)
		{
			TEST_LOG("Failed to create target machine: {}", toString(targetMachine.takeError()));
			return;
		}
		emitMachineCode(returnData.get(), *targetMachine.get());
	}

	void ManagerHook::generateSource(Internal::InternalHookInfo& ihi)
	{
		std::stringstream ss;
		std::vector<std::string> typeNames;
		std::vector<std::string> argNames;
		auto& ALI = ihi.mHookInfo.argumentLocationInfo;
		ss <<
			CODE_LINE("using u64 = unsigned long long;")
			CODE_LINE("using u32 = unsigned int;")
			CODE_LINE("using u16 = unsigned short;")
			CODE_LINE("using u8 = unsigned char;")
			CODE_LINE("using f32 = float;")
			CODE_LINE("using f64 = double;")

			CODE_LINE("template<typename T>")
			CODE_LINE("class CxxVec {")
			CODE_LINE("private:")
			CODE_LINE("	T* mdata;")
			CODE_LINE("	T* mend;")
			CODE_LINE("	T* mallocationEnd;")
			CODE_LINE("public:")
			CODE_LINE("	CxxVec() = delete;")
			CODE_LINE("	inline T* begin() {return mdata;}")
			CODE_LINE("	inline T* end() {return mend;}")
			CODE_LINE("};");
		;
		for (int i = 0; i < ALI.size(); i++)
		{
			auto& ali = ALI[i];
			argNames.push_back(std::format("a{}", i));

			if (ali.size > 8)
			{
				ss << std::format(
					CODE_LINE("struct {}Struct {{")
					CODE_LINE("	u8 data[{}];")
					CODE_LINE("}};"),
					argNames.back(),
					ali.size
				);
				typeNames.push_back(std::format("{}Struct", argNames.back()));
			}
			else if (ali.size == 8 && ali.shouldTakeAddress == false)
				typeNames.push_back("void*");
			else if (ali.size > 4 && ali.shouldTakeAddress == true)
				if (TO_UNDERLYING(ali.location) >= TO_UNDERLYING(VariableLocations::XMM0) && TO_UNDERLYING(ali.location) <= TO_UNDERLYING(VariableLocations::XMM3))
					typeNames.push_back("f64");
				else
					typeNames.push_back("u64");
			else if (ali.size <= 4 && ali.size > 2)
				if (TO_UNDERLYING(ali.location) >= TO_UNDERLYING(VariableLocations::XMM0) && TO_UNDERLYING(ali.location) <= TO_UNDERLYING(VariableLocations::XMM3))
					typeNames.push_back("f32");
				else
					typeNames.push_back("u32");
			else if (ali.size <= 2 && ali.size > 1)
				typeNames.push_back("u16");
			else if (ali.size == 1)
				typeNames.push_back("u8");
		}
		std::string returnType = "void";
		auto& hi = ihi.mHookInfo;
		if (hi.ShouldPassReturnValue)
		{
			if (hi.ShouldUseWideRegisters)
			{
				std::format(
					CODE_LINE("struct ReturnStruct {{"),
					CODE_LINE("	u64 data[2];"),
					CODE_LINE("}};")
				);
				returnType = "ReturnStruct";
				goto BREAKOUT;
			}

			if (hi.ReturnArgSize > 8)
			{
				ss << std::format(
					CODE_LINE("struct ReturnStruct {{")
					CODE_LINE("	u8 data[{}];")
					CODE_LINE("}};"),
					hi.ReturnArgSize
				);
				returnType = "ReturnStruct";
			}
			else if (hi.ReturnArgSize > 4)
				returnType = "u64";
			else if (hi.ReturnArgSize <= 4 && hi.ReturnArgSize > 2)
				returnType = "u32";
			else if (hi.ReturnArgSize <= 2 && hi.ReturnArgSize > 1)
				returnType = "u16";
			else if (hi.ReturnArgSize == 1)
				returnType = "u8";
		}
	BREAKOUT:

		ss << std::format(CODE_LINE("using RetType = {};"), returnType);

		std::stringstream PreHook;
		PreHook << std::format(
			"void(*)("
		);
		for (int i = 0; i < typeNames.size(); i++)
		{
			char ref = ALI[i].shouldTakeAddress ? '&' : ' ';

			PreHook << std::format(
				"{}{} {}",
				typeNames[i],
				ref,
				argNames[i]
			);
			if (i != typeNames.size() - 1)
				PreHook << ", ";
		}
		PreHook << ");";
		std::string PreHookStr = PreHook.str();
		std::stringstream PostHook;
		PostHook << std::format(
			"void(*)("
		);
		if (hi.ShouldPassReturnValue)
		{
			PostHook << "RetType& returnData";
			if (typeNames.size() != 0)
				PostHook << ", ";
		}

		for (int i = 0; i < typeNames.size(); i++)
		{
			char ref = ALI[i].shouldTakeAddress ? '&' : ' ';

			PostHook << std::format(
				"{}{} {}",
				typeNames[i],
				ref,
				argNames[i]
			);
			if (i != typeNames.size() - 1)
				PostHook << ", ";
		}
		PostHook << ");";
		std::stringstream NormalFunction;
		NormalFunction << std::format(
			"RetType(*)("
		);

		for (int i = 0; i < typeNames.size(); i++)
		{
			NormalFunction << std::format(
				"{} {}",
				typeNames[i],
				argNames[i]
			);
			if (i != typeNames.size() - 1)
				NormalFunction << ", ";
		}
		NormalFunction << ");";

		std::stringstream CheckFunction;
		CheckFunction << std::format(
			"bool(*)("
		);

		for (int i = 0; i < typeNames.size(); i++)
		{
			char ref = ALI[i].shouldTakeAddress ? '&' : ' ';

			CheckFunction << std::format(
				"{}{} {}",
				typeNames[i],
				ref,
				argNames[i]
			);
			if (i != typeNames.size() - 1)
				CheckFunction << ", ";
		}
		CheckFunction << ");";

		std::string PostHookStr = PostHook.str();
		ss << std::format(
			CODE_LINE("using PreHook = {}"),
			PreHookStr
		);
		ss << std::format(
			CODE_LINE("using PostHook = {}"),
			PostHookStr
		);
		ss << std::format(
			CODE_LINE("using NormalFunction = {}"),
			NormalFunction.str()
		);
		ss << std::format(
			CODE_LINE("using CheckFunction = {}"),
			CheckFunction.str()
		);

		ss << std::format("RetType HookFunction(");
		for (int i = 0; i < typeNames.size(); i++)
		{
			ss << std::format(
				"{} {}, ",
				typeNames[i],
				argNames[i]
			);
		}

		std::stringstream call;
		for (int i = 0; i < typeNames.size(); i++)
		{
			call << std::format(
				"{}, ", 
				argNames[i]
			);
		}

		call.seekp(-2, std::ios_base::end);
		call << "  ";
		std::string callStr = call.str();
		std::string postHookCallStr = (ALI.size() == 0) ? (hi.ShouldPassReturnValue) ? "returnData" : "" : (hi.ShouldPassReturnValue) ? std::format("returnData, {}", callStr) : callStr;

		if (typeNames.size() != 0)
			ss.seekp(-2, std::ios_base::end);

		ss << std::format(
			CODE_LINE(") {{")
			CODE_LINE("	CxxVec<PreHook>* PreHooks = (CxxVec<PreHook>*){};")
			CODE_LINE("	CxxVec<PostHook>* PostHooks = (CxxVec<PostHook>*){};")
			CODE_LINE("	NormalFunction* ReplaceCallback = (NormalFunction*){};")
			CODE_LINE("	NormalFunction DefaultCallback = (NormalFunction){};")
			CODE_LINE("	for (auto& hook : *PreHooks)")
			CODE_LINE("		hook({});"),
			(u64)this->mPreHooks,
			(u64)this->mPostHooks,
			(u64)this->mReplaceCallback,
			(u64)this->mOriginalFunc.mCallback,
			callStr
		);



		if (hi.ShouldPassReturnValue)
			ss << std::format(
				CODE_LINE("	RetType returnData;")
			);

		ss << std::format(
			CODE_LINE("	bool shouldCallCustom = (*(u64*)ReplaceCallback != 0xDEADBEEF) ? true : false;")
		);
		if (hi.ShouldPassReturnValue)
			ss << std::format(
				CODE_LINE("	returnData = (shouldCallCustom) ? (*ReplaceCallback)({}) : DefaultCallback({});"),
				callStr,
				callStr
			);
		else
			ss << std::format(
				CODE_LINE("	(shouldCallCustom) ? (*ReplaceCallback)({}) : DefaultCallback({});"),
				callStr,
				callStr
			);
		ss << std::format(
			CODE_LINE("	for (auto& hook : *PostHooks)")
			CODE_LINE("		hook({});"),
			postHookCallStr
		);

		if (hi.ShouldPassReturnValue)
			ss << std::format(
				CODE_LINE("	return returnData;")
			);
		ss << CODE_LINE("}");
				
		
		TEST_LOG("CODE: {}", ss.str());
		ihi.mSource = std::move(ss.str());
	}

	llvm::Expected<std::unique_ptr<llvm::TargetMachine>> ManagerHook::createTargetMachine(ManagerHook::CompileResult& compResult, CraftContext* ctx)
	{
		using namespace llvm;
		std::string error;
		std::string triple = sys::getDefaultTargetTriple();
		std::string cpu = sys::getHostCPUName().str();
		std::string features = ctx->GetCpuFeats();
		const auto* target = TargetRegistry::lookupTarget(triple, error);
		if (!target)
		{
			TEST_LOG("Failed to get target: {}", error);
			return llvm::make_error<StringError>(error, llvm::inconvertibleErrorCode());
		};
		TargetOptions opts;
		auto* targetMachine = target->createTargetMachine(
			triple,
			cpu,
			features,
			opts,
			llvm::Optional<Reloc::Model>()
		);
		return std::unique_ptr<TargetMachine>(targetMachine);
	}

	llvm::Expected<ManagerHook::CompileResult> ManagerHook::compileSource(Internal::InternalHookInfo& ihi)
	{
		using namespace llvm;
		using namespace clang;
		IntrusiveRefCntPtr<DiagnosticOptions> diagOpts = new DiagnosticOptions();
		diagOpts->ShowColors = 1;
		diagOpts->ShowPresumedLoc = 1;
		 
		TextDiagnosticPrinter* diagClient = new TextDiagnosticPrinter(llvm::errs(), diagOpts.get());
		std::unique_ptr<DiagnosticConsumer> diagConsumer(diagClient);

		auto diagEngine = std::make_unique<DiagnosticsEngine>
			(
				nullptr,
				diagOpts,
				diagClient,
				false
			);

		constexpr const char* codeFileName = "hook.cpp";


		auto& cc = ihi.mCompilerInstance;
		if (!CompilerInvocation::CreateFromArgs(
			cc.getInvocation(),
			ArrayRef<const char*>{ codeFileName },
			*diagEngine
		))
		{
			TEST_LOG("Failed to create compiler invocation");
			return llvm::make_error<StringError>(
				"Failed to create compiler invocation",
				errc::invalid_argument);
		}
		auto& opts = cc.getTargetOpts();
		opts.Triple = sys::getDefaultTargetTriple();
		opts.CPU = sys::getHostCPUName();
		opts.FeaturesAsWritten = ihi.mContext->GetCPUFeaturesVec();

		cc.createDiagnostics(diagConsumer.get(), false);
		std::unique_ptr<MemoryBuffer> codeBuffer = MemoryBuffer::getMemBuffer(ihi.mSource);
		cc.getPreprocessorOpts().addRemappedFile(
			codeFileName, codeBuffer.release()
		);

		auto& codeGenOpts = cc.getCodeGenOpts();
		codeGenOpts.OptimizationLevel = 3;
		codeGenOpts.setInlining(CodeGenOptions::NormalInlining);

		EmitLLVMOnlyAction emitLLVMAction;
		if (!cc.ExecuteAction(emitLLVMAction))
		{
			TEST_LOG("Failed to execute action");
			return llvm::make_error<StringError>(
				"Failed to execute action",
				errc::invalid_argument);
		}
		auto mod = emitLLVMAction.takeModule();
		assert(mod);
		auto* ctx = emitLLVMAction.takeLLVMContext();
		TEST_ONLY(
			mod->print(outs(), nullptr, false, true)
			);

		return CompileResult{ std::unique_ptr<llvm::LLVMContext>(ctx), std::move(mod)};
	}

	void ManagerHook::emitMachineCode(CompileResult& compResult, llvm::TargetMachine& targetMachine)
	{
		using namespace llvm;
		auto mod = std::move(compResult.M);
		auto ctx = std::move(compResult.C);
		mod->setDataLayout(targetMachine.createDataLayout());

		llvm::legacy::PassManager passManager;
		llvm::SmallVector<char, 0> buffer;
		llvm::raw_svector_ostream dest(buffer);


		if (targetMachine.addPassesToEmitFile(
			passManager,
			dest,
			nullptr,
			llvm::CGFT_ObjectFile))
		{
			TEST_LOG("Failed to add passes to emit file");
			return;
		}
		passManager.run(*mod);
		auto memBuff = MemoryBuffer::getMemBufferCopy(StringRef(buffer.data(), buffer.size()));
		auto objectFileOrError = llvm::object::ObjectFile::createObjectFile(memBuff->getMemBufferRef());
		if (!objectFileOrError) {
			llvm::errs() << "Error creating object file from memory buffer\n";
			return;
		}
		auto objectFile = std::move(objectFileOrError.get());

		for (const auto& section : objectFile->sections()) {
			if (section.isText()) {
				auto data = section.getContents();
				if (!data) {
					llvm::errs() << "Error reading section contents\n";
					return;
				}
				memcpy(this->mTrampoline.mTrampoline, data->data(), data->size());
				break;
			}
		}
		mod.release();
		ctx.release();
	}

	void ManagerHook::CreateManagerHook(UnkFunc originalFunc, UnkFunc targetFunc, NeededHookInfo& hookInfo, HookType hookType, CraftContext* ctx, bool pauseThreads)
	{
		if (pauseThreads)
			AllOSThreads pauseThreads{}; // This auto releases when it goes out of scope so we dont have to worry about it
		this->mOriginalFunc.mOldFunc = originalFunc;
		setupMemory(originalFunc);
		generateASM(hookInfo, ctx);

		installThunk(); // Do this last so if threads are not paused we hopefully dont jump while building the hook

	}
	void ManagerHook::RemoveHook(UnkFunc targetFunc, HookType hookType)
	{
	}

}


