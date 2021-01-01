/****************************************************************************
 * Copyright (C) 2018 Maschell
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
#include "RelocationData.h"
#include "SectionInfo.h"
#include "ExportData.h"
#include "HookData.h"

class ModuleData {
public:
    ModuleData() {
    }

    ~ModuleData() {
    }

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

    void setEndAddress(uint32_t endAddress) {
        this->endAddress = endAddress;
    }

    void addRelocationData(const RelocationData &relocation_data) {
        relocation_data_list.push_back(relocation_data);
    }

    const std::vector<RelocationData> &getRelocationDataList() const {
        return relocation_data_list;
    }

    void addExportData(const ExportData &data) {
        export_data_list.push_back(data);
    }

    const std::vector<ExportData> &getExportDataList() const {
        return export_data_list;
    }

    void addHookData(const HookData &data) {
        hook_data_list.push_back(data);
    }

    const std::vector<HookData> &getHookDataList() const {
        return hook_data_list;
    }

    void addSectionInfo(const SectionInfo &sectionInfo) {
        section_info_list[sectionInfo.getName()] = sectionInfo;
    }

    const std::map<std::string, SectionInfo> &getSectionInfoList() const {
        return section_info_list;
    }

    std::optional<SectionInfo> getSectionInfo(const std::string &sectionName) const {
        if (getSectionInfoList().count(sectionName) > 0) {
            return section_info_list.at(sectionName);
        }
        return std::nullopt;
    }

    uint32_t getBSSAddr() const {
        return bssAddr;
    }

    uint32_t getBSSSize() const {
        return bssSize;
    }

    uint32_t getSBSSAddr() const {
        return sbssAddr;
    }

    uint32_t getSBSSSize() const {
        return sbssSize;
    }

    uint32_t getEntrypoint() const {
        return entrypoint;
    }

    uint32_t getStartAddress() const {
        return startAddress;
    }

    uint32_t getEndAddress() const {
        return endAddress;
    }

    std::string toString() const;

    void setExportName(const std::string &name) {
        this->export_name = name;
    }

    std::string getExportName() const {
        return this->export_name;
    }

    bool isSkipEntrypoint() const {
        return this->skipEntrypoint;
    }

    bool isInitBeforeRelocationDoneHook() const {
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
    std::vector<RelocationData> relocation_data_list;
    std::vector<ExportData> export_data_list;
    std::vector<HookData> hook_data_list;
    std::map<std::string, SectionInfo> section_info_list;

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
