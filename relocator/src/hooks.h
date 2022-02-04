#pragma once

#include "ModuleDataMinimal.h"
#include <vector>
#include <wums.h>

void CallHook(const std::vector<std::shared_ptr<ModuleDataMinimal>> &modules, wums_hook_type_t type, bool condition);

void CallHook(const std::vector<std::shared_ptr<ModuleDataMinimal>> &modules, wums_hook_type_t type);

void CallHook(const std::shared_ptr<ModuleDataMinimal> &module, wums_hook_type_t type, bool condition);

void CallHook(const std::shared_ptr<ModuleDataMinimal> &module, wums_hook_type_t type);
