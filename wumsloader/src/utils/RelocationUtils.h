#pragma once
#include "module/ModuleData.h"
#include <coreinit/dynload.h>
#include <map>
#include <vector>

bool ResolveRelocations(std::vector<std::shared_ptr<ModuleData>> &loadedModules,
                        bool skipMemoryMappingModule,
                        std::map<std::string, OSDynLoad_Module> &usedRPls);

bool doRelocation(const std::vector<std::shared_ptr<ModuleData>> &moduleList,
                  const std::vector<std::unique_ptr<RelocationData>> &relocData,
                  relocation_trampoline_entry_t *tramp_data,
                  uint32_t tramp_length,
                  std::map<std::string, OSDynLoad_Module> &usedRPls);