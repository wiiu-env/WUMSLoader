#include <cstring>

#include <elfio/elfio.hpp>
#include <sysapp/launch.h>
#include <nn/act/client_cpp.h>

#include "fs/DirList.h"
#include "module/ModuleDataPersistence.h"
#include "module/ModuleDataFactory.h"
#include "ElfUtils.h"
#include "kernel.h"
#include "globals.h"

extern "C" uint32_t textStart();
extern "C" void __fini();

int main(int argc, char **argv) {
    initLogging();

    // We subtract 0x100 to be safe.
    uint32_t textSectionStart = textStart() - 0x100;

    memset((void *) gModuleData, 0, sizeof(module_information_t));
    gModuleData->version = MODULE_INFORMATION_VERSION;

    std::string basePath = "fs:/vol/external01/wiiu";
    if (argc >= 1) {
        basePath = argv[0];
    }

    DirList modules(basePath + "/modules", ".wms", DirList::Files, 1);
    modules.SortList();

    uint32_t destination_address = ((uint32_t) gModuleData + (sizeof(module_information_t) + 0x0000FFFF)) & 0xFFFF0000;
    for (int i = 0; i < modules.GetFilecount(); i++) {
        DEBUG_FUNCTION_LINE("Loading module %s", modules.GetFilepath(i));
        auto moduleData = ModuleDataFactory::load(modules.GetFilepath(i), &destination_address, textSectionStart - destination_address, gModuleData->trampolines,
                                                  DYN_LINK_TRAMPOLINE_LIST_LENGTH);
        if (moduleData) {
            DEBUG_FUNCTION_LINE("Successfully loaded %s", modules.GetFilepath(i));
            ModuleDataPersistence::saveModuleData(gModuleData, moduleData.value());
        } else {
            DEBUG_FUNCTION_LINE("Failed to load %s", modules.GetFilepath(i));
        }
    }

    DEBUG_FUNCTION_LINE("Setup relocator");
    SetupRelocator();

    nn::act::Initialize();
    nn::act::SlotNo slot = nn::act::GetSlotNo();
    nn::act::SlotNo defaultSlot = nn::act::GetDefaultAccount();
    nn::act::Finalize();

    if (defaultSlot) { //normal menu boot
        SYSLaunchMenu();
    } else { //show mii select
        _SYSLaunchMenuWithCheckingAccount(slot);
    }

    deinitLogging();

    __fini();
    return 0;
}
