#pragma once

#include <wums.h>

#include <vector>
#include "../../source/module/ModuleData.h"

void CallHook(const std::vector<ModuleData> &modules, wums_hook_type_t type);