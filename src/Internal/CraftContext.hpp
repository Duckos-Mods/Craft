#pragma once
#include <llvm/Support/InitLLVM.h>
#include <string>
#include <llvm/ADT/StringRef.h>
#include <vector>

namespace Craft
{
	class CraftContext
	{
	public:
		CraftContext(int argc = 0, char** argv = nullptr);
		~CraftContext();

		std::string& GetCpuFeats() { return mCpuFeats; }
		const std::vector<std::string>& GetCPUFeaturesVec() const { return mCPUFeaturesVec; }
	private:
		llvm::InitLLVM mllvmInit;
		std::string mCpuFeats;
		std::vector<std::string> mCPUFeaturesVec;
	};
}