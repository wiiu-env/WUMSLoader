#pragma once

#include "ModuleData.h"
#include <coreinit/memheap.h>
#include <wums.h>

class ModuleDataPersistence {
public:
    static bool saveModuleData(module_information_t *moduleInformation, const std::vector<std::shared_ptr<ModuleData>> &moduleList);
    static bool saveModuleData(module_information_single_t &module_data, const std::shared_ptr<ModuleData> &module);
    static bool saveRelocationDataForModule(module_information_single_t &module_data, const std::shared_ptr<ModuleData> &module);
    static bool saveExportDataForModule(module_information_single_t &module_data, const std::shared_ptr<ModuleData> &module);
    static bool saveHookDataForModule(module_information_single_t &module_data, const std::shared_ptr<ModuleData> &module);
    static bool saveFunctionSymbolDataForModule(module_information_single_t &module_data, const std::shared_ptr<ModuleData> &module);
};
