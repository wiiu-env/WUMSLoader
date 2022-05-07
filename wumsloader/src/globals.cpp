#include "globals.h"

MEMHeapHandle gHeapHandle __attribute__((section(".data"))) = nullptr;
uint8_t gInitCalled __attribute__((section(".data")))       = 0;
module_information_t gModuleInformation __attribute__((section(".data")));
std::vector<std::shared_ptr<ModuleData>> gLoadedModules __attribute__((section(".data")));
std::unique_ptr<module_information_single_t[]> gModuleDataInfo __attribute__((section(".data")));
