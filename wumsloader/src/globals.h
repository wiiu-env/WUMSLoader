#pragma once

#include "module/ModuleData.h"
#include <coreinit/dynload.h>
#include <coreinit/memheap.h>
#include <cstdint>
#include <memory>

extern uint8_t gInitCalled;
extern MEMHeapHandle gHeapHandle;
extern module_information_t gModuleInformation;
extern std::vector<std::shared_ptr<ModuleData>> gLoadedModules;
extern std::unique_ptr<module_information_single_t[]> gModuleDataInfo;
extern std::map<std::string, OSDynLoad_Module> gUsedRPLs;
extern std::vector<void *> gAllocatedAddresses;

#define MEMORY_REGION_START                   0x00800000
#define MEMORY_REGION_SIZE                    0x00800000

#define CUSTOM_RPX_LOADER_RETURN_CODE         0x00009000 // We have to skip the first 0x00009000 bytes because it's still used
#define RELOCATOR_SIZE                        0x52000    // Maximum size of the wumsloader, needs to match the one defined in link.ld
#define ENVIRONMENT_PATH_LENGTH               0x100      // Length of the EnvironmentPath.

#define MEMORY_REGION_ENVIRONMENT_STRING_ADRR (MEMORY_REGION_START + CUSTOM_RPX_LOADER_RETURN_CODE + RELOCATOR_SIZE)
#define MEMORY_REGION_USABLE_HEAP_START       (MEMORY_REGION_ENVIRONMENT_STRING_ADRR + ENVIRONMENT_PATH_LENGTH)
#define MEMORY_REGION_USABLE_HEAP_END         (0x00FFF000) // We need to leave space for the BAT hook

#define ENVRIONMENT_STRING                    ((char *) MEMORY_REGION_ENVIRONMENT_STRING_ADRR)
