#include "../wumsloader/src/globals.h"
#include "kernel.h"
#include "utils/logger.h"
#include <nn/act/client_cpp.h>
#include <sysapp/launch.h>

extern "C" void __fini();
int main(int argc, char **argv) {
    initLogging();

    std::string basePath = "fs:/vol/external01/wiiu";
    if (argc >= 1) {
        basePath = argv[0];
    }
    if (argc < 4 || std::string_view("EnvironmentLoader") != argv[1] || (uint32_t) argv[2] < 2 || (uint32_t) argv[3] == 0) {
        OSFatal("WUMSLoader:\n"
                "Failed to parse arguments! Make sure to use the latest\n"
                "version of the EnvironmentLoader.\n"
                "\n\n"
                "See https://wiiu.hacks.guide/ for instructions on how to update!");
    }

    memcpy(MEMORY_REGION_USABLE_MEM_REGION_END_VALUE_PTR, &argv[3], sizeof(uint32_t));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wstringop-overflow="
    strncpy(ENVRIONMENT_STRING, basePath.c_str(), ENVIRONMENT_PATH_LENGTH - 1);
#pragma GCC diagnostic pop

    DEBUG_FUNCTION_LINE("Setup wumsloader");
    SetupWUMSLoader();

    nn::act::Initialize();
    nn::act::SlotNo slot        = nn::act::GetSlotNo();
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
