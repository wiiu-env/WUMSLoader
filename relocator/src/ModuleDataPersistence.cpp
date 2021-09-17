#include "ModuleDataPersistence.h"
#include "DynamicLinkingHelper.h"
#include <coreinit/cache.h>
#include <wums.h>

std::vector<ModuleData> ModuleDataPersistence::loadModuleData(module_information_t *moduleInformation) {
    std::vector<ModuleData> result;
    if (moduleInformation == nullptr) {
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
        moduleData.setSkipEntrypoint(module_data->skipEntrypoint);
        moduleData.setInitBeforeRelocationDoneHook(module_data->initBeforeRelocationDoneHook);

        moduleData.setExportName(module_data->module_export_name);

        for (auto & export_entry : module_data->export_entries) {
            if (export_entry.address == 0) {
                continue;
            }
            moduleData.addExportData(ExportData(static_cast<wums_entry_type_t>(export_entry.type), export_entry.name, reinterpret_cast<const void *>(export_entry.address)));
        }

        for (auto & hook_entry : module_data->hook_entries) {
            if (hook_entry.target == 0) {
                continue;
            }
            moduleData.addHookData(HookData(static_cast<wums_hook_type_t>(hook_entry.type), reinterpret_cast<const void *>(hook_entry.target)));
        }

        for (auto & linking_entry : module_data->linking_entries) {
            if (linking_entry.destination == nullptr) {
                break;
            }
            dyn_linking_import_t *importEntry = linking_entry.importEntry;
            if (importEntry == nullptr) {
                DEBUG_FUNCTION_LINE("importEntry was NULL, skipping relocation entry\n");
                continue;
            }
            if (importEntry->importName == nullptr) {
                DEBUG_FUNCTION_LINE("importEntry->importName was NULL, skipping relocation entry\n");
                continue;
            }
            dyn_linking_function_t *functionEntry = linking_entry.functionEntry;

            if (functionEntry == nullptr) {
                DEBUG_FUNCTION_LINE("functionEntry was NULL, skipping relocation entry\n");
                continue;
            }
            if (functionEntry->functionName == nullptr) {
                DEBUG_FUNCTION_LINE("functionEntry->functionName was NULL, skipping relocation entry\n");
                continue;
            }
            ImportRPLInformation rplInfo(importEntry->importName, importEntry->isData);
            RelocationData reloc(linking_entry.type, linking_entry.offset, linking_entry.addend, linking_entry.destination, functionEntry->functionName, rplInfo);

            moduleData.addRelocationData(reloc);
        }
        result.push_back(moduleData);
    }
    return result;
}
