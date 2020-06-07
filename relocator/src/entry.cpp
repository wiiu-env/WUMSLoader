#include <vector>
#include <string>
#include <string.h>
#include <stdint.h>
#include <coreinit/dynload.h>
#include <coreinit/cache.h>
#include <nsysnet/socket.h>
#include <coreinit/memorymap.h>
#include <map>
#include <algorithm>
#include "../../source/module/RelocationData.h"
#include "../../source/module/ModuleData.h"
#include "ModuleDataPersistence.h"
#include "ElfUtils.h"

#include "utils/logger.h"
#include "utils/dynamic.h"
#include "globals.h"
#include "hooks.h"

uint8_t gFunctionsPatched __attribute__((section(".data"))) = 0;
uint8_t gInitCalled __attribute__((section(".data"))) = 0;

std::vector<ModuleData> OrderModulesByDependencies(const std::vector<ModuleData> &loadedModules);

extern "C" void doStart(int argc, char **argv);
// We need to wrap it to make sure the main function is called AFTER our code.
// The compiler tries to optimize this otherwise and calling the main function earlier
extern "C" int _start(int argc, char **argv) {
    InitFunctionPointers();
    socket_lib_init();
    log_init();

    doStart(argc, argv);

    DEBUG_FUNCTION_LINE("Call real one\n");
    return ((int (*)(int, char **)) (*(unsigned int *) 0x1005E040))(argc, argv);
}

bool doRelocation(std::vector<RelocationData> &relocData, relocation_trampolin_entry_t *tramp_data, uint32_t tramp_length, bool replaceAllocFunctions) {
    std::map<std::string, OSDynLoad_Module> moduleCache;
    for (auto const &curReloc : relocData) {
        std::string functionName = curReloc.getName();
        std::string rplName = curReloc.getImportRPLInformation().getName();
        uint32_t functionAddress = 0;

        for (uint32_t i = 0; i < MAXIMUM_MODULES; i++) {
            if (rplName.compare(gModuleData->module_data[i].module_export_name) == 0) {
                export_data_t *exportEntries = gModuleData->module_data[i].export_entries;
                for (uint32_t j = 0; j < EXPORT_ENTRY_LIST_LENGTH; j++) {
                    if (functionName.compare(exportEntries[j].name) == 0) {
                        functionAddress = (uint32_t) exportEntries[j].address;
                    }
                }
            }
        }

        if ((functionAddress == 0) && replaceAllocFunctions) {
            if (functionName.compare("MEMAllocFromDefaultHeap") == 0) {
                OSDynLoad_Module rplHandle;
                OSDynLoad_Acquire("homebrew_memorymapping", &rplHandle);
                OSDynLoad_FindExport(rplHandle, 1, "MEMAllocFromMappedMemory", (void **) &functionAddress);

            } else if (functionName.compare("MEMAllocFromDefaultHeapEx") == 0) {
                OSDynLoad_Module rplHandle;
                OSDynLoad_Acquire("homebrew_memorymapping", &rplHandle);
                OSDynLoad_FindExport(rplHandle, 1, "MEMAllocFromMappedMemoryEx", (void **) &functionAddress);
            } else if (functionName.compare("MEMFreeToDefaultHeap") == 0) {
                OSDynLoad_Module rplHandle;
                OSDynLoad_Acquire("homebrew_memorymapping", &rplHandle);
                OSDynLoad_FindExport(rplHandle, 1, "MEMFreeToMappedMemory", (void **) &functionAddress);
            }
            if (functionAddress != 0) {
                // DEBUG_FUNCTION_LINE("Using memorymapping function %08X %08X\n", functionAddress, *((uint32_t*)functionAddress));
            }
        }
        if (functionAddress == 0) {
            int32_t isData = curReloc.getImportRPLInformation().isData();
            OSDynLoad_Module rplHandle = 0;
            if (moduleCache.count(rplName) == 0) {
                OSDynLoad_Acquire(rplName.c_str(), &rplHandle);
                moduleCache[rplName] = rplHandle;
            }
            rplHandle = moduleCache.at(rplName);

            OSDynLoad_FindExport(rplHandle, isData, functionName.c_str(), (void **) &functionAddress);
            if (functionAddress == 0) {
                OSFatal_printf("Failed to find export %s of %s", functionName.c_str(), rplName.c_str());
                return false;
            }
        }
        if (!ElfUtils::elfLinkOne(curReloc.getType(), curReloc.getOffset(), curReloc.getAddend(), (uint32_t) curReloc.getDestination(), functionAddress, tramp_data, tramp_length, RELOC_TYPE_IMPORT)) {
            DEBUG_FUNCTION_LINE("Relocation failed\n");
            return false;
        }
    }

    DCFlushRange(tramp_data, tramp_length * sizeof(relocation_trampolin_entry_t));
    ICInvalidateRange(tramp_data, tramp_length * sizeof(relocation_trampolin_entry_t));
    return true;
}

bool ResolveRelocations(const std::vector<ModuleData> &loadedModules, bool replaceAllocFunctions) {
    bool wasSuccessful = true;

    for (auto const &curModule : loadedModules) {
        DEBUG_FUNCTION_LINE("Let's do the relocations for %s\n", curModule.getExportName().c_str());
        if (wasSuccessful) {
            std::vector<RelocationData> relocData = curModule.getRelocationDataList();
            bool replaceAlloc = replaceAllocFunctions;
            if (replaceAlloc) {
                if (curModule.getExportName().compare("homebrew_memorymapping") == 0) {
                    DEBUG_FUNCTION_LINE("Skip malloc replacement for mapping\n");
                    replaceAlloc = false;
                }
            }
            if (!doRelocation(relocData, gModuleData->trampolines, DYN_LINK_TRAMPOLIN_LIST_LENGTH, replaceAlloc)) {
                DEBUG_FUNCTION_LINE("FAIL\n");
                wasSuccessful = false;
            }
        }
        if (curModule.getBSSAddr() != 0) {
            // DEBUG_FUNCTION_LINE("memset .bss %08X (%d)\n", curModule.getBSSAddr(), curModule.getBSSSize());
            // memset((void *) curModule.getBSSAddr(), 0, curModule.getBSSSize());
        }
        if (curModule.getSBSSAddr() != 0) {
            // DEBUG_FUNCTION_LINE("memset .sbss %08X (%d)\n", curModule.getSBSSAddr(), curModule.getSBSSSize());
            // memset((void *) curModule.getSBSSAddr(), 0, curModule.getSBSSSize());
        }
    }
    DCFlushRange((void *) 0x00800000, 0x00800000);
    ICInvalidateRange((void *) 0x00800000, 0x00800000);
    return wasSuccessful;
}

extern "C" void doStart(int argc, char **argv) {
    if (!gFunctionsPatched) {
        gFunctionsPatched = 1;
    }
    DEBUG_FUNCTION_LINE("Loading module data\n");
    std::vector<ModuleData> loadedModulesUnordered = ModuleDataPersistence::loadModuleData(gModuleData);
    std::vector<ModuleData> loadedModules = OrderModulesByDependencies(loadedModulesUnordered);

    DEBUG_FUNCTION_LINE("Number of modules %d\n", gModuleData->number_used_modules);
    if (!gInitCalled) {
        gInitCalled = 1;

        DEBUG_FUNCTION_LINE("Resolve relocations without replacing alloc functions\n");
        ResolveRelocations(loadedModules, false);

        DEBUG_FUNCTION_LINE("Try to call kernel init\n");
        // Call init hook of kernel
        for (auto &curModule : loadedModules) {
            if (curModule.isInitBeforeEntrypoint()) {
                CallHook(curModule, WUMS_HOOK_INIT);
            }
        }

        DEBUG_FUNCTION_LINE("Resolve relocations and replace alloc functions\n");
        ResolveRelocations(loadedModules, true);

        for (int i = 0; i < gModuleData->number_used_modules; i++) {
            DEBUG_FUNCTION_LINE("About to call %08X\n", gModuleData->module_data[i].entrypoint);
            int ret = ((int (*)(int, char **)) (gModuleData->module_data[i].entrypoint))(argc, argv);
            DEBUG_FUNCTION_LINE("return code was %d\n", ret);
        }

        for (auto &curModule : loadedModules) {
            if (!curModule.isInitBeforeEntrypoint()) {
                CallHook(curModule, WUMS_HOOK_INIT);
            }
        }
    } else {
        DEBUG_FUNCTION_LINE("Resolve relocations and replace alloc functions\n");
        ResolveRelocations(loadedModules, true);
    }

    // CallHook(loadedModules, WUMS_HOOK_FINI_WUT);
    // CallHook(loadedModules, WUMS_HOOK_INIT_WUT);

    CallHook(loadedModules, WUMS_HOOK_APPLICATION_STARTS);
}

std::vector<ModuleData> OrderModulesByDependencies(const std::vector<ModuleData> &loadedModules) {
    std::vector<ModuleData> finalOrder;
    std::vector<std::string> loadedModulesExportNames;
    std::vector<uint32_t> loadedModulesEntrypoints;

    while (true) {
        bool canBreak = true;
        bool weDidSomething = false;
        for (auto const &curModule : loadedModules) {
            if (std::find(loadedModulesEntrypoints.begin(), loadedModulesEntrypoints.end(), curModule.getEntrypoint()) != loadedModulesEntrypoints.end()) {
                DEBUG_FUNCTION_LINE("%s [%08X] is already loaded\n", curModule.getExportName().c_str(), curModule.getEntrypoint());
                continue;
            }
            canBreak = false;
            DEBUG_FUNCTION_LINE("Missing %s\n", curModule.getExportName().c_str());
            std::vector<std::string> importsFromOtherModules;
            for (auto curReloc: curModule.getRelocationDataList()) {
                std::string curRPL = curReloc.getImportRPLInformation().getName();
                if (curRPL.rfind("homebrew", 0) == 0) {
                    if (std::find(importsFromOtherModules.begin(), importsFromOtherModules.end(), curRPL) != importsFromOtherModules.end()) {
                        // is already in vector
                    } else {
                        DEBUG_FUNCTION_LINE("%s is importing from %s\n", curModule.getExportName().c_str(), curRPL.c_str());
                        importsFromOtherModules.push_back(curRPL);
                    }
                }
            }
            bool canLoad = true;
            for (auto &curImportRPL : importsFromOtherModules) {
                if (std::find(loadedModulesExportNames.begin(), loadedModulesExportNames.end(), curImportRPL) != loadedModulesExportNames.end()) {

                } else {
                    DEBUG_FUNCTION_LINE("We can't load the module, because %s is not loaded yet\n", curImportRPL.c_str());
                    canLoad = false;
                    break;
                }
            }
            if (canLoad) {
                weDidSomething = true;
                DEBUG_FUNCTION_LINE("############## load %s\n", curModule.getExportName().c_str());
                finalOrder.push_back(curModule);
                loadedModulesExportNames.push_back(curModule.getExportName());
                loadedModulesEntrypoints.push_back(curModule.getEntrypoint());
            }
        }
        if (canBreak) {
            break;
        } else if (!weDidSomething) {
            OSFatal_printf("Failed to resolve dependencies.");
        }
    }
    return finalOrder;
}