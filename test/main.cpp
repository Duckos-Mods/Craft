#include <Craft.hpp>
#include <iostream>
#include <map>
#include <list>
#include <print>
#include <chrono>
#include <intrin.h>


[[nodiscard]]
int HookTarget(int i, int b)
{
    i+= b;
    std::println("CALLED HookTarget i = {}", i);
    return 69;
}

void* ogFunc = nullptr;


[[nodiscard]]
static int ReplaceHookTarget()
{
    return reinterpret_cast<int(*)()>(ogFunc)();
}

[[nodiscard]]
static void PreHook()
{
    std::println("PREHOOK");
}
[[nodiscard]]
static void PostHook(int& returnData, float& i, int& j, int& k, int& l, uint64_t& m, int& n)
{
    std::println("Return Data: {}", returnData);
    returnData++;
    i++;
    j++;
    k++;
    l++;
    m++;
    n++;
    std::println("i: {}, j: {}, k: {}, l: {}, m: {}, n: {}", i, j, k, l, m, n);
}

int main(int argc, char** argv) {

    Craft::CraftContext ctx(argc, argv);

    Craft::ManagerHook hm;
    auto returnTI = Craft::GetTypeInformation<int>();
    std::vector<Craft::TypeInformation> argTIs;
    argTIs.push_back(Craft::GetTypeInformation<int>());
    argTIs.push_back(Craft::GetTypeInformation<int>());
    auto NHI = Craft::GetNeededHookInfo(returnTI, argTIs);
    // Start timer
    auto start = std::chrono::high_resolution_clock::now();
    auto Func = Craft::NonStaticLocalFunctionToRealAddress(HookTarget);
    hm.CreateManagerHook(Func, PreHook, NHI, Craft::HookType::PreHook, &ctx);
    // Stop timer
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    std::println("Time taken to create hook: {} microseconds", duration.count());

    //hm.AddNewFuncToHooksWithNoProcess(PreHook);
    //hm.AddPreNewFuncToHooksWithNoProcess(PostHook);
    //hm.WriteReplaceBypass(ReplaceHookTarget);

    ogFunc = hm.GetOriginalFunc();
    HookTarget(1, 1);

}

