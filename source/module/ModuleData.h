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

#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include "RelocationData.h"
#include "SectionInfo.h"
#include "ExportData.h"
#include "HookData.h"
#include "FunctionSymbolData.h"

struct FunctionSymbolDataComparator {
    bool operator()(const std::shared_ptr<FunctionSymbolData>& lhs,
                    const std::shared_ptr<FunctionSymbolData>& rhs) const
    {
        return (uint32_t) lhs->getAddress() < (uint32_t) rhs->getAddress();
    }
};

class ModuleData {
public:
    ModuleData() = default;

    ~ModuleData() = default;

    void setBSSLocation(uint32_t addr, uint32_t size) {
        this->bssAddr = addr;
        this->bssSize = size;
    }

    void setSBSSLocation(uint32_t addr, uint32_t size) {
        this->sbssAddr = addr;
        this->sbssSize = size;
    }

    void setEntrypoint(uint32_t addr) {
        this->entrypoint = addr;
    }

    void setStartAddress(uint32_t addr) {
        this->startAddress = addr;
    }

    void setEndAddress(uint32_t _endAddress) {
        this->endAddress = _endAddress;
    }

    void addRelocationData(const std::shared_ptr<RelocationData> &relocation_data) {
        relocation_data_list.push_back(relocation_data);
    }

    [[nodiscard]] const std::vector<std::shared_ptr<RelocationData>> &getRelocationDataList() const {
        return relocation_data_list;
    }

    void addExportData(const std::shared_ptr<ExportData> &data) {
        export_data_list.push_back(data);
    }

    [[nodiscard]] const std::vector<std::shared_ptr<ExportData>> &getExportDataList() const {
        return export_data_list;
    }

    void addHookData(const std::shared_ptr<HookData> &data) {
        hook_data_list.push_back(data);
    }

    [[nodiscard]] const std::vector<std::shared_ptr<HookData>> &getHookDataList() const {
        return hook_data_list;
    }

    void addSectionInfo(const std::shared_ptr<SectionInfo> &sectionInfo) {
        section_info_list[sectionInfo->getName()] = sectionInfo;
    }

    void addFunctionSymbolData(const std::shared_ptr<FunctionSymbolData> &symbol_data) {
        symbol_data_list.insert(symbol_data);
    }

    [[nodiscard]] const std::set<std::shared_ptr<FunctionSymbolData>, FunctionSymbolDataComparator> &getFunctionSymbolDataList() const {
        return symbol_data_list;
    }

    [[nodiscard]] const std::map<std::string, std::shared_ptr<SectionInfo>> &getSectionInfoList() const {
        return section_info_list;
    }

    [[nodiscard]] std::optional<std::shared_ptr<SectionInfo>> getSectionInfo(const std::string &sectionName) const {
        if (getSectionInfoList().count(sectionName) > 0) {
            return section_info_list.at(sectionName);
        }
        return std::nullopt;
    }

    [[nodiscard]] uint32_t getBSSAddr() const {
        return bssAddr;
    }

    [[nodiscard]] uint32_t getBSSSize() const {
        return bssSize;
    }

    [[nodiscard]] uint32_t getSBSSAddr() const {
        return sbssAddr;
    }

    [[nodiscard]] uint32_t getSBSSSize() const {
        return sbssSize;
    }

    [[nodiscard]] uint32_t getEntrypoint() const {
        return entrypoint;
    }

    [[nodiscard]] uint32_t getStartAddress() const {
        return startAddress;
    }

    [[nodiscard]] uint32_t getEndAddress() const {
        return endAddress;
    }

    [[nodiscard]] std::string toString() const;

    void setExportName(const std::string &name) {
        this->export_name = name;
    }

    [[nodiscard]] std::string getExportName() const {
        return this->export_name;
    }

    [[nodiscard]] bool isSkipEntrypoint() const {
        return this->skipEntrypoint;
    }

    [[nodiscard]] bool isInitBeforeRelocationDoneHook() const {
        return this->initBeforeRelocationDoneHook;
    }

    void setSkipEntrypoint(bool value) {
        this->skipEntrypoint = value;
    }

    void setInitBeforeRelocationDoneHook(bool value) {
        this->initBeforeRelocationDoneHook = value;
    }

    bool relocationsDone = false;
private:
    std::vector<std::shared_ptr<RelocationData>> relocation_data_list;
    std::vector<std::shared_ptr<ExportData>> export_data_list;
    std::vector<std::shared_ptr<HookData>> hook_data_list;
    std::set<std::shared_ptr<FunctionSymbolData>, FunctionSymbolDataComparator> symbol_data_list;
    std::map<std::string, std::shared_ptr<SectionInfo>> section_info_list;

    std::string export_name;

    uint32_t bssAddr = 0;
    uint32_t bssSize = 0;
    uint32_t sbssAddr = 0;
    uint32_t sbssSize = 0;
    uint32_t startAddress = 0;
    uint32_t endAddress = 0;
    uint32_t entrypoint = 0;
    bool skipEntrypoint = false;
    bool initBeforeRelocationDoneHook = false;
};
