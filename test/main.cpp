#include <Craft.hpp>
#include <iostream>
#include <map>
#include <list>
#include <print>

struct thing
{
    float x, y, z, w, f;
};

[[nodiscard]]
static int HookTarget(int i, float, float, thing, thing*, thing*)
{
    std::println("I={}", i++);
    std::println("I={}", i++);
    std::println("I={}", i++);
    std::println("I={}", i++);
    std::println("I={}", i++);
    std::println("I={}", i++);
    return i;
}

[[nodiscard]]
static void PreHook(int& i, float&, float&, thing, thing*, thing*)
{
	std::println("PreHook I={}", i);
    i++;
}


template <typename FunctionPtr>
void* lftrw(FunctionPtr func)
{

    void* pFunction = *reinterpret_cast<void**>(&func);

    char* ptr = reinterpret_cast<char*>(pFunction);
    ptr++;
    int32_t offset = *reinterpret_cast<int32_t*>(ptr);
    ptr--;
    uint64_t target = ((uint64_t)ptr + offset);
    while (target % 16 != 0) {
        target++;
    }
    return (void*)target;
}


int main() {
}