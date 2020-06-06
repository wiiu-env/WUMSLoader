#include <wums.h>
#include "hooks.h"
#include "utils/logger.h"

static const char **hook_names = (const char *[]) {
        "WUMS_HOOK_INIT",
        "WUMS_HOOK_APPLICATION_STARTS",
        "WUMS_HOOK_APPLICATION_ENDS",
        "WUMS_HOOK_INIT_WUT",
        "WUMS_HOOK_FINI_WUT"};

void CallHook(const std::vector<ModuleData> &modules, wums_hook_type_t type) {
    DEBUG_FUNCTION_LINE("Calling hook of type %s [%d] for all modules\n", hook_names[type], type);
    for (auto &curModule: modules) {
        CallHook(curModule, type);
    }
}


void CallHook(const ModuleData &module, wums_hook_type_t type) {
    for (auto &curHook : module.getHookDataList()) {
        if ((type == WUMS_HOOK_INIT ||
             type == WUMS_HOOK_APPLICATION_STARTS ||
             type == WUMS_HOOK_APPLICATION_ENDS ||
             type == WUMS_HOOK_INIT_WUT ||
             type == WUMS_HOOK_FINI_WUT) &&
            type == curHook.getType()) {
            uint32_t func_ptr = (uint32_t) curHook.getTarget();
            if (func_ptr == 0) {
                DEBUG_FUNCTION_LINE("Hook ptr was NULL\n");
            } else {
                DEBUG_FUNCTION_LINE("Calling hook of type %s [%d] %d for %s \n", hook_names[type], type, curHook.getType(), module.getExportName().c_str());
                ((void (*)(void)) ((uint32_t *) func_ptr))();
            }
        }
    }
}
