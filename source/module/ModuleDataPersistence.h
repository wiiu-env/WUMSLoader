#pragma once

#include <wums.h>
#include "ModuleData.h"

class ModuleDataPersistence {
public:
    static bool saveModuleData(module_information_t *moduleInformation, const std::shared_ptr<ModuleData> &module);

    static std::vector<std::shared_ptr<ModuleData>> loadModuleData(module_information_t *moduleInformation);
};
