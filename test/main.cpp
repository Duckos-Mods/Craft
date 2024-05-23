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
static void PreHook(int& i, float&, float&, thing, thing&)
{
	std::println("PreHook I={}", i);
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
[[noreturn]]
int main()
{
    constexpr Craft::TypeInformation returnTi = Craft::GetTypeInformation<int>();
    std::vector<Craft::TypeInformation> argTI;
    argTI.push_back(Craft::GetTypeInformation<int>());
    argTI.push_back(Craft::GetTypeInformation<float>());
    argTI.push_back(Craft::GetTypeInformation<float>());
    argTI.push_back(Craft::GetTypeInformation<thing>());
    argTI.push_back(Craft::GetTypeInformation<thing*>());
    argTI.push_back(Craft::GetTypeInformation<thing*>());

    Craft::NeededHookInfo nhi = Craft::GetNeededHookInfo(returnTi, argTI);


    Craft::ManagerHook mh;
    mh.CreateManagerHook(HookTarget, PreHook, nhi, Craft::HookType::PreHook);
    auto* trampPointer = mh.GetTrampoline();
    auto* tramp = reinterpret_cast<int(*)(int, float, float, thing, thing*, thing*)>(trampPointer);
    int i = 50;
    float f = 1.0f;
    thing t1{ 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };

    tramp(i, f, 2.f, t1, (thing*)123456789, (thing*)987654321);


    return 0;

}
