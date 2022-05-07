#pragma once
#include <stdint.h>

int32_t LoadFileToMem(const std::string &filepath, uint8_t **inbuffer, uint32_t *size);