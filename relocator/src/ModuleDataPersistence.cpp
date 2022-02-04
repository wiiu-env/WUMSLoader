#include "ModuleDataPersistence.h"
#include "DynamicLinkingHelper.h"
#include <coreinit/cache.h>
#include <wums.h>

std::vector<std::shared_ptr<ModuleDataMinimal>> ModuleDataPersistence::loadModuleData(module_information_t *moduleInformation) {
    std::vector<std::shared_ptr<ModuleDataMinimal>> result;
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
        auto moduleData                          = std::make_shared<ModuleDataMinimal>();

        moduleData->setEntrypoint(module_data->entrypoint);
        moduleData->setInitBeforeRelocationDoneHook(module_data->initBeforeRelocationDoneHook);
        moduleData->setSkipInitFini(module_data->skipInitFini);
        moduleData->setExportName(module_data->module_export_name);

        for (auto &hook_entry : module_data->hook_entries) {
            if (hook_entry.target == 0) {
                continue;
            }
            moduleData->addHookData(std::make_shared<HookData>(static_cast<wums_hook_type_t>(hook_entry.type), reinterpret_cast<const void *>(hook_entry.target)));
        }

        for (auto &linking_entry : module_data->linking_entries) {
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
            auto rplInfo = std::make_shared<ImportRPLInformation>(importEntry->importName, importEntry->isData);
            auto reloc   = std::make_shared<RelocationData>(linking_entry.type, linking_entry.offset, linking_entry.addend, linking_entry.destination, functionEntry->functionName, rplInfo);

            moduleData->addRelocationData(reloc);
        }

        result.push_back(moduleData);
    }
    return result;
}
