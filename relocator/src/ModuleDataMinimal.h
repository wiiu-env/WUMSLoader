/****************************************************************************
 * Copyright (C) 2018-2021 Maschell
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

#include "../../source/module/HookData.h"
#include "../../source/module/RelocationData.h"
#include <vector>

#pragma once

class ModuleDataMinimal {
public:
    ModuleDataMinimal() = default;

    ~ModuleDataMinimal() = default;

    void setExportName(const std::string &name) {
        this->export_name = name;
    }

    [[nodiscard]] std::string getExportName() const {
        return this->export_name;
    }

    void addRelocationData(const std::shared_ptr<RelocationData> &relocation_data) {
        relocation_data_list.push_back(relocation_data);
    }

    [[nodiscard]] const std::vector<std::shared_ptr<RelocationData>> &getRelocationDataList() const {
        return relocation_data_list;
    }

    void addHookData(const std::shared_ptr<HookData> &data) {
        hook_data_list.push_back(data);
    }

    [[nodiscard]] const std::vector<std::shared_ptr<HookData>> &getHookDataList() const {
        return hook_data_list;
    }

    void setEntrypoint(uint32_t addr) {
        this->entrypoint = addr;
    }

    [[nodiscard]] uint32_t getEntrypoint() const {
        return entrypoint;
    }

    [[nodiscard]] bool isInitBeforeRelocationDoneHook() const {
        return this->initBeforeRelocationDoneHook;
    }

    void setInitBeforeRelocationDoneHook(bool value) {
        this->initBeforeRelocationDoneHook = value;
    }

    [[nodiscard]] bool isSkipInitFini() const {
        return this->skipInitFini;
    }

    void setSkipInitFini(bool value) {
        this->skipInitFini = value;
    }

    bool relocationsDone = false;

private:
    std::vector<std::shared_ptr<RelocationData>> relocation_data_list;
    std::vector<std::shared_ptr<HookData>> hook_data_list;
    std::string export_name;
    uint32_t entrypoint               = 0;
    bool initBeforeRelocationDoneHook = false;
    bool skipInitFini                 = false;
};
