#include <wums.h>
#include "hooks.h"
#include "globals.h"

static const char **hook_names = (const char *[]) {
        "WUMS_HOOK_INIT_WUT_MALLOC",
        "WUMS_HOOK_FINI_WUT_MALLOC",
        "WUMS_HOOK_INIT_WUT_NEWLIB",
        "WUMS_HOOK_FINI_WUT_NEWLIB",
        "WUMS_HOOK_INIT_WUT_STDCPP",
        "WUMS_HOOK_FINI_WUT_STDCPP",
        "WUMS_HOOK_INIT_WUT_DEVOPTAB",
        "WUMS_HOOK_FINI_WUT_DEVOPTAB",
        "WUMS_HOOK_INIT_WUT_SOCKETS",
        "WUMS_HOOK_FINI_WUT_SOCKETS",

        "WUMS_HOOK_INIT",
        "WUMS_HOOK_APPLICATION_STARTS",
        "WUMS_HOOK_APPLICATION_ENDS",
        "WUMS_HOOK_RELOCATIONS_DONE",
        "WUMS_HOOK_APPLICATION_REQUESTS_EXI"};

void CallHook(const std::vector<ModuleData> &modules, wums_hook_type_t type) {
    DEBUG_FUNCTION_LINE_VERBOSE("Calling hook of type %s [%d] for all modules\n", hook_names[type], type);
    for (auto &curModule: modules) {
        CallHook(curModule, type);
    }
}

void CallHook(const ModuleData &module, wums_hook_type_t type) {
    if (!module.relocationsDone) {
        DEBUG_FUNCTION_LINE("Hook not called because the relocations failed\n");
        return;
    }

    for (auto &curHook: module.getHookDataList()) {
        auto func_ptr = (uint32_t) curHook.getTarget();
        if (func_ptr == 0) {
            DEBUG_FUNCTION_LINE("Hook ptr was NULL\n");
            break;
        }

        if (type == curHook.getType()) {
            if ((type == WUMS_HOOK_APPLICATION_STARTS ||
                 type == WUMS_HOOK_APPLICATION_ENDS ||
                 type == WUMS_HOOK_INIT_WUT_MALLOC ||
                 type == WUMS_HOOK_FINI_WUT_MALLOC ||
                 type == WUMS_HOOK_INIT_WUT_NEWLIB ||
                 type == WUMS_HOOK_FINI_WUT_NEWLIB ||
                 type == WUMS_HOOK_INIT_WUT_STDCPP ||
                 type == WUMS_HOOK_FINI_WUT_STDCPP ||
                 type == WUMS_HOOK_INIT_WUT_DEVOPTAB ||
                 type == WUMS_HOOK_FINI_WUT_DEVOPTAB ||
                 type == WUMS_HOOK_INIT_WUT_SOCKETS ||
                 type == WUMS_HOOK_FINI_WUT_SOCKETS
            )) {
                DEBUG_FUNCTION_LINE_VERBOSE("Calling hook of type %s [%d] %d for %s: %08X\n", hook_names[type], type, curHook.getType(), module.getExportName().c_str(), curHook.getTarget());
                ((void (*)()) ((uint32_t *) func_ptr))();
                break;
            } else if (type == WUMS_HOOK_INIT ||
                       type == WUMS_HOOK_RELOCATIONS_DONE) {
                DEBUG_FUNCTION_LINE_VERBOSE("Calling hook of type %s [%d] %d for %s: %08X\n", hook_names[type], type, curHook.getType(), module.getExportName().c_str(), curHook.getTarget());
                wums_app_init_args_t args;
                args.module_information = gModuleData;
                ((void (*)(wums_app_init_args_t *)) ((uint32_t *) func_ptr))(&args);
            } else {
                DEBUG_FUNCTION_LINE("#########################################\n");
                DEBUG_FUNCTION_LINE("#########HOOK NOT IMPLEMENTED %d#########\n", type);
                DEBUG_FUNCTION_LINE("#########################################\n");
            }
            break;
        }
    }
}