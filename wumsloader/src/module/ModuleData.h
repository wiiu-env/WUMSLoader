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

#include "ExportData.h"
#include "FunctionSymbolData.h"
#include "HookData.h"
#include "RelocationData.h"
#include "SectionInfo.h"
#include <map>
#include <set>
#include <string>
#include <vector>

struct FunctionSymbolDataComparator {
    bool operator()(const std::shared_ptr<FunctionSymbolData> &lhs,
                    const std::shared_ptr<FunctionSymbolData> &rhs) const {
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

    void addRelocationData(std::unique_ptr<RelocationData> relocation_data) {
        addDependency(relocation_data->getImportRPLInformation()->getRPLName());
        relocation_data_list.push_back(std::move(relocation_data));
    }

    [[nodiscard]] const std::vector<std::unique_ptr<RelocationData>> &getRelocationDataList() const {
        return relocation_data_list;
    }

    void addExportData(std::unique_ptr<ExportData> data) {
        export_data_list.push_back(std::move(data));
    }

    [[nodiscard]] const std::vector<std::unique_ptr<ExportData>> &getExportDataList() const {
        return export_data_list;
    }

    void addHookData(std::unique_ptr<HookData> data) {
        hook_data_list.push_back(std::move(data));
    }

    [[nodiscard]] const std::vector<std::unique_ptr<HookData>> &getHookDataList() const {
        return hook_data_list;
    }

    void addDependency(std::string module_name) {
        dependency_list.insert(std::move(module_name));
    }

    [[nodiscard]] const std::set<std::string> &getDependencies() const {
        return dependency_list;
    }

    void addSectionInfo(std::shared_ptr<SectionInfo> sectionInfo) {
        section_info_list[sectionInfo->getName()] = std::move(sectionInfo);
    }

    void addFunctionSymbolData(std::shared_ptr<FunctionSymbolData> symbol_data) {
        symbol_data_list.insert(std::move(symbol_data));
    }

    [[nodiscard]] const std::set<std::shared_ptr<FunctionSymbolData>, FunctionSymbolDataComparator> &getFunctionSymbolDataList() const {
        return symbol_data_list;
    }

    [[nodiscard]] const std::map<std::string_view, std::shared_ptr<SectionInfo>> &getSectionInfoList() const {
        return section_info_list;
    }

    [[nodiscard]] std::optional<std::shared_ptr<SectionInfo>> getSectionInfo(const std::string &sectionName) const {
        if (getSectionInfoList().count(sectionName) > 0) {
            return section_info_list.at(sectionName);
        }
        return {};
    }

    [[nodiscard]] uint32_t getBSSAddress() const {
        return bssAddr;
    }

    [[nodiscard]] uint32_t getBSSSize() const {
        return bssSize;
    }

    [[nodiscard]] uint32_t getSBSSAddress() const {
        return sbssAddr;
    }

    [[nodiscard]] uint32_t getSBSSSize() const {
        return sbssSize;
    }

    [[nodiscard]] uint32_t getStartAddress() const {
        return reinterpret_cast<uint32_t>(dataPtr.get());
    }

    [[nodiscard]] uint32_t getEndAddress() const {
        return reinterpret_cast<uint32_t>(dataPtr.get()) + this->totalSize;
    }

    [[nodiscard]] uint32_t getEntrypoint() const {
        return entrypoint;
    }

    void setExportName(std::string name) {
        this->export_name = std::move(name);
    }

    [[nodiscard]] const std::string &getExportName() const {
        return this->export_name;
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

    void setDataPtr(std::unique_ptr<uint8_t[]> ptr, uint32_t size) {
        this->dataPtr   = std::move(ptr);
        this->totalSize = size;
    }

    std::unique_ptr<hook_data_t[]> hookDataStruct;
    std::unique_ptr<export_data_t[]> exportDataStruct;
    std::unique_ptr<module_function_symbol_data_t[]> functionSymbolDataStruct;
    std::unique_ptr<dyn_linking_import_t[]> rplDataStruct;
    std::unique_ptr<dyn_linking_relocation_entry_t[]> relocationDataStruct;

private:
    std::vector<std::unique_ptr<RelocationData>> relocation_data_list;
    std::vector<std::unique_ptr<ExportData>> export_data_list;
    std::vector<std::unique_ptr<HookData>> hook_data_list;
    std::set<std::string> dependency_list;
    std::set<std::shared_ptr<FunctionSymbolData>, FunctionSymbolDataComparator> symbol_data_list;
    std::map<std::string_view, std::shared_ptr<SectionInfo>> section_info_list;

    std::unique_ptr<uint8_t[]> dataPtr;
    std::string export_name;

    uint32_t bssAddr                  = 0;
    uint32_t bssSize                  = 0;
    uint32_t sbssAddr                 = 0;
    uint32_t sbssSize                 = 0;
    uint32_t entrypoint               = 0;
    uint32_t totalSize                = 0;
    bool initBeforeRelocationDoneHook = false;
    bool skipInitFini                 = false;
};
