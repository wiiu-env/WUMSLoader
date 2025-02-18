#include "RelocationUtils.h"
#include "ElfUtils.h"
#include "globals.h"
#include "memory.h"
#include <algorithm>
#include <coreinit/cache.h>
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
    if (const auto it = std::ranges::find(gAllocatedAddresses, addr); it != gAllocatedAddresses.end()) {
        gAllocatedAddresses.erase(it);
    }
}

bool ResolveRelocations(const std::vector<std::shared_ptr<ModuleData>> &loadedModules, const bool skipUnloadedRpl, std::map<std::string, OSDynLoad_Module> &usedRPls) {
    bool wasSuccessful = true;


    OSDynLoadAllocFn prevDynLoadAlloc = nullptr;
    OSDynLoadFreeFn prevDynLoadFree   = nullptr;

    if (!skipUnloadedRpl) {
        OSDynLoad_GetAllocator(&prevDynLoadAlloc, &prevDynLoadFree);
        if (gCustomRPLAllocatorAllocFn != nullptr && gCustomRPLAllocatorFreeFn != nullptr) {
            OSDynLoad_SetAllocator(reinterpret_cast<OSDynLoadAllocFn>(gCustomRPLAllocatorAllocFn), gCustomRPLAllocatorFreeFn);
        } else {
            OSDynLoad_SetAllocator(CustomDynLoadAlloc, CustomDynLoadFree);
        }
    }

    for (auto &curModule : loadedModules) {
        DEBUG_FUNCTION_LINE("Let's do the relocations for %s", curModule->getExportName().c_str());

        auto &relocData = curModule->getRelocationDataList();

        if (!doRelocation(gLoadedModules, relocData, nullptr, 0, usedRPls, skipUnloadedRpl)) {
            wasSuccessful = false;
            DEBUG_FUNCTION_LINE_ERR("Failed to do Relocations for %s", curModule->getExportName().c_str());
            OSFatal("Failed to do Relocations");
        }
    }
    if (!skipUnloadedRpl) {
        OSDynLoad_SetAllocator(prevDynLoadAlloc, prevDynLoadFree);
    }

    DCFlushRange((void *) MEMORY_REGION_START, MEMORY_REGION_SIZE);
    ICInvalidateRange((void *) MEMORY_REGION_START, MEMORY_REGION_SIZE);
    return wasSuccessful;
}


bool doRelocation(const std::vector<std::shared_ptr<ModuleData>> &moduleList,
                  const std::vector<std::unique_ptr<RelocationData>> &relocData,
                  relocation_trampoline_entry_t *tramp_data,
                  const uint32_t tramp_length,
                  std::map<std::string, OSDynLoad_Module> &usedRPls,
                  const bool skipUnloadedRpl) {
    for (auto const &curReloc : relocData) {
        auto &functionName       = curReloc->getName();
        std::string rplName      = curReloc->getImportRPLInformation()->getRPLName();
        uint32_t functionAddress = 0;

        for (auto &module : moduleList) {
            if (rplName == module->getExportName()) {
                auto found = false;
                for (auto &exportData : module->getExportDataList()) {
                    if (functionName == exportData->getName()) {
                        functionAddress = (uint32_t) exportData->getAddress();
                        found           = true;
                    }
                }
                if (!found) {
                    DEBUG_FUNCTION_LINE_ERR("Failed to find export %s of module: %s", functionName.c_str(), rplName.c_str());
                    OSFatal("Failed to find export of module.");
                }
            }
        }

        if (functionName == "MEMAllocFromDefaultHeap") {
            functionAddress = reinterpret_cast<uint32_t>(&MEMAlloc);
        } else if (functionName == "MEMAllocFromDefaultHeapEx") {
            functionAddress = reinterpret_cast<uint32_t>(&MEMAllocEx);
        } else if (functionName == "MEMFreeToDefaultHeap") {
            functionAddress = reinterpret_cast<uint32_t>(&MEMFree);
        }

        if (functionAddress == 0) {
            int32_t isData             = curReloc->getImportRPLInformation()->isData();
            OSDynLoad_Module rplHandle = nullptr;

            if (!usedRPls.contains(rplName)) {
                OSDynLoad_Module tmp = nullptr;
                if (OSDynLoad_IsModuleLoaded(rplName.c_str(), &tmp) != OS_DYNLOAD_OK || tmp == nullptr) {
                    if (skipUnloadedRpl) {
                        DEBUG_FUNCTION_LINE_VERBOSE("Skip acquire of %s", rplName.c_str());
                        continue;
                    }
                }

                // Always acquire to increase refcount and make sure it won't get unloaded while we're using it.
                OSDynLoad_Error err = OSDynLoad_Acquire(rplName.c_str(), &rplHandle);
                if (err != OS_DYNLOAD_OK) {
                    DEBUG_FUNCTION_LINE_ERR("Failed to acquire %s", rplName.c_str());
                    return false;
                }
                // Keep track RPLs we are using.
                // They will be released on exit (See: AromaBaseModule)
                usedRPls[rplName] = rplHandle;
            } else {
                //DEBUG_FUNCTION_LINE_VERBOSE("Use from usedRPLs cache! %s", rplName.c_str());
            }
            rplHandle = usedRPls[rplName];

            OSDynLoad_FindExport(rplHandle, (OSDynLoad_ExportType) isData, functionName.c_str(), (void **) &functionAddress);
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