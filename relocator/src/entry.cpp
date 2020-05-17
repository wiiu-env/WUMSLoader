#include <vector>
#include <string>
#include <string.h>
#include <stdint.h>
#include <coreinit/dynload.h>
#include <coreinit/cache.h>
#include <nsysnet/socket.h>
#include <coreinit/memorymap.h>
#include <map>
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

#define gModuleData ((module_information_t *) (0x00880000))

uint8_t gFunctionsPatched __attribute__((section(".data"))) = 0;

extern "C" void doStart(int argc, char **argv);
// We need to wrap it to make sure the main function is called AFTER our code.
// The compiler tries to optimize this otherwise and calling the main function earlier
extern "C" int _start(int argc, char **argv) {
    InitFunctionPointers();
    socket_lib_init();
    log_init();

    doStart(argc,argv);

    DEBUG_FUNCTION_LINE("Call real one\n");
    return ( (int (*)(int, char **))(*(unsigned int*)0x1005E040) )(argc, argv);
}

bool doRelocation(std::vector<RelocationData> &relocData, relocation_trampolin_entry_t * tramp_data, uint32_t tramp_length) {
    std::map<std::string,OSDynLoad_Module> moduleCache;
    for (auto const& curReloc : relocData) {
        std::string functionName = curReloc.getName();
        std::string rplName = curReloc.getImportRPLInformation().getName();
        int32_t isData = curReloc.getImportRPLInformation().isData();
        OSDynLoad_Module rplHandle = 0;
        if(moduleCache.count(rplName) == 0){
            OSDynLoad_Acquire(rplName.c_str(), &rplHandle);
            moduleCache[rplName] = rplHandle;
        }
        rplHandle = moduleCache.at(rplName);
        uint32_t functionAddress = 0;
        OSDynLoad_FindExport(rplHandle, isData, functionName.c_str(), (void**) &functionAddress);
        if(functionAddress == 0) {
            DEBUG_FUNCTION_LINE("Failed to find function\n");
            return false;
        }
        if(!ElfUtils::elfLinkOne(curReloc.getType(), curReloc.getOffset(), curReloc.getAddend(), (uint32_t) curReloc.getDestination(), functionAddress, tramp_data, tramp_length, RELOC_TYPE_IMPORT)) {
            DEBUG_FUNCTION_LINE("Relocation failed\n");
            return false;
        }
    }

    DCFlushRange(tramp_data, tramp_length * sizeof(relocation_trampolin_entry_t));
    ICInvalidateRange(tramp_data, tramp_length * sizeof(relocation_trampolin_entry_t));
    return true;
}
bool ResolveRelocations() {
    std::vector<ModuleData> loadedModules = ModuleDataPersistence::loadModuleData(gModuleData);
    bool wasSuccessful = true;
    uint32_t count = 0;
    for (auto const& curModule : loadedModules) {
        if(wasSuccessful) {
            std::vector<RelocationData> relocData = curModule.getRelocationDataList();
            if(!doRelocation(relocData, gModuleData->trampolines,DYN_LINK_TRAMPOLIN_LIST_LENGTH)) {
                DEBUG_FUNCTION_LINE("FAIL\n");
                wasSuccessful = false;
            }
        }
        if(curModule.getBSSAddr() != 0){
            DEBUG_FUNCTION_LINE("memset .bss %08X (%d)\n", curModule.getBSSAddr(), curModule.getBSSSize());
            memset((void*)curModule.getBSSAddr(), 0, curModule.getBSSSize());
        }
        if(curModule.getSBSSAddr() != 0){
            DEBUG_FUNCTION_LINE("memset .sbss %08X (%d)\n", curModule.getSBSSAddr(), curModule.getSBSSSize());
            memset((void*)curModule.getSBSSAddr(), 0, curModule.getSBSSSize());
        }
    }
    if(count > 0) {
        DCFlushRange((void*) 0x00800000, 0x00800000);
        ICInvalidateRange((void*) 0x00800000, 0x00800000);
    }
    return wasSuccessful;
}

extern "C" void doStart(int argc, char **argv) {
    if(!gFunctionsPatched){
        gFunctionsPatched = 1;
        kernelInitialize();
        PatchInvidualMethodHooks(method_hooks_hooks_static, method_hooks_size_hooks_static, method_calls_hooks_static);
    }
    DEBUG_FUNCTION_LINE("Resolve relocations\n");
    DEBUG_FUNCTION_LINE("Number of modules %d\n", gModuleData->number_used_modules);
    ResolveRelocations();
    for(int i = 0; i<gModuleData->number_used_modules; i++) {
        DEBUG_FUNCTION_LINE("About to call %08X\n",gModuleData->module_data[i].entrypoint);
        int ret = ( (int (*)(int, char **))(gModuleData->module_data[i].entrypoint) )(argc, argv);
        DEBUG_FUNCTION_LINE("return code was %d\n", ret);
    }
}
