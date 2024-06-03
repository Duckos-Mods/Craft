#include "CraftContext.hpp"

#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
    
#include <llvm/Support/FileSystem.h>
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
	}
    CraftContext::~CraftContext()
    {
    }
}
