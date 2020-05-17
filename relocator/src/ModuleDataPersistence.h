#pragma once

#include "../../source/common/module_defines.h"
#include "../../source/module/ModuleData.h"

class ModuleDataPersistence {
public:
    static bool saveModuleData(module_information_t *moduleInformation, const ModuleData &module);

    static std::vector<ModuleData> loadModuleData(module_information_t *moduleInformation);
};
