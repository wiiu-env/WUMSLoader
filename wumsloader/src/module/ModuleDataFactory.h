/****************************************************************************
 * Copyright (C) 2022 Maschell
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/

#pragma once

#include "ModuleData.h"
#include "elfio/elfio.hpp"
#include <coreinit/memheap.h>
#include <map>
#include <string>
#include <vector>
#include <wums.h>

class ModuleDataFactory {
public:
    static std::optional<std::shared_ptr<ModuleData>> load(const std::string &path);

    static bool linkSection(ELFIO::elfio &reader,
                            uint32_t section_index,
                            uint32_t destination,
                            uint32_t base_text,
                            uint32_t base_data,
                            relocation_trampoline_entry_t *trampoline_data,
                            uint32_t trampoline_data_length);

    static bool getImportRelocationData(std::shared_ptr<ModuleData> &moduleData, ELFIO::elfio &reader, uint8_t **destinations);
};
