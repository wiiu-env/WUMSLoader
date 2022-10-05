#include "RelocationUtils.h"
#include "ElfUtils.h"
#include "globals.h"
#include "memory.h"
#include <algorithm>
#include <coreinit/cache.h>
#include <iostream>
#include <iterator>
#include <malloc.h>
#include <vector>

static OSDynLoad_Error CustomDynLoadAlloc(int32_t size, int32_t align, void **outAddr) {
    if (!outAddr) {
        return OS_DYNLOAD_INVALID_ALLOCATOR_PTR;
    }

    if (align >= 0 && align < 4) {
        align = 4;
    } else if (align < 0 && align > -4) {
        align = -4;
    }

    if (!(*outAddr = memalign(align, size))) {
        return OS_DYNLOAD_OUT_OF_MEMORY;
    }

    // keep track of allocated memory to clean it up in case the RPLs won't get unloaded properly
    gAllocatedAddresses.push_back(*outAddr);

    return OS_DYNLOAD_OK;
}

static void CustomDynLoadFree(void *addr) {
    free(addr);

    // Remove from list
    auto it = std::find(gAllocatedAddresses.begin(), gAllocatedAddresses.end(), addr);
    if (it != gAllocatedAddresses.end()) {
        gAllocatedAddresses.erase(it);
    }
}

bool ResolveRelocations(std::vector<std::shared_ptr<ModuleData>> &loadedModules, bool skipMemoryMappingModule, std::map<std::string, OSDynLoad_Module> &usedRPls) {
    bool wasSuccessful = true;

    OSDynLoadAllocFn prevDynLoadAlloc = nullptr;
    OSDynLoadFreeFn prevDynLoadFree   = nullptr;

    OSDynLoad_GetAllocator(&prevDynLoadAlloc, &prevDynLoadFree);
    OSDynLoad_SetAllocator(CustomDynLoadAlloc, CustomDynLoadFree);

    for (auto &curModule : loadedModules) {
        DEBUG_FUNCTION_LINE("Let's do the relocations for %s", curModule->getExportName().c_str());

        auto &relocData = curModule->getRelocationDataList();

        // On first usage we can't redirect the alloc functions to our custom heap
        // because threads can't run it on it. In order to patch the kernel
        // to fully support our memory region, we have to run the MemoryMapping once with the default heap.
        // Afterwards we can just rely on the custom heap.
        bool skipAllocFunction = skipMemoryMappingModule && (std::string_view(curModule->getExportName()) == "homebrew_memorymapping");
        DEBUG_FUNCTION_LINE_VERBOSE("Skip alloc replace? %d", skipAllocFunction);
        if (!doRelocation(gLoadedModules, relocData, nullptr, 0, skipAllocFunction, usedRPls)) {
            wasSuccessful = false;
            DEBUG_FUNCTION_LINE_ERR("Failed to do Relocations for %s", curModule->getExportName().c_str());
            OSFatal("Failed to do Relocations");
        }
    }
    OSDynLoad_SetAllocator(prevDynLoadAlloc, prevDynLoadFree);

    DCFlushRange((void *) MEMORY_REGION_START, MEMORY_REGION_SIZE);
    ICInvalidateRange((void *) MEMORY_REGION_START, MEMORY_REGION_SIZE);
    return wasSuccessful;
}


bool doRelocation(const std::vector<std::shared_ptr<ModuleData>> &moduleList,
                  const std::vector<std::unique_ptr<RelocationData>> &relocData,
                  relocation_trampoline_entry_t *tramp_data,
                  uint32_t tramp_length,
                  bool skipAllocReplacement,
                  std::map<std::string, OSDynLoad_Module> &usedRPls) {
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

            if (!usedRPls.contains(rplName)) {
                DEBUG_FUNCTION_LINE_VERBOSE("Acquire %s", rplName.c_str());
                // Always acquire to increase refcount and make sure it won't get unloaded while we're using it.
                OSDynLoad_Acquire(rplName.c_str(), &rplHandle);
                // Keep track RPLs we are using.
                // They will be released on exit in the AromaBaseModule
                usedRPls[rplName] = rplHandle;
            } else {
                DEBUG_FUNCTION_LINE_VERBOSE("Use from usedRPLs cache! %s", rplName.c_str());
            }
            rplHandle = usedRPls[rplName];

            OSDynLoad_FindExport(rplHandle, isData, functionName.c_str(), (void **) &functionAddress);
            if (functionAddress == 0) {
                DEBUG_FUNCTION_LINE_ERR("Failed to find export %s of %s", functionName.c_str(), rplName.c_str());
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