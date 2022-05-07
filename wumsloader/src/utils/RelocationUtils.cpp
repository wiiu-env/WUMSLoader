#include "RelocationUtils.h"
#include "globals.h"
#include "utils/ElfUtils.h"
#include "utils/memory.h"
#include <coreinit/cache.h>
#include <coreinit/dynload.h>

bool ResolveRelocations(std::vector<std::shared_ptr<ModuleData>> &loadedModules, bool skipMemoryMappingModule) {
    bool wasSuccessful = true;

    for (auto &curModule : loadedModules) {
        DEBUG_FUNCTION_LINE("Let's do the relocations for %s", curModule->getExportName().c_str());

        auto &relocData = curModule->getRelocationDataList();

        // On first usage we can't redirect the alloc functions to our custom heap
        // because threads can't run it on it. In order to patch the kernel
        // to fully support our memory region, we have to run the MemoryMapping once with the default heap.
        // Afterwards we can just rely on the custom heap.
        bool skipAllocFunction = skipMemoryMappingModule && (std::string_view(curModule->getExportName()) == "homebrew_memorymapping");
        DEBUG_FUNCTION_LINE_VERBOSE("Skip alloc replace? %d", skipAllocFunction);
        if (!doRelocation(gLoadedModules, relocData, nullptr, 0, skipAllocFunction)) {
            wasSuccessful = false;
            DEBUG_FUNCTION_LINE_ERR("Failed to do Relocations for %s", curModule->getExportName().c_str());
            OSFatal("Failed to do Reloations");
        }
    }
    DCFlushRange((void *) MEMORY_REGION_START, MEMORY_REGION_SIZE);
    ICInvalidateRange((void *) MEMORY_REGION_START, MEMORY_REGION_SIZE);
    return wasSuccessful;
}


bool doRelocation(const std::vector<std::shared_ptr<ModuleData>> &moduleList,
                  const std::vector<std::unique_ptr<RelocationData>> &relocData,
                  relocation_trampoline_entry_t *tramp_data,
                  uint32_t tramp_length,
                  bool skipAllocReplacement) {
    std::map<std::string, OSDynLoad_Module> moduleCache;
    for (auto const &curReloc : relocData) {
        auto &functionName       = curReloc->getName();
        std::string rplName      = curReloc->getImportRPLInformation()->getRPLName();
        uint32_t functionAddress = 0;

        for (auto &module : moduleList) {
            if (rplName == module->getExportName()) {
                for (auto &exportData : module->getExportDataList()) {
                    if (functionName == exportData->getName()) {
                        functionAddress = (uint32_t) exportData->getAddress();
                    }
                }
            }
        }

        if (!skipAllocReplacement) {
            if (functionName == "MEMAllocFromDefaultHeap") {
                functionAddress = reinterpret_cast<uint32_t>(&MEMAlloc);
            } else if (functionName == "MEMAllocFromDefaultHeapEx") {
                functionAddress = reinterpret_cast<uint32_t>(&MEMAllocEx);
            } else if (functionName == "MEMFreeToDefaultHeap") {
                functionAddress = reinterpret_cast<uint32_t>(&MEMFree);
            }
        }

        if (functionAddress == 0) {
            int32_t isData             = curReloc->getImportRPLInformation()->isData();
            OSDynLoad_Module rplHandle = nullptr;
            if (moduleCache.count(rplName) == 0) {
                OSDynLoad_Error err = OSDynLoad_IsModuleLoaded(rplName.c_str(), &rplHandle);
                if (err != OS_DYNLOAD_OK || rplHandle == nullptr) {
                    DEBUG_FUNCTION_LINE_VERBOSE("%s is not yet loaded", rplName.c_str());
                    // only acquire if not already loaded.
                    err = OSDynLoad_Acquire(rplName.c_str(), &rplHandle);
                    if (err != OS_DYNLOAD_OK) {
                        DEBUG_FUNCTION_LINE_ERR("Failed to acquire %s", rplName.c_str());
                        return false;
                    }
                }
                moduleCache[rplName] = rplHandle;
            }
            rplHandle = moduleCache.at(rplName);

            OSDynLoad_FindExport(rplHandle, isData, functionName.c_str(), (void **) &functionAddress);
            if (functionAddress == 0) {
                DEBUG_FUNCTION_LINE_ERR("Failed to find export %s of %s", functionName.begin(), rplName.c_str());
                OSFatal("Failed to find export");
                return false;
            }
        }

        if (!ElfUtils::elfLinkOne(curReloc->getType(), curReloc->getOffset(), curReloc->getAddend(), (uint32_t) curReloc->getDestination(), functionAddress, tramp_data, tramp_length,
                                  RELOC_TYPE_IMPORT)) {
            DEBUG_FUNCTION_LINE_ERR("Relocation failed");
            return false;
        }
    }
    if (tramp_data != nullptr) {
        DCFlushRange(tramp_data, tramp_length * sizeof(relocation_trampoline_entry_t));
        ICInvalidateRange(tramp_data, tramp_length * sizeof(relocation_trampoline_entry_t));
    }
    return true;
}