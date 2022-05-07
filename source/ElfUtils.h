#pragma once

#include <cstdint>
#include <cstdio>

uint32_t load_loader_elf(unsigned char *baseAddress, char *elf_data, uint32_t fileSize);
