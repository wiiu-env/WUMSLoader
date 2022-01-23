#include <coreinit/cache.h>
#include "ModuleDataPersistence.h"
#include "DynamicLinkingHelper.h"

bool ModuleDataPersistence::saveModuleData(module_information_t *moduleInformation, const std::shared_ptr<ModuleData> &module) {
    int32_t module_count = moduleInformation->number_used_modules;

    if (module_count >= MAXIMUM_MODULES) {
        DEBUG_FUNCTION_LINE("Reached maximum module count of %d", MAXIMUM_MODULES);
        return false;
    }
    // Copy data to global struct.
    module_information_single_t *module_data = &(moduleInformation->module_data[module_count]);

    DEBUG_FUNCTION_LINE("Saving relocation data for module at %08X", module->getEntrypoint());
    // Relocation
    auto relocationData = module->getRelocationDataList();
    for (auto const &reloc: relocationData) {
        if (!DynamicLinkingHelper::addRelocationEntry(&(moduleInformation->linking_data), module_data->linking_entries,
                                                      DYN_LINK_RELOCATION_LIST_LENGTH, reloc)) {
            DEBUG_FUNCTION_LINE("Failed to add relocation entry\n");
            return false;
        }
    }

    auto exportData = module->getExportDataList();
    for (auto const &curExport: exportData) {
        bool found = false;
        for (auto &export_entry: module_data->export_entries) {
            if (export_entry.address == 0) {
                export_entry.type = curExport->getType();
                export_entry.name[0] = '\0';
                strncat(export_entry.name, curExport->getName().c_str(), sizeof(export_entry.name) - 1);
                export_entry.address = (uint32_t) curExport->getAddress();
                found = true;
                break;
            }
        }
        if (!found) {
            DEBUG_FUNCTION_LINE("Failed to found enough exports slots");
            break;
        }
    }

    auto hookData = module->getHookDataList();
    for (auto const &curHook: hookData) {
        bool found = false;
        for (auto &hook_entry: module_data->hook_entries) {
            if (hook_entry.target == 0) {
                hook_entry.type = curHook->getType();
                hook_entry.target = (uint32_t) curHook->getTarget();
                found = true;
                break;
            }
        }
        if (!found) {
            DEBUG_FUNCTION_LINE("Failed to found enough hook slots");
            break;
        }
    }

    strncpy(module_data->module_export_name, module->getExportName().c_str(), MAXIMUM_EXPORT_MODULE_NAME_LENGTH);

    uint32_t entryCount = module->getFunctionSymbolDataList().size();
    if (entryCount > 0) {
        auto ptr = &moduleInformation->function_symbols[moduleInformation->number_used_function_symbols];
        module_data->function_symbol_entries = ptr;

        uint32_t sym_offset = 0;
        for (auto &curFuncSym: module->getFunctionSymbolDataList()) {
            if (moduleInformation->number_used_function_symbols >= FUNCTION_SYMBOL_LIST_LENGTH) {
                DEBUG_FUNCTION_LINE("Function symbol list is full");
                break;
            }
            module_data->function_symbol_entries[sym_offset].address = curFuncSym->getAddress();
            module_data->function_symbol_entries[sym_offset].name = (char *) curFuncSym->getName();
            module_data->function_symbol_entries[sym_offset].size = curFuncSym->getSize();

            sym_offset++;
            moduleInformation->number_used_function_symbols++;
        }
        module_data->number_used_function_symbols = sym_offset;
    } else {
        module_data->function_symbol_entries = nullptr;
        module_data->number_used_function_symbols = 0;
    }

    module_data->bssAddr = module->getBSSAddr();
    module_data->bssSize = module->getBSSSize();
    module_data->sbssAddr = module->getSBSSAddr();
    module_data->sbssSize = module->getSBSSSize();
    module_data->startAddress = module->getStartAddress();
    module_data->endAddress = module->getEndAddress();
    module_data->entrypoint = module->getEntrypoint();
    module_data->skipInitFini = module->isSkipInitFini();
    module_data->initBeforeRelocationDoneHook = module->isInitBeforeRelocationDoneHook();

    moduleInformation->number_used_modules++;

    DCFlushRange((void *) moduleInformation, sizeof(module_information_t));
    ICInvalidateRange((void *) moduleInformation, sizeof(module_information_t));

    return true;
}

std::vector<std::shared_ptr<ModuleData>> ModuleDataPersistence::loadModuleData(module_information_t *moduleInformation) {
    std::vector<std::shared_ptr<ModuleData>> result;
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
        auto moduleData = std::make_shared<ModuleData>();
        moduleData->setBSSLocation(module_data->bssAddr, module_data->bssSize);
        moduleData->setSBSSLocation(module_data->sbssAddr, module_data->sbssSize);
        moduleData->setEntrypoint(module_data->entrypoint);
        moduleData->setStartAddress(module_data->startAddress);
        moduleData->setEndAddress(module_data->endAddress);
        moduleData->setExportName(module_data->module_export_name);
        moduleData->setSkipInitFini(module_data->skipInitFini);
        moduleData->setInitBeforeRelocationDoneHook(module_data->initBeforeRelocationDoneHook);

        for (auto &export_entrie: module_data->export_entries) {
            export_data_t *export_entry = &export_entrie;
            if (export_entry->address == 0) {
                continue;
            }
            auto exportData = std::make_shared<ExportData>(static_cast<wums_entry_type_t>(export_entry->type), export_entry->name, reinterpret_cast<const void *>(export_entry->address));
            moduleData->addExportData(exportData);
        }

        for (auto &hook_entry: module_data->hook_entries) {
            if (hook_entry.target == 0) {
                continue;
            }
            auto hookData = std::make_shared<HookData>(static_cast<wums_hook_type_t>(hook_entry.type), reinterpret_cast<const void *>(hook_entry.target));
            moduleData->addHookData(hookData);
        }

        for (auto &linking_entry: module_data->linking_entries) {
            if (linking_entry.destination == nullptr) {
                break;
            }
            dyn_linking_import_t *importEntry = linking_entry.importEntry;
            if (importEntry == nullptr) {
                DEBUG_FUNCTION_LINE("importEntry was NULL, skipping relocation entry\n");
                continue;
            }
            dyn_linking_function_t *functionEntry = linking_entry.functionEntry;

            if (functionEntry == nullptr) {
                DEBUG_FUNCTION_LINE("functionEntry was NULL, skipping relocation entry\n");
                continue;
            }
            auto rplInfo = std::make_shared<ImportRPLInformation>(importEntry->importName, importEntry->isData);
            auto reloc = std::make_shared<RelocationData>(linking_entry.type, linking_entry.offset, linking_entry.addend, linking_entry.destination, functionEntry->functionName, rplInfo);

            moduleData->addRelocationData(reloc);
        }

        if (module_data->function_symbol_entries != nullptr && module_data->number_used_function_symbols > 0) {
            for (uint32_t j = 0; j < module_data->number_used_function_symbols; j++) {
                auto symbol = &module_data->function_symbol_entries[j];
                auto functionSymbolData = std::make_shared<FunctionSymbolData>(symbol->name, symbol->address, symbol->size);
                moduleData->addFunctionSymbolData(functionSymbolData);
            }
        }
        result.push_back(moduleData);
    }
    return result;
}
