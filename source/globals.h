#pragma once
#include <cstdint>
#include <wums/defines/module_defines.h>

#define MEMORY_REGION_START	        0x00800000
#define MEMORY_REGION_SIZE          0x00800000

#define MEMORY_REGION_USABLE_START  MEMORY_REGION_START
#define MEMORY_REGION_USABLE_END    0x00FFF000

#define gModuleData ((module_information_t *) (MEMORY_REGION_USABLE_START))