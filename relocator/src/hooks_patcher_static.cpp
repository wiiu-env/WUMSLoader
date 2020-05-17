#include "utils/logger.h"
#include "utils/function_patcher.h"
#include <malloc.h>
#include <coreinit/dynload.h>

DECL(OSDynLoad_Error, OSDynLoad_Acquire, char const *name, OSDynLoad_Module *outModule) {
    DEBUG_FUNCTION_LINE("%s\n", name);
    return real_OSDynLoad_Acquire(name, outModule);
}

DECL(OSDynLoad_Error, OSDynLoad_FindExport, OSDynLoad_Module module, BOOL isData, char const *name, void **outAddr) {
    DEBUG_FUNCTION_LINE("%s\n", name);
    return real_OSDynLoad_FindExport(module,isData, name, outAddr);
}

hooks_magic_t method_hooks_hooks_static[] __attribute__((section(".data"))) = {
        MAKE_MAGIC(OSDynLoad_Acquire,           LIB_CORE_INIT, STATIC_FUNCTION),
        MAKE_MAGIC(OSDynLoad_FindExport,   LIB_CORE_INIT, STATIC_FUNCTION)
};

uint32_t method_hooks_size_hooks_static __attribute__((section(".data"))) = sizeof(method_hooks_hooks_static) / sizeof(hooks_magic_t);

//! buffer to store our instructions needed for our replacements
volatile uint32_t method_calls_hooks_static[sizeof(method_hooks_hooks_static) / sizeof(hooks_magic_t) * FUNCTION_PATCHER_METHOD_STORE_SIZE] __attribute__((section(".data")));

