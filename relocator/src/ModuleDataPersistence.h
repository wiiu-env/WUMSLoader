#pragma once

#include "ModuleDataMinimal.h"
#include <vector>
#include <wums.h>

class ModuleDataPersistence {
public:
    static std::vector<std::shared_ptr<ModuleDataMinimal>> loadModuleData(module_information_t *moduleInformation);
};
