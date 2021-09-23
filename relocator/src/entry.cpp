#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <coreinit/dynload.h>
#include <coreinit/cache.h>
#include <map>
#include <algorithm>
#include <coreinit/memexpheap.h>
#include "../../source/module/RelocationData.h"
#include "../../source/module/ModuleData.h"
#include "ModuleDataPersistence.h"
#include "ElfUtils.h"
#include "utils/dynamic.h"
#include "globals.h"
#include "hooks.h"
#include "utils/memory.h"

MEMHeapHandle gHeapHandle __attribute__((section(".data"))) = nullptr;
uint8_t gFunctionsPatched __attribute__((section(".data"))) = 0;
uint8_t gInitCalled __attribute__((section(".data"))) = 0;

extern "C" void socket_lib_init();

std::vector<ModuleData> OrderModulesByDependencies(const std::vector<ModuleData> &loadedModules);

extern "C" void doStart(int argc, char **argv);
// We need to wrap it to make sure the main function is called AFTER our code.
// The compiler tries to optimize this otherwise and calling the main function earlier
extern "C" int _start(int argc, char **argv) {
    InitFunctionPointers();

    static uint8_t ucSetupRequired = 1;
    if (ucSetupRequired) {
        gHeapHandle = MEMCreateExpHeapEx((void *) (MEMORY_REGION_USABLE_HEAP_START), MEMORY_REGION_USABLE_HEAP_END - MEMORY_REGION_USABLE_HEAP_START, 0);
        ucSetupRequired = 0;
    }

    socket_lib_init();
    log_init();

    doStart(argc, argv);

    DEBUG_FUNCTION_LINE_VERBOSE("Call real one\n");
    log_deinit();

    return ((int (*)(int, char **)) (*(unsigned int *) 0x1005E040))(argc, argv);
}

bool doRelocation(std::vector<RelocationData> &relocData, relocation_trampolin_entry_t *tramp_data, uint32_t tramp_length, bool skipAllocReplacement) {
    std::map<std::string, OSDynLoad_Module> moduleCache;
    for (auto const &curReloc: relocData) {
        std::string functionName = curReloc.getName();
        std::string rplName = curReloc.getImportRPLInformation().getName();
        uint32_t functionAddress = 0;

        for (uint32_t i = 0; i < MAXIMUM_MODULES; i++) {
            if (rplName == gModuleData->module_data[i].module_export_name) {
                export_data_t *exportEntries = gModuleData->module_data[i].export_entries;
                for (uint32_t j = 0; j < EXPORT_ENTRY_LIST_LENGTH; j++) {
                    if (functionName == exportEntries[j].name) {
                        functionAddress = (uint32_t) exportEntries[j].address;
                    }
                }
            }
        }

        if (!skipAllocReplacement) {
            if (functionName == "MEMAllocFromDefaultHeap") {
                functionAddress = reinterpret_cast<uint32_t>(&MEMAlloc);
            } else if (functionName == "MEMAllocFromDefaultHeapEx") {
                functionAddress = reinterpret_cast<uint32_t>(&MEMAllocEx);
            } else if (functionName == "MEMFreeToDefaultHeap") {
                functionAddress = reinterpret_cast<uint32_t>(&MEMFree);
            }
        }

        if (functionAddress == 0) {
            int32_t isData = curReloc.getImportRPLInformation().isData();
            OSDynLoad_Module rplHandle = nullptr;
            if (moduleCache.count(rplName) == 0) {
                OSDynLoad_Error err = OSDynLoad_IsModuleLoaded(rplName.c_str(), &rplHandle);
                if (err != OS_DYNLOAD_OK || rplHandle == nullptr) {
                    DEBUG_FUNCTION_LINE_VERBOSE("%s is not yet loaded\n", rplName.c_str());
                    // only acquire if not already loaded.
                    err = OSDynLoad_Acquire(rplName.c_str(), &rplHandle);
                    if (err != OS_DYNLOAD_OK) {
                        DEBUG_FUNCTION_LINE("Failed to acquire %s\n", rplName.c_str());
                        //return false;
                    }
                }
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

bool ResolveRelocations(std::vector<ModuleData> &loadedModules, bool skipMemoryMappingModule) {
    bool wasSuccessful = true;

    for (auto &curModule: loadedModules) {
        DEBUG_FUNCTION_LINE_VERBOSE("Let's do the relocations for %s\n", curModule.getExportName().c_str());
        if (wasSuccessful) {
            std::vector<RelocationData> relocData = curModule.getRelocationDataList();

            // On first usage we can't redirect the alloc functions to our custom heap
            // because threads can't run it on it. In order to patch the kernel
            // to fully support our memory region, we have to run the FunctionPatcher and MemoryMapping
            // once with the default heap. Afterwards we can just rely on the custom heap.
            bool skipAllocFunction = skipMemoryMappingModule && (curModule.getExportName() == "homebrew_memorymapping" || curModule.getExportName() == "homebrew_functionpatcher");
            DEBUG_FUNCTION_LINE_VERBOSE("Skip alloc replace? %d\n", skipAllocFunction);
            if (!doRelocation(relocData, gModuleData->trampolines, DYN_LINK_TRAMPOLIN_LIST_LENGTH, skipAllocFunction)) {
                DEBUG_FUNCTION_LINE("FAIL\n");
                wasSuccessful = false;
                curModule.relocationsDone = false;
            }
            curModule.relocationsDone = true;

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
    DCFlushRange((void *) MEMORY_REGION_START, MEMORY_REGION_SIZE);
    ICInvalidateRange((void *) MEMORY_REGION_START, MEMORY_REGION_SIZE);
    return wasSuccessful;
}

extern "C" void doStart(int argc, char **argv) {
    if (!gFunctionsPatched) {
        gFunctionsPatched = 1;
    }
    DEBUG_FUNCTION_LINE("Loading module data\n");
    std::vector<ModuleData> loadedModulesUnordered = ModuleDataPersistence::loadModuleData(gModuleData);
    std::vector<ModuleData> loadedModules = OrderModulesByDependencies(loadedModulesUnordered);

    bool applicationEndHookLoaded = false;
    for (auto &curModule: loadedModules) {
        if (curModule.getExportName() == "homebrew_applicationendshook") {
            DEBUG_FUNCTION_LINE_VERBOSE("We have ApplicationEndsHook Module!\n");
            applicationEndHookLoaded = true;
            break;
        }
    }

    // Make sure WUMS_HOOK_APPLICATION_ENDS and WUMS_HOOK_FINI_WUT are called
    for (auto &curModule: loadedModules) {
        for (auto &curHook: curModule.getHookDataList()) {
            if (curHook.getType() == WUMS_HOOK_APPLICATION_ENDS || curHook.getType() == WUMS_HOOK_FINI_WUT_DEVOPTAB) {
                if (!applicationEndHookLoaded) {
                    OSFatal_printf("%s requires module homebrew_applicationendshook", curModule.getExportName().c_str());
                }
            }
        }
    }

    DEBUG_FUNCTION_LINE_VERBOSE("Number of modules %d\n", gModuleData->number_used_modules);
    if (!gInitCalled) {
        gInitCalled = 1;

        DEBUG_FUNCTION_LINE_VERBOSE("Resolve relocations without replacing alloc functions\n");
        ResolveRelocations(loadedModules, true);

        for (auto &curModule: loadedModules) {
            if (curModule.isInitBeforeRelocationDoneHook()) {
                CallHook(curModule, WUMS_HOOK_INIT_WUT_MALLOC);
                CallHook(curModule, WUMS_HOOK_INIT_WUT_NEWLIB);
                CallHook(curModule, WUMS_HOOK_INIT_WUT_STDCPP);
                CallHook(curModule, WUMS_HOOK_INIT_WUT_DEVOPTAB);
                CallHook(curModule, WUMS_HOOK_INIT_WUT_SOCKETS);
                CallHook(curModule, WUMS_HOOK_INIT);
                CallHook(curModule, WUMS_HOOK_FINI_WUT_SOCKETS);
                CallHook(curModule, WUMS_HOOK_FINI_WUT_DEVOPTAB);
                CallHook(curModule, WUMS_HOOK_FINI_WUT_STDCPP);
                CallHook(curModule, WUMS_HOOK_FINI_WUT_NEWLIB);
                CallHook(curModule, WUMS_HOOK_FINI_WUT_MALLOC);
            }
        }

        DEBUG_FUNCTION_LINE_VERBOSE("Relocations done\n");
        CallHook(loadedModules, WUMS_HOOK_RELOCATIONS_DONE);


        for (int i = 0; i < gModuleData->number_used_modules; i++) {
            if (!gModuleData->module_data[i].skipEntrypoint) {
                DEBUG_FUNCTION_LINE_VERBOSE("About to call %08X\n", gModuleData->module_data[i].entrypoint);
                int ret = ((int (*)(int, char **)) (gModuleData->module_data[i].entrypoint))(argc, argv);
                DEBUG_FUNCTION_LINE_VERBOSE("return code was %d\n", ret);
            }
        }

        for (auto &curModule: loadedModules) {
            if (!curModule.isInitBeforeRelocationDoneHook()) {
                CallHook(curModule, WUMS_HOOK_INIT_WUT_MALLOC);
                CallHook(curModule, WUMS_HOOK_INIT_WUT_NEWLIB);
                CallHook(curModule, WUMS_HOOK_INIT_WUT_STDCPP);
                CallHook(curModule, WUMS_HOOK_INIT_WUT_DEVOPTAB);
                CallHook(curModule, WUMS_HOOK_INIT_WUT_SOCKETS);
                CallHook(curModule, WUMS_HOOK_INIT);
                CallHook(curModule, WUMS_HOOK_FINI_WUT_SOCKETS);
                CallHook(curModule, WUMS_HOOK_FINI_WUT_DEVOPTAB);
                CallHook(curModule, WUMS_HOOK_FINI_WUT_STDCPP);
                CallHook(curModule, WUMS_HOOK_FINI_WUT_NEWLIB);
                CallHook(curModule, WUMS_HOOK_FINI_WUT_MALLOC);
            }
        }
    } else {
        DEBUG_FUNCTION_LINE_VERBOSE("Resolve relocations and replace alloc functions\n");
        ResolveRelocations(loadedModules, false);
        CallHook(loadedModules, WUMS_HOOK_RELOCATIONS_DONE);
    }
    CallHook(loadedModules, WUMS_HOOK_INIT_WUT_MALLOC);
    CallHook(loadedModules, WUMS_HOOK_INIT_WUT_NEWLIB);
    CallHook(loadedModules, WUMS_HOOK_INIT_WUT_STDCPP);
    CallHook(loadedModules, WUMS_HOOK_INIT_WUT_DEVOPTAB);
    CallHook(loadedModules, WUMS_HOOK_INIT_WUT_SOCKETS);
    CallHook(loadedModules, WUMS_HOOK_APPLICATION_STARTS);
    //CallHook(loadedModules, WUMS_HOOK_FINI_WUT);
}

std::vector<ModuleData> OrderModulesByDependencies(const std::vector<ModuleData> &loadedModules) {
    std::vector<ModuleData> finalOrder;
    std::vector<std::string> loadedModulesExportNames;
    std::vector<uint32_t> loadedModulesEntrypoints;

    while (true) {
        bool canBreak = true;
        bool weDidSomething = false;
        for (auto const &curModule: loadedModules) {
            if (std::find(loadedModulesEntrypoints.begin(), loadedModulesEntrypoints.end(), curModule.getEntrypoint()) != loadedModulesEntrypoints.end()) {
                // DEBUG_FUNCTION_LINE("%s [%08X] is already loaded\n", curModule.getExportName().c_str(), curModule.getEntrypoint());
                continue;
            }
            canBreak = false;
            DEBUG_FUNCTION_LINE_VERBOSE("Check if we can load %s\n", curModule.getExportName().c_str());
            std::vector<std::string> importsFromOtherModules;
            for (const auto &curReloc: curModule.getRelocationDataList()) {
                std::string curRPL = curReloc.getImportRPLInformation().getName();
                if (curRPL.rfind("homebrew", 0) == 0) {
                    if (std::find(importsFromOtherModules.begin(), importsFromOtherModules.end(), curRPL) != importsFromOtherModules.end()) {
                        // is already in vector
                    } else {
                        DEBUG_FUNCTION_LINE_VERBOSE("%s is importing from %s\n", curModule.getExportName().c_str(), curRPL.c_str());
                        importsFromOtherModules.push_back(curRPL);
                    }
                }
            }
            bool canLoad = true;
            for (auto &curImportRPL: importsFromOtherModules) {
                if (std::find(loadedModulesExportNames.begin(), loadedModulesExportNames.end(), curImportRPL) != loadedModulesExportNames.end()) {

                } else {
                    DEBUG_FUNCTION_LINE_VERBOSE("We can't load the module, because %s is not loaded yet\n", curImportRPL.c_str());
                    canLoad = false;
                    break;
                }
            }
            if (canLoad) {
                weDidSomething = true;
                DEBUG_FUNCTION_LINE_VERBOSE("We can load: %s\n", curModule.getExportName().c_str());
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