#pragma once
#include "module/ModuleData.h"
#include <vector>

bool ResolveRelocations(std::vector<std::shared_ptr<ModuleData>> &loadedModules,
                        bool skipMemoryMappingModule);

bool doRelocation(const std::vector<std::shared_ptr<ModuleData>> &moduleList,
                  const std::vector<std::unique_ptr<RelocationData>> &relocData,
                  relocation_trampoline_entry_t *tramp_data,
                  uint32_t tramp_length,
                  bool skipAllocReplacement);