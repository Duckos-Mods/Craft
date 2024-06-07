#pragma once
#include <cstdint>
#include <array>
#include <span>

#include <Internal/CraftUtils.hpp>
namespace Craft
{
    struct jmp64
    {
        std::array<uint8_t, 14> code = {
            0xFF,
            0x25,
            0x0,
            0x0,
            0x0,
            0x0,
            0x1,
            0x2,
            0x3,
            0x4,
            0x5,
            0x6,
            0x7,
            0x8
        };

        jmp64(uint64_t address)
        {
            uint8_t* codeData = code.data();
            uint64_t* addressPtr = (uint64_t*)(codeData + 6);
            *addressPtr = address;
        }

        uint8_t* GetASM()
        {
            return code.data();
        }

        std::span<uint8_t> GetASMSpan()
        {
            return code;
        }

    };

    struct jmp32
    {
        std::array<uint8_t, 5> code = {
			0xE9,
			0x0,
			0x0,
			0x0,
			0x0
		};
        jmp32(i32 address)
        {
			uint8_t* codeData = code.data();
			uint32_t* addressPtr = (uint32_t*)(codeData + 1);
			*addressPtr = address;
		}

        uint8_t* GetASM()
        {
			return code.data();
		}

        std::span<uint8_t> GetASMSpan()
        {
			return code;
		}

    };

    class ASMBuff
    {
    private:
        u8* getWritePtr() {
            return mByteBuffer + mOffset;
        }
    private:
        u8* mByteBuffer{};
        u32 mSize{};
        u32 mOffset = {};
    public:
        ASMBuff(u32 size)
		{
			mByteBuffer = new u8[size];
			mSize = size;
		}

        ASMBuff(u8* buffer, u32 size) : mByteBuffer(buffer), mSize(size) {}

		~ASMBuff()
		{
			delete[] mByteBuffer;
		}

		u8* GetBuffer()
		{
			return mByteBuffer;
		}

		u32 GetSize()
		{
			return mSize;
		}

        void Write(u8* data, u32 size) {
            memcpy(getWritePtr(), data, size);
            mOffset += size;
        }

        template<typename T>
        void Write(T* data, u32 size = sizeof(T))
        {
            u8* dataPtr = (u8*)data;
            Write(dataPtr, size);
        }

        void Seekg(u32 offset) { mOffset = offset; }
        u32 Tellg() { return mOffset; }



    };
    namespace ASMUtil
    {
        uSize GetMinBytesForNeededSize(void* ASM, uSize minBytesNeeded);


    }
}
  