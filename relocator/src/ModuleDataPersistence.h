#pragma once

#include <wums.h>
#include "../../source/module/ModuleData.h"

class ModuleDataPersistence {
public:
    static std::vector<ModuleData> loadModuleData(module_information_t *moduleInformation);
};
