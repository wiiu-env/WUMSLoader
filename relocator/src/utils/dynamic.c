#include <coreinit/debug.h>
#include <coreinit/dynload.h>

#define IMPORT(name) void *addr_##name
#define IMPORT_BEGIN(lib)
#define IMPORT_END()

#include "imports.h"

#undef IMPORT
#undef IMPORT_BEGIN
#undef IMPORT_END

#define IMPORT(name)                                                                                         \
    do {                                                                                                     \
        if (OSDynLoad_FindExport(handle, 0, #name, &addr_##name) < 0) OSFatal("Function " #name " is NULL"); \
    } while (0)
#define IMPORT_BEGIN(lib)                                                                                        \
    do {                                                                                                         \
        if (OSDynLoad_IsModuleLoaded(#lib ".rpl", &handle) != OS_DYNLOAD_OK) OSFatal(#lib ".rpl is not loaded"); \
    } while (0)
#define IMPORT_END()


void InitFunctionPointers(void) {
    OSDynLoad_Module handle;
    addr_OSDynLoad_Acquire        = (void *) 0x0102A3B4; // 0x0200dfb4 - 0xFE3C00
    addr_OSDynLoad_FindExport     = (void *) 0x0102B828; // 0200f428 - 0xFE3C00
    addr_OSDynLoad_IsModuleLoaded = (void *) 0x0102A59C; // 0200e19c - 0xFE3C00

#include "imports.h"
}
