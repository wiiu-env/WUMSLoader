#include "ModuleDataPersistence.h"
#include "DynamicLinkingHelper.h"
#include "../../source/common/module_defines.h"
#include "../../source/module/ModuleData.h"
#include "../../source/module/RelocationData.h"
#include <coreinit/cache.h>

bool ModuleDataPersistence::saveModuleData(module_information_t *moduleInformation, const ModuleData &module) {
    int32_t module_count = moduleInformation->number_used_modules;

    if (module_count >= MAXIMUM_MODULES) {
        return false;
    }
    // Copy data to global struct.
    module_information_single_t *module_data = &(moduleInformation->module_data[module_count]);

    // Relocation
    std::vector<RelocationData> relocationData = module.getRelocationDataList();
    for (auto const &reloc : relocationData) {
        if (!DynamicLinkingHelper::addReloationEntry(&(moduleInformation->linking_data), module_data->linking_entries, DYN_LINK_RELOCATION_LIST_LENGTH, reloc)) {
            return false;
        }
    }

    module_data->bssAddr = module.getBSSAddr();
    module_data->bssSize = module.getBSSSize();
    module_data->sbssAddr = module.getSBSSAddr();
    module_data->sbssSize = module.getSBSSSize();
    module_data->endAddress = module.getStartAddress();
    module_data->startAddress = module.getEndAddress();

    module_data->entrypoint = module.getEntrypoint();

    moduleInformation->number_used_modules++;

    DCFlushRange((void *) moduleInformation, sizeof(module_information_t));
    ICInvalidateRange((void *) moduleInformation, sizeof(module_information_t));
    return true;
}

std::vector<ModuleData> ModuleDataPersistence::loadModuleData(module_information_t *moduleInformation) {
    std::vector<ModuleData> result;
    if (moduleInformation == NULL) {
        DEBUG_FUNCTION_LINE("moduleInformation == NULL\n");
        return result;
    }
    DCFlushRange((void *) moduleInformation, sizeof(module_information_t));
    ICInvalidateRange((void *) moduleInformation, sizeof(module_information_t));

    int32_t module_count = moduleInformation->number_used_modules;

    if (module_count > MAXIMUM_MODULES) {
        DEBUG_FUNCTION_LINE("moduleInformation->module_count was bigger then allowed. %d > %d. Limiting to %d\n", module_count, MAXIMUM_MODULES, MAXIMUM_MODULES);
        module_count = MAXIMUM_MODULES;
    }

    for (int32_t i = 0; i < module_count; i++) {
        // Copy data from struct.
        module_information_single_t *module_data = &(moduleInformation->module_data[i]);
        ModuleData moduleData;

        moduleData.setBSSLocation(module_data->bssAddr, module_data->bssSize);
        moduleData.setSBSSLocation(module_data->sbssAddr, module_data->sbssSize);
        moduleData.setEntrypoint(module_data->entrypoint);
        moduleData.setStartAddress(module_data->startAddress);
        moduleData.setEndAddress(module_data->endAddress);


        moduleData.setExportName(module_data->module_export_name);
        for (uint32_t j = 0; j < DYN_LINK_RELOCATION_LIST_LENGTH; j++) {
            dyn_linking_relocation_entry_t *linking_entry = &(module_data->linking_entries[j]);
            if (linking_entry->destination == NULL) {
                break;
            }
            dyn_linking_import_t *importEntry = linking_entry->importEntry;
            if (importEntry == NULL) {
                DEBUG_FUNCTION_LINE("importEntry was NULL, skipping relocation entry\n");
                continue;
            }
            if (importEntry->importName == NULL) {
                DEBUG_FUNCTION_LINE("importEntry->importName was NULL, skipping relocation entry\n");
                continue;
            }
            dyn_linking_function_t *functionEntry = linking_entry->functionEntry;

            if (functionEntry == NULL) {
                DEBUG_FUNCTION_LINE("functionEntry was NULL, skipping relocation entry\n");
                continue;
            }
            if (functionEntry->functionName == NULL) {
                DEBUG_FUNCTION_LINE("functionEntry->functionName was NULL, skipping relocation entry\n");
                continue;
            }
            ImportRPLInformation rplInfo(importEntry->importName, importEntry->isData);
            RelocationData reloc(linking_entry->type, linking_entry->offset, linking_entry->addend, linking_entry->destination, functionEntry->functionName, rplInfo);

            moduleData.addRelocationData(reloc);
        }
        result.push_back(moduleData);
    }
    return result;
}
