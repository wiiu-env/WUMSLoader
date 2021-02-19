#include <wums.h>
#include "hooks.h"
#include "globals.h"

static const char **hook_names = (const char *[]) {
        "WUMS_HOOK_INIT",
        "WUMS_HOOK_APPLICATION_STARTS",
        "WUMS_HOOK_APPLICATION_ENDS",
        "WUMS_HOOK_INIT_WUT",
        "WUMS_HOOK_FINI_WUT",
        "WUMS_HOOK_RELOCATIONS_DONE"};

void CallHook(const std::vector<ModuleData> &modules, wums_hook_type_t type) {
    DEBUG_FUNCTION_LINE("Calling hook of type %s [%d] for all modules\n", hook_names[type], type);
    for (auto &curModule: modules) {
        CallHook(curModule, type);
    }
}

void CallHook(const ModuleData &module, wums_hook_type_t type) {
    if(!module.relocationsDone){
        DEBUG_FUNCTION_LINE("Hook not called because the relocations failed\n");
        return;
    }

    if ((type == WUMS_HOOK_INIT_WUT || type == WUMS_HOOK_FINI_WUT) && module.isSkipWUTInit()) {
        DEBUG_FUNCTION_LINE("Skip WUMS_HOOK_INIT_WUT/WUMS_HOOK_FINI_WUT for %s\n", module.getExportName().c_str());
        return;
    }

    for (auto &curHook : module.getHookDataList()) {
        auto func_ptr = (uint32_t) curHook.getTarget();
        if (func_ptr == 0) {
            DEBUG_FUNCTION_LINE("Hook ptr was NULL\n");
            break;
        }

        if (type == curHook.getType()) {
            if ((type == WUMS_HOOK_APPLICATION_STARTS ||
                 type == WUMS_HOOK_APPLICATION_ENDS ||
                 type == WUMS_HOOK_INIT_WUT ||
                 type == WUMS_HOOK_FINI_WUT)) {
                DEBUG_FUNCTION_LINE("Calling hook of type %s [%d] %d for %s \n", hook_names[type], type, curHook.getType(), module.getExportName().c_str());
                ((void (*)()) ((uint32_t *) func_ptr))();
                break;
            } else if (type == WUMS_HOOK_INIT ||
                       type == WUMS_HOOK_RELOCATIONS_DONE) {
                DEBUG_FUNCTION_LINE("Calling hook of type %s [%d] %d for %s\n", hook_names[type], type, curHook.getType(), module.getExportName().c_str(), gModuleData);
                wums_app_init_args_t args;
                args.module_information = gModuleData;
                ((void (*)(wums_app_init_args_t *)) ((uint32_t *) func_ptr))(&args);
            }
            break;
        }
    }
}