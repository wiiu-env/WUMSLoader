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

extern WUMSRPLAllocatorAllocFn gCustomRPLAllocatorAllocFn;
extern WUMSRPLAllocatorFreeFn gCustomRPLAllocatorFreeFn;

#define MEMORY_REGION_START                           0x00800000
#define MEMORY_REGION_SIZE                            0x00800000

#define RELOCATOR_SIZE                                0x50000 // Maximum size of the wumsloader, needs to match the one defined in link.ld
#define ENVIRONMENT_PATH_LENGTH                       0x100   // Length of the EnvironmentPath.
#define MEMORY_REGION_USABLE_MEM_REGION_END_LENGTH    0x04    // sizeof(uint32_t)


#define MEMORY_REGION_ENVIRONMENT_STRING_ADRR         (MEMORY_REGION_START + RELOCATOR_SIZE)
#define MEMORY_REGION_USABLE_MEM_REGION_END_VALUE_PTR ((uint32_t *) (MEMORY_REGION_ENVIRONMENT_STRING_ADRR + ENVIRONMENT_PATH_LENGTH))
#define MEMORY_REGION_USABLE_MEM_REGION_END_VALUE     (*MEMORY_REGION_USABLE_MEM_REGION_END_VALUE_PTR)

// Technically we overwrite the CustomRPXLoader that is still loaded at 0x00800000...
// We can get away with it because the EnvironmentLoader exits instead of returning to the CustomRPXLoader.
#define MEMORY_REGION_USABLE_HEAP_START               ((uint32_t) MEMORY_REGION_USABLE_MEM_REGION_END_VALUE_PTR + MEMORY_REGION_USABLE_MEM_REGION_END_LENGTH)
#define MEMORY_REGION_USABLE_HEAP_END                 MEMORY_REGION_USABLE_MEM_REGION_END_VALUE

#define ENVRIONMENT_STRING                            ((char *) MEMORY_REGION_ENVIRONMENT_STRING_ADRR)

#define WUMS_LOADER_SETUP_MAGIC_WORD                  0x13371337
