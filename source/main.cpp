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
