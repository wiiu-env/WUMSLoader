#include "../../source/common/module_defines.h"
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
    DEBUG_FUNCTION_LINE("Calling hook of type %s [%d]", hook_names[type], type);
    for (auto &curModule: modules) {
        for (auto &curHook : curModule.getHookDataList()) {
            if (curHook.getType() == WUMS_HOOK_INIT ||
                curHook.getType() == WUMS_HOOK_APPLICATION_STARTS ||
                curHook.getType() == WUMS_HOOK_APPLICATION_ENDS ||
                curHook.getType() == WUMS_HOOK_INIT_WUT ||
                curHook.getType() == WUMS_HOOK_FINI_WUT) {
                uint32_t func_ptr = (uint32_t) curHook.getTarget();
                if (func_ptr == 0) {
                    DEBUG_FUNCTION_LINE("Hook ptr was NULL\n");
                } else {
                    DEBUG_FUNCTION_LINE("Calling for module [%s]\n", curModule.getExportName().c_str());
                    ((void (*)(void)) ((uint32_t *) func_ptr))();
                }
            }
        }
    }
}
