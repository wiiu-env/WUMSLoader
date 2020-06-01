#include <coreinit/cache.h>

#include "ModuleDataPersistence.h"
#include "DynamicLinkingHelper.h"
#include "common/module_defines.h"
#include "ModuleData.h"
#include "RelocationData.h"

bool ModuleDataPersistence::saveModuleData(module_information_t *moduleInformation, const ModuleData &module) {
    int32_t module_count = moduleInformation->number_used_modules;

    if (module_count >= MAXIMUM_MODULES) {
        DEBUG_FUNCTION_LINE("Reached maximum module count of %d", MAXIMUM_MODULES);
        return false;
    }
    // Copy data to global struct.
    module_information_single_t *module_data = &(moduleInformation->module_data[module_count]);

    DEBUG_FUNCTION_LINE("Saving reloation data for module at %08X", module.getEntrypoint());
    // Relocation
    std::vector<RelocationData> relocationData = module.getRelocationDataList();
    for (auto const &reloc : relocationData) {
        if (!DynamicLinkingHelper::addReloationEntry(&(moduleInformation->linking_data), module_data->linking_entries, DYN_LINK_RELOCATION_LIST_LENGTH, reloc)) {
            DEBUG_FUNCTION_LINE("Failed to add relocation entry\n");
            return false;
        }
    }

    std::vector<ExportData> exportData = module.getExportDataList();
    for (auto const &curExport : exportData) {
        bool found = false;
        for (uint32_t j = 0; j < EXPORT_ENTRY_LIST_LENGTH; j++) {
            export_data_t *export_entry = &(module_data->export_entries[j]);
            if (export_entry->address == 0) {
                export_entry->type = curExport.getType();
                strncpy(export_entry->name, curExport.getName().c_str(), EXPORT_MAXIMUM_NAME_LENGTH);
                export_entry->address = (uint32_t) curExport.getAddress();
                found = true;
                break;
            }
        }
        if (!found) {
            DEBUG_FUNCTION_LINE("Failed to found enough exports slots");
            break;
        }
    }

    std::vector<HookData> hookData = module.getHookDataList();
    for (auto const &curHook : hookData) {
        bool found = false;
        for (uint32_t j = 0; j < HOOK_ENTRY_LIST_LENGTH; j++) {
            hook_data_t *hook_entry = &(module_data->hook_entries[j]);
            if (hook_entry->target == 0) {
                hook_entry->type = curHook.getType();
                hook_entry->target = (uint32_t) curHook.getTarget();
                found = true;
                break;
            }
        }
        if (!found) {
            DEBUG_FUNCTION_LINE("Failed to found enough hook slots");
            break;
        }
    }

    strncpy(module_data->module_export_name, module.getExportName().c_str(), MAXIMUM_EXPORT_MODULE_NAME_LENGTH);

    module_data->bssAddr = module.getBSSAddr();
    module_data->bssSize = module.getBSSSize();
    module_data->sbssAddr = module.getSBSSAddr();
    module_data->sbssSize = module.getSBSSSize();
    module_data->startAddress = module.getStartAddress();
    module_data->endAddress = module.getEndAddress();

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

        for (uint32_t j = 0; j < EXPORT_ENTRY_LIST_LENGTH; j++) {
            export_data_t *export_entry = &(module_data->export_entries[j]);
            if (export_entry->address == NULL) {
                continue;
            }
            moduleData.addExportData(ExportData(static_cast<wums_entry_type_t>(export_entry->type), export_entry->name, reinterpret_cast<const void *>(export_entry->address)));
        }

        for (uint32_t j = 0; j < HOOK_ENTRY_LIST_LENGTH; j++) {
            hook_data_t *hook_entry = &(module_data->hook_entries[j]);
            if (hook_entry->target == NULL) {
                continue;
            }
            moduleData.addHookData(HookData(static_cast<wums_hook_type_t>(hook_entry->type), reinterpret_cast<const void *>(hook_entry->target)));
        }

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
