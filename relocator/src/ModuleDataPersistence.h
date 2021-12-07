#pragma once

#include <wums.h>
#include <vector>
#include "ModuleDataMinimal.h"

class ModuleDataPersistence {
public:
    static std::vector<std::shared_ptr<ModuleDataMinimal>> loadModuleData(module_information_t *moduleInformation);
};
