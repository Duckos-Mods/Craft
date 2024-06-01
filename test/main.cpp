#include <Craft.hpp>
#include <iostream>
#include <map>
#include <list>
#include <print>

struct alignas(16) thing
{
    float x, y, z, w, f;
};

[[nodiscard]]
static int HookTarget(float, int, int, int, uint64_t, thing*)
{
    return 0;
}

[[nodiscard]]
static void PreHook(float& i, int& j, int& k, int& l, int& m, thing* t)
{
    std::println("I={}", i);
	std::println("J={}", j);
	std::println("K={}", k);
	std::println("L={}", l);
	std::println("M={}", m);
    i += 2;
    j += 2;
    k += 2;
    l += 2;
    m += 2;

    std::println("t->x={}", t->x);
    std::println("t->y={}", t->y);
    std::println("t->z={}", t->z);
    std::println("t->w={}", t->w);
    std::println("t->f={}", t->f);
    t->x += 2;
    t->y += 2;
    t->z += 2;
    t->w += 2;
    t->f += 2;


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

    Craft::ManagerHook hm;
    auto returnTI = Craft::GetTypeInformation<int>();
    auto arg1TI = Craft::GetTypeInformation<float>();
    auto arg2TI = Craft::GetTypeInformation<int>();
    auto arg3TI = Craft::GetTypeInformation<int>();
    auto arg4TI = Craft::GetTypeInformation<int>();
    auto arg5TI = Craft::GetTypeInformation<int>();
    auto arg6TI = Craft::GetTypeInformation<thing*>();
    auto NHI = Craft::GetNeededHookInfo(returnTI, {
        arg1TI,
        arg2TI,
        arg3TI,
        arg4TI,
        arg5TI,
        arg6TI
        });
    hm.CreateManagerHook(HookTarget, PreHook, NHI, Craft::HookType::PreHook, true);

    auto tmp = hm.GetTrampoline();
    for (int i = 0; i < 100; i++) {
        hm.AddNewFuncToHooksWithNoProcess(PreHook);
	}
    auto func = reinterpret_cast<int(*)(float, int, int, int, int,thing*)>(tmp);
    thing t = { 7.0f, 6.0f, 4.0f, 3.0f, 2.0f };
    func(1222.222f, 2, 3, 4, 5, &t);

    std::println("Ran Code And Stack Was Not Corrupted!!!!!");


}

