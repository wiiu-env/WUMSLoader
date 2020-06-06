#pragma once

#include <wums.h>
#include "ModuleData.h"

class ModuleDataPersistence {
public:
    static bool saveModuleData(module_information_t *moduleInformation, const ModuleData &module);

    static std::vector<ModuleData> loadModuleData(module_information_t *moduleInformation);
};
