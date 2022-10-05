#pragma once
#include "module/ModuleData.h"
#include <coreinit/dynload.h>
#include <vector>

bool ResolveRelocations(std::vector<std::shared_ptr<ModuleData>> &loadedModules,
                        bool skipMemoryMappingModule,
                        std::vector<OSDynLoad_Module> &loadedRPLs);

bool doRelocation(const std::vector<std::shared_ptr<ModuleData>> &moduleList,
                  const std::vector<std::unique_ptr<RelocationData>> &relocData,
                  relocation_trampoline_entry_t *tramp_data,
                  uint32_t tramp_length,
                  bool skipAllocReplacement,
                  std::vector<OSDynLoad_Module> &loadedRPLs);