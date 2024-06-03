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


}

int main(int argc, char** argv) {

    Craft::CraftContext ctx(argc, argv);

    Craft::ManagerHook hm;
    auto returnTI = Craft::GetTypeInformation<void>();
    auto arg1TI = Craft::GetTypeInformation<int>();
    auto arg2TI = Craft::GetTypeInformation<thing*>();
    auto arg3TI = Craft::GetTypeInformation<thing*>();
    auto arg4TI = Craft::GetTypeInformation<thing*>();
    auto arg5TI = Craft::GetTypeInformation<thing*>();
    auto arg6TI = Craft::GetTypeInformation<int>();
    std::vector<Craft::TypeInformation> argTIs = {arg1TI, arg2TI, arg3TI, arg4TI, arg5TI, arg6TI};
    /*for (int i = 0; i < 20; i++)
        argTIs.push_back(Craft::GetTypeInformation<int>());*/
    auto NHI = Craft::GetNeededHookInfo(returnTI, argTIs);

    hm.CreateManagerHook(HookTarget, PreHook, NHI, Craft::HookType::PreHook, false);
}

