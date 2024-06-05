#include "CraftContext.hpp"

#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
    
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/AllocatorBase.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/ADT/StringMap.h>
#include <sstream>
	

namespace Craft
{
	CraftContext::CraftContext(int argc, char** argv) : mllvmInit(argc, argv, true)
	{
        llvm::InitializeAllTargetInfos();
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllDisassemblers();
        llvm::InitializeAllAsmPrinters();
        llvm::InitializeAllAsmParsers();
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmParser();
        llvm::InitializeNativeTargetAsmPrinter();

		llvm::StringMap<bool, llvm::MallocAllocator> features;
		llvm::sys::getHostCPUFeatures(features);
        std::stringstream ss;
		for (auto& [key, value] : features)
		{
            if (!value)
                continue;
            ss << std::format("+{},", key.str());
            mCPUFeaturesVec.push_back(std::format("+{}", key.str()));
		}
        // Remove trailing comma
        this->mCpuFeats = ss.str();
        mCpuFeats.pop_back();


	}
    CraftContext::~CraftContext()
    {
    }
}
