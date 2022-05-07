#pragma once

#include "module/ModuleData.h"
#include <memory>
#include <vector>
#include <wums.h>

void CallHook(const std::vector<std::shared_ptr<ModuleData>> &modules, wums_hook_type_t type, bool condition);

void CallHook(const std::vector<std::shared_ptr<ModuleData>> &modules, wums_hook_type_t type);

void CallHook(const std::shared_ptr<ModuleData> &module, wums_hook_type_t type, bool condition);

void CallHook(const std::shared_ptr<ModuleData> &module, wums_hook_type_t type);
