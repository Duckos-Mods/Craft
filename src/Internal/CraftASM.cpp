#include "CraftASM.hpp"
#include <zasm/zasm.hpp>
namespace Craft::ASMUtil
{
    uSize GetMinBytesForNeededSize(void* ASM, uSize minBytesNeeded)
    {
		size_t size = 0;
		char* addr = static_cast<char*>(ASM);
		zasm::Decoder decoder(zasm::MachineMode::AMD64);
		while (size < minBytesNeeded)
		{
			zasm::Instruction instr;
			const auto res = decoder.decode(addr + size, 0xFF, (uint64_t)addr);
			CRAFT_THROW_IF_NOT(res, "Failed to decode instruction");
			const auto& instrInfo = *res;
			size += instrInfo.getLength();
		}
		return size;
    }
}
