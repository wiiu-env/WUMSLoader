#include "entry.h"
#include "fs/DirList.h"
#include "globals.h"
#include "module/ModuleDataFactory.h"
#include "module/ModuleDataPersistence.h"
#include "utils/ElfUtils.h"
#include "utils/RelocationUtils.h"
#include "utils/dynamic.h"
#include "utils/hooks.h"
#include "utils/logger.h"
#include "version.h"
#include <coreinit/debug.h>
#include <coreinit/memexpheap.h>
#include <cstdint>

#define VERSION "v0.2"

void CallInitHooksForModule(const std::shared_ptr<ModuleData> &curModule);

std::vector<std::shared_ptr<ModuleData>> OrderModulesByDependencies(const std::vector<std::shared_ptr<ModuleData>> &loadedModules);

// We need to wrap it to make sure the main function is called AFTER our code.
// The compiler tries to optimize this otherwise and calling the main function earlier
extern "C" int _start(int argc, char **argv) {
    InitFunctionPointers();

    static uint8_t ucSetupRequired = 1;
    if (ucSetupRequired) {
        gHeapHandle = MEMCreateExpHeapEx((void *) (MEMORY_REGION_USABLE_HEAP_START), MEMORY_REGION_USABLE_HEAP_END - MEMORY_REGION_USABLE_HEAP_START, 1);
        if (!gHeapHandle) {
            OSFatal("Failed to alloc heap");
        }

        __init();
        ucSetupRequired = 0;
    }

    uint32_t upid = OSGetUPID();
    if (upid == 2 || upid == 15) {
        doStart(argc, argv);
    }

    return ((int (*)(int, char **))(*(unsigned int *) 0x1005E040))(argc, argv);
}

void SaveLoadedRPLsInGlobalInformation(module_information_t *globalInformation,
                                       std::map<std::string, OSDynLoad_Module> &usedRPls) {
    // free previous allocations.
    if (globalInformation->acquired_rpls) {
        free(globalInformation->acquired_rpls);
    }
    globalInformation->number_acquired_rpls = usedRPls.size();
    globalInformation->acquired_rpls        = (uint32_t *) malloc(usedRPls.size() * sizeof(uint32_t));
    if (!globalInformation->acquired_rpls) {
        OSFatal("Failed to allocate memory");
    }
    uint32_t i = 0;
    for (auto &rpl : usedRPls) {
        globalInformation->acquired_rpls[i] = (uint32_t) rpl.second;
        ++i;
    }
}

void doStart(int argc, char **argv) {
    init_wut();
    initLogging();

    OSReport("Running WUMSLoader " VERSION VERSION_EXTRA "\n");

    gUsedRPLs.clear();
    // If an allocated rpl was not released properly (e.g. if something else calls OSDynload_Acquire without releasing it)
    // memory gets leaked. Let's clean this up!
    for (auto &addr : gAllocatedAddresses) {
        DEBUG_FUNCTION_LINE_WARN("Memory allocated by OSDynload was not freed properly, let's clean it up! (%08X)", addr);
        free((void *) addr);
    }
    gAllocatedAddresses.clear();

    if (!gInitCalled) {
        gInitCalled = 1;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overread"
        std::string basePath = ENVRIONMENT_STRING;
#pragma GCC diagnostic pop

        DEBUG_FUNCTION_LINE("We need to load the modules. basePath %s", basePath.c_str());
        DirList modules(basePath + "/modules", ".wms", DirList::Files, 1);
        modules.SortList();
        for (int i = 0; i < modules.GetFilecount(); i++) {
            std::string_view asView(modules.GetFilename(i));
            if (asView.starts_with('.') || asView.starts_with('_')) {
                DEBUG_FUNCTION_LINE_WARN("Skip file %s", modules.GetFilename(i));
                continue;
            }
            DEBUG_FUNCTION_LINE("Loading module %s", modules.GetFilepath(i));
            auto moduleData = ModuleDataFactory::load(modules.GetFilepath(i));
            if (moduleData) {
                DEBUG_FUNCTION_LINE("Successfully loaded %s", modules.GetFilepath(i));
                gLoadedModules.push_back(std::move(moduleData.value()));
            } else {
                DEBUG_FUNCTION_LINE_ERR("Failed to load %s", modules.GetFilepath(i));
            }
        }

        gModuleInformation = {.version = MODULE_INFORMATION_VERSION};
        ModuleDataPersistence::saveModuleData(&gModuleInformation, gLoadedModules);

        auto orderedModules = OrderModulesByDependencies(gLoadedModules);

        // make sure the plugin backend module is at the end.
        auto it = std::find_if(gLoadedModules.begin(),
                               gLoadedModules.end(),
                               [](auto &cur) { return std::string_view(cur->getExportName()) == "homebrew_wupsbackend"; });
        if (it != gLoadedModules.end()) {
            auto module = *it;
            gLoadedModules.erase(it);
            gLoadedModules.push_back(module);
        }

        bool aromaBaseModuleLoaded = false;
        for (auto &curModule : gLoadedModules) {
            if (std::string_view(curModule->getExportName()) == "homebrew_basemodule") {
                DEBUG_FUNCTION_LINE_VERBOSE("We have AromaBaseModule!");
                aromaBaseModuleLoaded = true;
                break;
            }
        }

        // Make sure WUMS_HOOK_APPLICATION_ENDS and WUMS_HOOK_FINI_WUT are called
        for (auto &curModule : gLoadedModules) {
            for (auto &curHook : curModule->getHookDataList()) {
                if (curHook->getType() == WUMS_HOOK_APPLICATION_ENDS || curHook->getType() == WUMS_HOOK_FINI_WUT_DEVOPTAB) {
                    if (!aromaBaseModuleLoaded) {
                        DEBUG_FUNCTION_LINE_ERR("%s requires module homebrew_basemodule", curModule->getExportName().c_str());
                        OSFatal("module requires module homebrew_basemodule");
                    }
                }
            }
        }

        DEBUG_FUNCTION_LINE_VERBOSE("Resolve relocations without replacing alloc functions");
        ResolveRelocations(gLoadedModules, true, gUsedRPLs);

        for (auto &curModule : gLoadedModules) {
            if (curModule->isInitBeforeRelocationDoneHook()) {
                CallInitHooksForModule(curModule);
            }
        }

        DEBUG_FUNCTION_LINE_VERBOSE("Call Relocations done hook");
        CallHook(gLoadedModules, WUMS_HOOK_RELOCATIONS_DONE);

        for (auto &curModule : gLoadedModules) {
            if (!curModule->isInitBeforeRelocationDoneHook()) {
                CallInitHooksForModule(curModule);
            }
        }
    } else {
        DEBUG_FUNCTION_LINE("Resolve relocations and replace alloc functions");
        ResolveRelocations(gLoadedModules, false, gUsedRPLs);
        CallHook(gLoadedModules, WUMS_HOOK_RELOCATIONS_DONE);
    }

    SaveLoadedRPLsInGlobalInformation(&gModuleInformation, gUsedRPLs);

    CallHook(gLoadedModules, WUMS_HOOK_INIT_WUT_DEVOPTAB);
    CallHook(gLoadedModules, WUMS_HOOK_INIT_WUT_SOCKETS);
    CallHook(gLoadedModules, WUMS_HOOK_APPLICATION_STARTS);

    deinitLogging();
    fini_wut();
}

void CallInitHooksForModule(const std::shared_ptr<ModuleData> &curModule) {
    CallHook(curModule, WUMS_HOOK_INIT_WUT_MALLOC);
    CallHook(curModule, WUMS_HOOK_INIT_WUT_NEWLIB);
    CallHook(curModule, WUMS_HOOK_INIT_WUT_STDCPP);
    CallHook(curModule, WUMS_HOOK_INIT_WUT_DEVOPTAB);
    CallHook(curModule, WUMS_HOOK_INIT_WUT_SOCKETS);
    CallHook(curModule, WUMS_HOOK_INIT_WRAPPER, !curModule->isSkipInitFini());
    CallHook(curModule, WUMS_HOOK_INIT);
}

std::vector<std::shared_ptr<ModuleData>> OrderModulesByDependencies(const std::vector<std::shared_ptr<ModuleData>> &loadedModules) {
    std::vector<std::shared_ptr<ModuleData>> finalOrder;
    std::vector<std::string_view> loadedModulesExportNames;
    std::vector<uint32_t> loadedModulesEntrypoints;

    while (true) {
        bool canBreak       = true;
        bool weDidSomething = false;
        for (auto const &curModule : loadedModules) {
            if (std::find(loadedModulesEntrypoints.begin(), loadedModulesEntrypoints.end(), curModule->getEntrypoint()) != loadedModulesEntrypoints.end()) {
                // DEBUG_FUNCTION_LINE("%s [%08X] is already loaded" curModule->getExportName().c_str(), curModule->getEntrypoint());
                continue;
            }
            canBreak = false;
            DEBUG_FUNCTION_LINE_VERBOSE("Check if we can load %s", curModule->getExportName().c_str());
            std::vector<std::string_view> importsFromOtherModules;
            for (const auto &curReloc : curModule->getRelocationDataList()) {
                std::string_view curRPL = curReloc->getImportRPLInformation()->getRPLName();
                if (curRPL == "homebrew_wupsbackend") {
                    OSFatal("Error: module depends on homebrew_wupsbackend, this is not supported");
                }
                if (curRPL.starts_with("homebrew")) {
                    if (std::find(importsFromOtherModules.begin(), importsFromOtherModules.end(), curRPL) == importsFromOtherModules.end()) {
                        DEBUG_FUNCTION_LINE_VERBOSE("%s is importing from %s", curModule->getExportName().c_str(), curRPL.begin());
                        importsFromOtherModules.push_back(curRPL);
                    }
                }
            }
            bool canLoad = true;
            for (auto &curImportRPL : importsFromOtherModules) {
                if (std::find(loadedModulesExportNames.begin(), loadedModulesExportNames.end(), curImportRPL) == loadedModulesExportNames.end()) {
                    DEBUG_FUNCTION_LINE_VERBOSE("We can't load the module, because %s is not loaded yet", curImportRPL.begin());
                    canLoad = false;
                    break;
                }
            }
            if (canLoad) {
                weDidSomething = true;
                DEBUG_FUNCTION_LINE_VERBOSE("We can load: %s", curModule->getExportName().c_str());
                finalOrder.push_back(curModule);
                loadedModulesExportNames.emplace_back(curModule->getExportName());
                loadedModulesEntrypoints.push_back(curModule->getEntrypoint());
            }
        }
        if (canBreak) {
            break;
        } else if (!weDidSomething) {
            OSFatal("Failed to resolve dependencies.");
        }
    }
    return finalOrder;
}