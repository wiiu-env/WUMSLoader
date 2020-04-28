#include <vector>
#include <string>
#include <string.h>
#include <stdint.h>
#include <coreinit/dynload.h>
#include <coreinit/cache.h>
#include <nsysnet/socket.h>
#include "common/dynamic_linking_defines.h"
#include "common/module_defines.h"
#include "RelocationData.h"
#include "ModuleData.h"
#include "ModuleDataPersistence.h"
#include "ElfUtils.h"
#include "common/relocation_defines.h"

#include "logger.h"
#include "dynamic.h"

#define gModuleData ((module_information_t *) (0x00800000))


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

bool doRelocation(std::vector<RelocationData *> &relocData, relocation_trampolin_entry_t * tramp_data, uint32_t tramp_length) {
    for (auto const& curReloc : relocData) {
        RelocationData * cur = curReloc;
        std::string functionName = cur->getName();
        std::string rplName = cur->getImportRPLInformation()->getName();
        int32_t isData = cur->getImportRPLInformation()->isData();
        OSDynLoad_Module rplHandle = 0;
        OSDynLoad_Acquire(rplName.c_str(), &rplHandle);

        uint32_t functionAddress = 0;
        OSDynLoad_FindExport(rplHandle, isData, functionName.c_str(), (void**) &functionAddress);
        if(functionAddress == 0) {
            return false;
        }
        if(!ElfUtils::elfLinkOne(cur->getType(), cur->getOffset(), cur->getAddend(), (uint32_t) cur->getDestination(), functionAddress, tramp_data, tramp_length, RELOC_TYPE_IMPORT)) {
            DEBUG_FUNCTION_LINE("Relocation failed\n");
            return false;
        }

    }

    DCFlushRange(tramp_data, tramp_length * sizeof(relocation_trampolin_entry_t));
    ICInvalidateRange(tramp_data, tramp_length * sizeof(relocation_trampolin_entry_t));
    return true;
}
bool ResolveRelocations() {
    std::vector<ModuleData *> loadedModules = ModuleDataPersistence::loadModuleData(gModuleData);
    bool wasSuccessful = true;
    uint32_t count = 0;
    for (auto const& curModule : loadedModules) {
        if(wasSuccessful) {
            std::vector<RelocationData *> relocData = curModule->getRelocationDataList();
            if(!doRelocation(relocData, gModuleData->trampolines,DYN_LINK_TRAMPOLIN_LIST_LENGTH)) {
                DEBUG_FUNCTION_LINE("FAIL\n");
                wasSuccessful = false;
            }
        }
        if(curModule->getBSSAddr() != 0){
            DEBUG_FUNCTION_LINE("memset .bss %08X (%d)\n", curModule->getBSSAddr(), curModule->getBSSSize());
            memset((void*)curModule->getBSSAddr(), 0, curModule->getBSSSize());
        }
        if(curModule->getSBSSAddr() != 0){
            DEBUG_FUNCTION_LINE("memset .sbss %08X (%d)\n", curModule->getSBSSAddr(), curModule->getSBSSSize());
            memset((void*)curModule->getSBSSAddr(), 0, curModule->getSBSSSize());
        }
        delete curModule;
    }
    if(count > 0) {
        DCFlushRange((void*) 0x00800000, 0x00800000);
        ICInvalidateRange((void*) 0x00800000, 0x00800000);
    }
    return wasSuccessful;
}

extern "C" void doStart(int argc, char **argv) {
    ResolveRelocations();

    for(int i = 0; i<gModuleData->number_used_modules; i++) {
        DEBUG_FUNCTION_LINE("About to call %08X\n",gModuleData->module_data[i].entrypoint);
        int ret = ( (int (*)(int, char **))(gModuleData->module_data[i].entrypoint) )(argc, argv);
        DEBUG_FUNCTION_LINE("return code was %d\n", ret);
    }
}
