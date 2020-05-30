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
#include "../../source/common/dynamic_linking_defines.h"
#include "../../source/common/module_defines.h"
#include "../../source/module/RelocationData.h"
#include "../../source/module/ModuleData.h"
#include "ModuleDataPersistence.h"
#include "ElfUtils.h"
#include "../../source/common/relocation_defines.h"
#include "kernel/kernel_utils.h"
#include "hooks_patcher_static.h"

#include "utils/logger.h"
#include "utils/dynamic.h"
#include "globals.h"

#define gModuleData ((module_information_t *) (0x00880000))

uint8_t gFunctionsPatched __attribute__((section(".data"))) = 0;
uint8_t gInitCalled __attribute__((section(".data"))) = 0;

void CallInitHook(const std::vector<ModuleData> &loadedModules);

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

bool doRelocation(std::vector<RelocationData> &relocData, relocation_trampolin_entry_t *tramp_data, uint32_t tramp_length) {
    std::map<std::string, OSDynLoad_Module> moduleCache;
    for (auto const &curReloc : relocData) {
        std::string functionName = curReloc.getName();
        std::string rplName = curReloc.getImportRPLInformation().getName();
        int32_t isData = curReloc.getImportRPLInformation().isData();
        OSDynLoad_Module rplHandle = 0;
        if (moduleCache.count(rplName) == 0) {
            OSDynLoad_Acquire(rplName.c_str(), &rplHandle);
            moduleCache[rplName] = rplHandle;
        }
        rplHandle = moduleCache.at(rplName);
        uint32_t functionAddress = 0;

        OSDynLoad_FindExport(rplHandle, isData, functionName.c_str(), (void **) &functionAddress);
        if (functionAddress == 0) {
            OSFatal_printf("Failed to find export %s of %s", functionName.c_str(), rplName.c_str());
            return false;
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

bool ResolveRelocations(const std::vector<ModuleData> &loadedModules) {
    bool wasSuccessful = true;
    uint32_t count = 0;
    for (auto const &curModule : loadedModules) {
        if (wasSuccessful) {
            std::vector<RelocationData> relocData = curModule.getRelocationDataList();
            if (!doRelocation(relocData, gModuleData->trampolines, DYN_LINK_TRAMPOLIN_LIST_LENGTH)) {
                DEBUG_FUNCTION_LINE("FAIL\n");
                wasSuccessful = false;
            }
        }
        if (curModule.getBSSAddr() != 0) {
            DEBUG_FUNCTION_LINE("memset .bss %08X (%d)\n", curModule.getBSSAddr(), curModule.getBSSSize());
            memset((void *) curModule.getBSSAddr(), 0, curModule.getBSSSize());
        }
        if (curModule.getSBSSAddr() != 0) {
            DEBUG_FUNCTION_LINE("memset .sbss %08X (%d)\n", curModule.getSBSSAddr(), curModule.getSBSSSize());
            memset((void *) curModule.getSBSSAddr(), 0, curModule.getSBSSSize());
        }
    }
    DCFlushRange((void *) 0x00800000, 0x00800000);
    ICInvalidateRange((void *) 0x00800000, 0x00800000);
    return wasSuccessful;
}

extern "C" void doStart(int argc, char **argv) {
    if (!gFunctionsPatched) {
        gFunctionsPatched = 1;
        kernelInitialize();
        PatchInvidualMethodHooks(method_hooks_hooks_static, method_hooks_size_hooks_static, method_calls_hooks_static);
    }
    DEBUG_FUNCTION_LINE("Loading module data\n");
    std::vector<ModuleData> loadedModules = ModuleDataPersistence::loadModuleData(gModuleData);

    DEBUG_FUNCTION_LINE("Resolve relocations\n");
    DEBUG_FUNCTION_LINE("Number of modules %d\n", gModuleData->number_used_modules);
    ResolveRelocations(loadedModules);

    if (!gInitCalled) {
        gInitCalled = 1;
        CallInitHook(loadedModules);
        for(auto&  curModule : loadedModules){
            if(curModule.getExportName().compare("homebrew_memorymapping") == 0){
                for(auto& curExport : curModule.getExportDataList()){
                    if(curExport.getName().compare("MemoryMappingEffectiveToPhysical") == 0){
                        DEBUG_FUNCTION_LINE("Setting MemoryMappingEffectiveToPhysicalPTR to %08X\n", curExport.getAddress());
                        MemoryMappingEffectiveToPhysicalPTR = (uint32_t) curExport.getAddress();
                    }else if(curExport.getName().compare("MemoryMappingPhysicalToEffective") == 0){
                        DEBUG_FUNCTION_LINE("Setting MemoryMappingPhysicalToEffectivePTR to %08X\n", curExport.getAddress());
                        MemoryMappingPhysicalToEffectivePTR = (uint32_t) curExport.getAddress();
                    }
                }
                break;
            }
        }
    }

    for (int i = 0; i < gModuleData->number_used_modules; i++) {
        if(strcmp(gModuleData->module_data[i].module_export_name, "homebrew_memorymapping") == 0){
            DEBUG_FUNCTION_LINE("About to call memory mapping (%08X)\n", gModuleData->module_data[i].entrypoint);
            int ret = ((int (*)(int, char **)) (gModuleData->module_data[i].entrypoint))(argc, argv);
            DEBUG_FUNCTION_LINE("return code was %d\n", ret);
            break;
        }
    }

    for (int i = 0; i < gModuleData->number_used_modules; i++) {
        if(strcmp(gModuleData->module_data[i].module_export_name, "homebrew_memorymapping") != 0) {
            DEBUG_FUNCTION_LINE("About to call %08X\n", gModuleData->module_data[i].entrypoint);
            int ret = ((int (*)(int, char **)) (gModuleData->module_data[i].entrypoint))(argc, argv);
            DEBUG_FUNCTION_LINE("return code was %d\n", ret);
        }
    }
}

void CallInitHook(const std::vector<ModuleData> &loadedModules) {
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
                DEBUG_FUNCTION_LINE("We can load %s\n", curModule.getExportName().c_str());
                for (auto &curHook : curModule.getHookDataList()) {
                    if (curHook.getType() == WUMS_HOOK_INIT) {
                        uint32_t func_ptr = (uint32_t) curHook.getTarget();
                        if (func_ptr == NULL) {
                            DEBUG_FUNCTION_LINE("Hook ptr was NULL\n");
                        } else {
                            DEBUG_FUNCTION_LINE("Calling init of %s\n", curModule.getExportName().c_str());
                            ((void (*)(void)) ((uint32_t *) func_ptr))();
                        }
                    }
                }
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
}
