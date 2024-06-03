#pragma once
#include <llvm/Support/InitLLVM.h>



namespace Craft
{
	class CraftContext
	{
	public:
		CraftContext(int argc = 0, char** argv = nullptr);
		~CraftContext();
	private:
		llvm::InitLLVM mllvmInit;
	};
}