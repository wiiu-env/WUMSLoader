#pragma once

#include <wums.h>

#include <vector>
#include "ModuleDataMinimal.h"

void CallHook(const std::vector<ModuleDataMinimal> &modules, wums_hook_type_t type);

void CallHook(const ModuleDataMinimal &module, wums_hook_type_t type);