#include "CraftASM.hpp"
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/Host.h>
#include <llvm/MC/MCDisassembler/MCDisassembler.h>
#include <llvm/MC/MCInstPrinter.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/MC/MCContext.h>
#include <llvm/MC/MCAsmInfo.h>
#include <llvm/MC/MCSubtargetInfo.h>
#include <llvm/MC/MCInstrInfo.h>
#include <llvm/MC/MCRegisterInfo.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetOptions.h>

namespace Craft::ASMUtil
{
    uSize GetMinBytesForNeededSize(void* ASM, uSize minBytesNeeded)
    {
		std::string error;
		using namespace llvm;
		std::string tripple = llvm::sys::getDefaultTargetTriple();
		const auto TheTriple = Triple(tripple);

		const Target* target = TargetRegistry::lookupTarget(tripple, error);
		const MCTargetOptions& targetOptions = MCTargetOptions();
		if (!target)
		{
			llvm::errs() << error;
			return 0;
		}
		static const std::string cpu = "generic";
		static const std::string features = "+sse3,+sse4.1,+sse4.2,+avx,+avx2";
		TargetOptions options;
		std::unique_ptr<MCRegisterInfo> MRI(target->createMCRegInfo(tripple));
		std::unique_ptr<MCAsmInfo> MAI(target->createMCAsmInfo(*MRI.get(), StringRef(tripple), targetOptions));
		std::unique_ptr<MCSubtargetInfo> STI(target->createMCSubtargetInfo(tripple, cpu, features));
		std::unique_ptr<MCInstrInfo> MII(target->createMCInstrInfo());
		MCContext ctx(TheTriple, MAI.get(), MRI.get(), STI.get());
		MCDisassembler* DisAsm = target->createMCDisassembler(*STI, ctx);
		if (!DisAsm)
		{
			llvm::errs() << "Failed to create disassembler\n";
			return 0;
		}

		// we hard coded 1 because  it gives me intel syntax and thats the only true syntax AT&T is a lie
		std::unique_ptr<MCInstPrinter> IP(target->createMCInstPrinter(TheTriple, 1, *MAI, *MII, *MRI));

		ArrayRef<uint8_t> code((uint8_t*)ASM, minBytesNeeded * 10); // 10 is a guess
		uint64_t index = 0;
		uint64_t currentSize = 0;
		while (currentSize < minBytesNeeded)
		{
			MCInst instruction;
			uint64_t size;
			MCDisassembler::DecodeStatus S = DisAsm->getInstruction(instruction, size, code.slice(index), 0, nulls());
			if (S == MCDisassembler::Fail)
			{
				llvm::errs() << "Failed to decode instruction\n";
				index += size;
			}
			else
			{
				TEST_ONLY(IP->printInst(&instruction,(u64)ASM, "", *STI, outs()));
				TEST_ONLY(outs() << "\n");
				index += size;
				currentSize += size;
			}






		}
		return currentSize;
    }
}
