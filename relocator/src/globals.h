#pragma once

#include <cstdint>

extern uint32_t MemoryMappingEffectiveToPhysicalPTR;
extern uint32_t MemoryMappingPhysicalToEffectivePTR;

#define gModuleData ((module_information_t *) (0x00880000))