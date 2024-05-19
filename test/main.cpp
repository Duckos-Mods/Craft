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
static int HookTarget(int i, float, float, thing, thing&)
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
    std::println("PreHook Called!!!!! I = {}", i);
}

[[nodiscard]]
static void PostHook(int& i, int& returnValue)
{
    std::println("Post Hook Called! OG I = {}, Return Value = {}", i, returnValue);
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
    argTI.push_back(Craft::GetTypeInformation<float>());
    argTI.push_back(Craft::GetTypeInformation<float>());

    Craft::NeededHookInfo nhi = Craft::GetNeededHookInfo(returnTi, argTI);


    Craft::ManagerHook mh;
    mh.CreateManagerHook(HookTarget, PreHook, nhi, Craft::HookType::PreHook);

    return 0;

}
