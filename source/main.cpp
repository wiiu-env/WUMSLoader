#include <stdio.h>
#include <string.h>


#include <elfio/elfio.hpp>
#include <proc_ui/procui.h>
#include <sysapp/launch.h>
#include <coreinit/foreground.h>
#include <coreinit/cache.h>
#include <coreinit/cache.h>
#include <coreinit/memorymap.h>
#include <coreinit/dynload.h>
#include <whb/log.h>
#include <whb/log_udp.h>

#include "fs/DirList.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "module/ModuleDataPersistence.h"
#include "module/ModuleDataFactory.h"
#include "ElfUtils.h"
#include "kernel.h"

#include "common/module_defines.h"

bool CheckRunning() {

    switch(ProcUIProcessMessages(true)) {
    case PROCUI_STATUS_EXITING: {
        return false;
    }
    case PROCUI_STATUS_RELEASE_FOREGROUND: {
        ProcUIDrawDoneRelease();
        break;
    }
    case PROCUI_STATUS_IN_FOREGROUND: {
        break;
    }
    case PROCUI_STATUS_IN_BACKGROUND:
    default:
        break;
    }
    return true;
}


#define gModuleData ((module_information_t *) (0x00800000))
static_assert(sizeof(module_information_t) <= 0x80000);

extern "C" uint32_t textStart();

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

int main(int argc, char **argv)  {
    WHBLogUdpInit();

    // 0x100 because before the .text section is a .init section
    // Currently the size of the .init is ~ 0x24 bytes. We substract 0x100 to be safe.
    uint32_t textSectionStart = textStart() - 0x1000;

    *gModuleData = {};

    DirList setupModules("fs:/vol/external01/wiiu/modules/setup", ".rpx", DirList::Files, 1);
    setupModules.SortList();

    for(int i = 0; i < setupModules.GetFilecount(); i++) {
        *gModuleData = {};
        DEBUG_FUNCTION_LINE("Trying to run %s",setupModules.GetFilepath(i));
        ModuleData * moduleData = ModuleDataFactory::load(setupModules.GetFilepath(i), 0x00880000, 0x01000000 - textSectionStart, gModuleData->trampolines, DYN_LINK_TRAMPOLIN_LIST_LENGTH);
        if(moduleData == NULL) {
            DEBUG_FUNCTION_LINE("Failed to load %s", setupModules.GetFilepath(i));
            continue;
        }
        DEBUG_FUNCTION_LINE("Loaded module data");
        std::vector<RelocationData *> relocData = moduleData->getRelocationDataList();
        if(!doRelocation(relocData, gModuleData->trampolines,DYN_LINK_TRAMPOLIN_LIST_LENGTH)) {
            DEBUG_FUNCTION_LINE("relocations failed\n");
        }
        if(moduleData->getBSSAddr() != 0) {
            DEBUG_FUNCTION_LINE("memset .bss %08X (%d)", moduleData->getBSSAddr(), moduleData->getBSSSize());
            memset((void*)moduleData->getBSSAddr(), 0, moduleData->getBSSSize());
        }
        if(moduleData->getSBSSAddr() != 0) {
            DEBUG_FUNCTION_LINE("memset .sbss %08X (%d)", moduleData->getSBSSAddr(), moduleData->getSBSSSize());
            memset((void*)moduleData->getSBSSAddr(), 0, moduleData->getSBSSSize());
        }
        DCFlushRange((void*)0x00800000, 0x00800000);
        ICInvalidateRange((void*)0x00800000, 0x00800000);
        DEBUG_FUNCTION_LINE("Calling %08X", moduleData->getEntrypoint());
        ((int (*)(int, char **))moduleData->getEntrypoint())(argc, argv);
        DEBUG_FUNCTION_LINE("Back from module");
        delete moduleData;
    }

    DirList modules("fs:/vol/external01/wiiu/modules", ".rpx", DirList::Files, 1);
    modules.SortList();

    for(int i = 0; i < modules.GetFilecount(); i++) {
        *gModuleData = {};
        DEBUG_FUNCTION_LINE("Loading module %s",modules.GetFilepath(i));

        ModuleData * moduleData = ModuleDataFactory::load(modules.GetFilepath(i), 0x00880000, 0x01000000 - textSectionStart, gModuleData->trampolines, DYN_LINK_TRAMPOLIN_LIST_LENGTH);

        if(moduleData != NULL) {
            ModuleDataPersistence::saveModuleData(gModuleData, moduleData);
            delete moduleData;
        }
    }

    SetupRelocator();

    WHBLogUdpDeinit();

    ProcUIInit(OSSavesDone_ReadyToRelease);
    SYSLaunchMenu();
    while(CheckRunning()) {
        // wait.
        OSSleepTicks(OSMillisecondsToTicks(100));
    }
    ProcUIShutdown();

    return 0;
}
