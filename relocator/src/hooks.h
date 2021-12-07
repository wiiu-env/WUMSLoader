#pragma once

#include <wums.h>

#include <vector>
#include "ModuleDataMinimal.h"

void CallHook(const std::vector<std::shared_ptr<ModuleDataMinimal>> &modules, wums_hook_type_t type);

void CallHook(const std::shared_ptr<ModuleDataMinimal> &module, wums_hook_type_t type);