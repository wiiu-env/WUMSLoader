#include "utils/logger.h"
#include "utils/function_patcher.h"
#include "../../source/common/module_defines.h"
#include <malloc.h>
#include <coreinit/dynload.h>

#define gModuleData ((module_information_t *) (0x00880000))

DECL(OSDynLoad_Error, OSDynLoad_Acquire, char const *name, OSDynLoad_Module *outModule) {
    OSDynLoad_Error result = real_OSDynLoad_Acquire(name, outModule);
    if (result == OS_DYNLOAD_OK) {
        return OS_DYNLOAD_OK;
    }
    DEBUG_FUNCTION_LINE("Looking for module %s\n", name);
    for (uint32_t i = 0; i < MAXIMUM_MODULES; i++) {
        if (strncmp(name, gModuleData->module_data[i].module_export_name, MAXIMUM_EXPORT_MODULE_NAME_LENGTH) == 0) {
            *outModule = (OSDynLoad_Module) (0x13370000 + i);
            return OS_DYNLOAD_OK;
        }
    }
    return result;
}

DECL(OSDynLoad_Error, OSDynLoad_FindExport, OSDynLoad_Module module, BOOL isData, char const *name, void **outAddr) {
    //DEBUG_FUNCTION_LINE("%08X\n", module);
    OSDynLoad_Error result = real_OSDynLoad_FindExport(module, isData, name, outAddr);
    if (result == OS_DYNLOAD_OK) {
        return OS_DYNLOAD_OK;
    }

    DEBUG_FUNCTION_LINE("Looking for %s in handle %d\n", name);
    if (((uint32_t) module & 0xFFFF0000) == 0x13370000) {
        uint32_t modulehandle = ((uint32_t) module) & 0x0000FFFF;
        if (modulehandle > MAXIMUM_MODULES) {
            return result;
        }
        export_data_t *exportEntries = gModuleData->module_data[modulehandle].export_entries;
        for (uint32_t i = 0; i < EXPORT_ENTRY_LIST_LENGTH; i++) {
            if (strncmp(name, exportEntries[i].name, EXPORT_MAXIMUM_NAME_LENGTH) == 0) {
                if (isData && exportEntries[i].type != 1) {
                    return OS_DYNLOAD_INVALID_MODULE_NAME;
                }
                *outAddr = (void *) exportEntries[i].address;
                DEBUG_FUNCTION_LINE("Set outAddr to %08X. It's from module %s function %s\n",
                                    exportEntries[i].address,
                                    gModuleData->module_data[modulehandle].module_export_name,
                                    exportEntries[i].name);
                return OS_DYNLOAD_OK;
            }
        }
    }
    return result;
}

hooks_magic_t method_hooks_hooks_static[] __attribute__((section(".data"))) = {
        MAKE_MAGIC(OSDynLoad_Acquire, LIB_CORE_INIT, STATIC_FUNCTION),
        MAKE_MAGIC(OSDynLoad_FindExport, LIB_CORE_INIT, STATIC_FUNCTION)
};

uint32_t method_hooks_size_hooks_static __attribute__((section(".data"))) = sizeof(method_hooks_hooks_static) / sizeof(hooks_magic_t);

//! buffer to store our instructions needed for our replacements
volatile uint32_t method_calls_hooks_static[sizeof(method_hooks_hooks_static) / sizeof(hooks_magic_t) * FUNCTION_PATCHER_METHOD_STORE_SIZE] __attribute__((section(".data")));
