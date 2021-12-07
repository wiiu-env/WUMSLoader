#include <cstring>

#include <elfio/elfio.hpp>
#include <proc_ui/procui.h>
#include <sysapp/launch.h>
#include <coreinit/foreground.h>
#include <coreinit/cache.h>
#include <nn/act/client_cpp.h>
#include <coreinit/dynload.h>
#include <whb/log_udp.h>
#include <whb/log_cafe.h>
#include <whb/log_module.h>
#include <vector>

#include "fs/DirList.h"
#include "module/ModuleDataPersistence.h"
#include "module/ModuleDataFactory.h"
#include "ElfUtils.h"
#include "kernel.h"
#include "globals.h"

bool CheckRunning() {
    switch (ProcUIProcessMessages(true)) {
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

extern "C" uint32_t textStart();

extern "C" void _SYSLaunchMenuWithCheckingAccount(nn::act::SlotNo slot);

bool doRelocation(std::vector<RelocationData> &relocData, relocation_trampolin_entry_t *tramp_data, uint32_t tramp_length) {
    for (auto const &curReloc: relocData) {
        std::string functionName = curReloc.getName();
        std::string rplName = curReloc.getImportRPLInformation().getName();
        int32_t isData = curReloc.getImportRPLInformation().isData();
        OSDynLoad_Module rplHandle = nullptr;

        if (OSDynLoad_IsModuleLoaded(rplName.c_str(), &rplHandle) != OS_DYNLOAD_OK) {
            // only acquire if not already loaded.
            OSDynLoad_Acquire(rplName.c_str(), &rplHandle);
        }

        uint32_t functionAddress = 0;
        OSDynLoad_FindExport(rplHandle, isData, functionName.c_str(), (void **) &functionAddress);
        if (functionAddress == 0) {
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

int main(int argc, char **argv) {
    if (!WHBLogModuleInit()) {
        WHBLogCafeInit();
        WHBLogUdpInit();
    }

    // 0x100 because before the .text section is a .init section
    // Currently the size of the .init is ~ 0x24 bytes. We substract 0x100 to be safe.
    uint32_t textSectionStart = textStart() - 0x1000;

    DirList setupModules("fs:/vol/external01/wiiu/modules/setup", ".rpx", DirList::Files, 1);
    setupModules.SortList();

    for (int i = 0; i < setupModules.GetFilecount(); i++) {
        uint32_t destination_address = ((uint32_t) gModuleData + (sizeof(module_information_t) + 0x0000FFFF)) & 0xFFFF0000;
        memset((void *) gModuleData, 0, sizeof(module_information_t));
        DEBUG_FUNCTION_LINE("Trying to run %s", setupModules.GetFilepath(i));
        std::optional<ModuleData> moduleData = ModuleDataFactory::load(setupModules.GetFilepath(i), &destination_address, textSectionStart - destination_address, gModuleData->trampolines,
                                                                       DYN_LINK_TRAMPOLIN_LIST_LENGTH);
        if (!moduleData) {
            DEBUG_FUNCTION_LINE("Failed to load %s", setupModules.GetFilepath(i));
            continue;
        }
        DEBUG_FUNCTION_LINE("Loaded module data");
        std::vector<RelocationData> relocData = moduleData->getRelocationDataList();
        if (!doRelocation(relocData, gModuleData->trampolines, DYN_LINK_TRAMPOLIN_LIST_LENGTH)) {
            DEBUG_FUNCTION_LINE("relocations failed\n");
        }
        DCFlushRange((void *) moduleData->getStartAddress(), moduleData->getEndAddress() - moduleData->getStartAddress());
        ICInvalidateRange((void *) moduleData->getStartAddress(), moduleData->getEndAddress() - moduleData->getStartAddress());
        DEBUG_FUNCTION_LINE("Calling entrypoint @%08X", moduleData->getEntrypoint());
        ((int (*)(int, char **)) moduleData->getEntrypoint())(argc, argv);
        DEBUG_FUNCTION_LINE("Back from module");
    }

    memset((void *) gModuleData, 0, sizeof(module_information_t));
    gModuleData->version = MODULE_INFORMATION_VERSION;

    DirList modules("fs:/vol/external01/wiiu/modules", ".wms", DirList::Files, 1);
    modules.SortList();

    uint32_t destination_address = ((uint32_t) gModuleData + (sizeof(module_information_t) + 0x0000FFFF)) & 0xFFFF0000;
    for (int i = 0; i < modules.GetFilecount(); i++) {
        DEBUG_FUNCTION_LINE("Loading module %s", modules.GetFilepath(i));
        auto moduleData = ModuleDataFactory::load(modules.GetFilepath(i), &destination_address, MEMORY_REGION_USABLE_END - destination_address, gModuleData->trampolines,
                                                                       DYN_LINK_TRAMPOLIN_LIST_LENGTH);

        if (moduleData) {
            DEBUG_FUNCTION_LINE("Successfully loaded %s", modules.GetFilepath(i));
            ModuleDataPersistence::saveModuleData(gModuleData, moduleData.value());
        } else {
            DEBUG_FUNCTION_LINE("Failed to load %s", modules.GetFilepath(i));
        }
    }

    SetupRelocator();

    WHBLogUdpDeinit();

    ProcUIInit(OSSavesDone_ReadyToRelease);

    nn::act::Initialize();
    nn::act::SlotNo slot = nn::act::GetSlotNo();
    nn::act::SlotNo defaultSlot = nn::act::GetDefaultAccount();
    nn::act::Finalize();

    if (defaultSlot) { //normal menu boot
        SYSLaunchMenu();
    } else { //show mii select
        _SYSLaunchMenuWithCheckingAccount(slot);
    }
    while (CheckRunning()) {
        // wait.
        OSSleepTicks(OSMillisecondsToTicks(100));
    }
    ProcUIShutdown();

    return 0;
}
