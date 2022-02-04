#pragma once

#include <cstdint>
#include <wums/defines/module_defines.h>

#define MEMORY_REGION_START             0x00800000
#define MEMORY_REGION_SIZE              0x00800000

#define MEMORY_REGION_USABLE_HEAP_START (MEMORY_REGION_START + 0x00080000)             // We don't want to override the relocator
#define MEMORY_REGION_USABLE_HEAP_END   (MEMORY_REGION_USABLE_HEAP_START + 0x00080000) // heap size is 512 KiB for now

#define MEMORY_REGION_USABLE_START      MEMORY_REGION_USABLE_HEAP_END
#define MEMORY_REGION_USABLE_END        0x00FFF000 // The last 0x1000 bytes are reserved kernel hook

#define gModuleData                     ((module_information_t *) (MEMORY_REGION_USABLE_START))