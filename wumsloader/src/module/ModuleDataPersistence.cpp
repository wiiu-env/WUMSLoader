#include "ModuleDataPersistence.h"
#include "globals.h"
#include <coreinit/cache.h>

bool ModuleDataPersistence::saveModuleData(module_information_t *moduleInformation, const std::vector<std::shared_ptr<ModuleData>> &moduleList) {
    auto module_data_list = std::make_unique<module_information_single_t[]>(moduleList.size());
    if (!module_data_list) {
        DEBUG_FUNCTION_LINE_ERR("Failed to alloc memory to persist module data");
        return false;
    }

    moduleInformation->modules = module_data_list.get();

    uint32_t i = 0;
    for (auto &module : moduleList) {
        auto &module_data = module_data_list[i];
        module_data       = {};
        if (!saveModuleData(module_data, module)) {
            module->relocationDataStruct.reset();
            module->hookDataStruct.reset();
            module->exportDataStruct.reset();
            module->functionSymbolDataStruct.reset();
            module->rplDataStruct.reset();
            DEBUG_FUNCTION_LINE_ERR("Failed to persist data for module. No memory?");
            OSFatal("Failed to persist data for module. No memory?");
            continue;
        } else {
            moduleInformation->number_modules++;
            i++;
        }
    }

    gModuleDataInfo = std::move(module_data_list);
    OSMemoryBarrier();

    return true;
}

bool ModuleDataPersistence::saveModuleData(module_information_single_t &module_data, const std::shared_ptr<ModuleData> &module) {
    if (!saveRelocationDataForModule(module_data, module)) {
        DEBUG_FUNCTION_LINE_ERR("Failed to store relocation data of module");
        return false;
    }

    if (!saveHookDataForModule(module_data, module)) {
        DEBUG_FUNCTION_LINE_ERR("Failed to store hook data of module");
        return false;
    }

    if (!saveExportDataForModule(module_data, module)) {
        DEBUG_FUNCTION_LINE_ERR("Failed to store export data of module");
        return false;
    }

    if (!saveFunctionSymbolDataForModule(module_data, module)) {
        DEBUG_FUNCTION_LINE_ERR("Failed to store function symbol data of module");
        return false;
    }

    module_data.module_export_name = (char *) module->getExportName().c_str();

    module_data.bssAddr                      = module->getBSSAddress();
    module_data.bssSize                      = module->getBSSSize();
    module_data.sbssAddr                     = module->getSBSSAddress();
    module_data.sbssSize                     = module->getSBSSSize();
    module_data.startAddress                 = module->getStartAddress();
    module_data.endAddress                   = module->getEndAddress();
    module_data.entrypoint                   = module->getEntrypoint();
    module_data.skipInitFini                 = module->isSkipInitFini();
    module_data.initBeforeRelocationDoneHook = module->isInitBeforeRelocationDoneHook();
    return true;
}

bool ModuleDataPersistence::saveFunctionSymbolDataForModule(module_information_single_t &module_data, const std::shared_ptr<ModuleData> &module) {
    uint32_t entryCount       = module->getFunctionSymbolDataList().size();
    auto function_symbol_data = std::make_unique<module_function_symbol_data_t[]>(entryCount);
    if (!function_symbol_data) {
        DEBUG_FUNCTION_LINE_ERR("Failed to allocate memory for the function symbol data.");
        return false;
    }
    uint32_t i = 0;
    for (auto &curFuncSym : module->getFunctionSymbolDataList()) {
        if (i >= entryCount) {
            DEBUG_FUNCTION_LINE_ERR("We tried to write more entries than we have space for.");
            OSFatal("We tried to write more entries than we have space for.");
        }
        function_symbol_data[i].address = curFuncSym->getAddress();
        function_symbol_data[i].name    = (char *) curFuncSym->getName().c_str();
        function_symbol_data[i].size    = curFuncSym->getSize();
        i++;
    }
    module_data.function_symbol_entries = function_symbol_data.get();
    module_data.number_function_symbols = i;

    module->functionSymbolDataStruct = std::move(function_symbol_data);

    return true;
}

bool ModuleDataPersistence::saveExportDataForModule(module_information_single_t &module_data, const std::shared_ptr<ModuleData> &module) {
    auto export_data = std::make_unique<export_data_t[]>(module->getExportDataList().size());
    if (!export_data) {
        DEBUG_FUNCTION_LINE_ERR("Failed to allocate memory for the export data.");
        return false;
    }

    uint32_t exportCount = 0;
    for (auto const &export_ : module->getExportDataList()) {
        if (exportCount >= module->getExportDataList().size()) {
            DEBUG_FUNCTION_LINE_ERR("We tried to write more entries than we have space for.");
            OSFatal("We tried to write more entries than we have space for.");
        }
        auto *curExport    = &export_data[exportCount++];
        curExport->type    = export_->getType();
        curExport->name    = export_->getName().c_str();
        curExport->address = (uint32_t) export_->getAddress();
    }
    module_data.export_entries        = export_data.get();
    module_data.number_export_entries = exportCount;

    module->exportDataStruct = std::move(export_data);

    return true;
}

bool ModuleDataPersistence::saveHookDataForModule(module_information_single_t &module_data, const std::shared_ptr<ModuleData> &module) {
    auto hook_data = std::make_unique<hook_data_t[]>(module->getHookDataList().size());
    if (!hook_data) {
        DEBUG_FUNCTION_LINE_ERR("Failed to allocate memory for the hook data.");
        return false;
    }

    uint32_t hookCount = 0;
    for (auto const &hook : module->getHookDataList()) {
        if (hookCount >= module->getHookDataList().size()) {
            DEBUG_FUNCTION_LINE_ERR("We tried to write more entries than we have space for.");
            OSFatal("We tried to write more entries than we have space for.");
        }
        auto *curHook   = &hook_data[hookCount++];
        curHook->type   = hook->getType();
        curHook->target = (uint32_t) hook->getTarget();
    }
    module_data.hook_entries        = hook_data.get();
    module_data.number_hook_entries = hookCount;

    module->hookDataStruct = std::move(hook_data);

    return true;
}

bool ModuleDataPersistence::saveRelocationDataForModule(module_information_single_t &module_data, const std::shared_ptr<ModuleData> &module) {
    auto relocation_data = std::make_unique<dyn_linking_relocation_entry_t[]>(module->getRelocationDataList().size());
    if (!relocation_data) {
        DEBUG_FUNCTION_LINE_ERR("Failed to allocate memory for the relocation data.");
        return false;
    }

    // Determine how many dyn_linking_import_t entries we need.
    std::set<std::string_view> rplInfoCountSet;
    for (auto const &reloc : module->getRelocationDataList()) {
        rplInfoCountSet.insert(reloc->getImportRPLInformation()->getName());
    }

    uint32_t rplInfoTotalCount = rplInfoCountSet.size();
    rplInfoCountSet.clear();

    auto rpl_data = std::make_unique<dyn_linking_import_t[]>(rplInfoTotalCount);
    if (!rpl_data) {
        DEBUG_FUNCTION_LINE_ERR("Failed to allocate memory for the RPLInfos.");
        return false;
    }

    uint32_t relocationCount = 0;
    uint32_t rplInfoCount    = 0;
    std::map<std::string_view, dyn_linking_import_t *> rplInfoMap;
    for (auto const &reloc : module->getRelocationDataList()) {
        if (relocationCount >= module->getRelocationDataList().size()) {
            DEBUG_FUNCTION_LINE_ERR("We tried to write more entries than we have space for.");
            OSFatal("We tried to write more entries than we have space for.");
        }
        auto *curReloc         = &relocation_data[relocationCount++];
        curReloc->destination  = reloc->getDestination();
        curReloc->offset       = reloc->getOffset();
        curReloc->addend       = reloc->getAddend();
        curReloc->type         = reloc->getType();
        curReloc->functionName = reloc->getName().c_str();
        auto &rplInfo          = reloc->getImportRPLInformation();

        auto rplIt = rplInfoMap.find(rplInfo->getName());
        if (rplIt != rplInfoMap.end()) {
            curReloc->importEntry = rplIt->second;
        } else {
            if (rplInfoCount >= rplInfoTotalCount) {
                DEBUG_FUNCTION_LINE_ERR("We tried to write more entries than we have space for.");
                OSFatal("We tried to write more entries than we have space for.");
            }
            auto *rplInfoPtr               = &rpl_data[rplInfoCount++];
            rplInfoPtr->isData             = rplInfo->isData();
            rplInfoPtr->importName         = rplInfo->getName().c_str();
            rplInfoMap[rplInfo->getName()] = rplInfoPtr;
            curReloc->importEntry          = rplInfoPtr;
        }
    }
    module_data.linking_entries        = relocation_data.get();
    module_data.number_linking_entries = relocationCount;

    module->relocationDataStruct = std::move(relocation_data);
    module->rplDataStruct        = std::move(rpl_data);

    return true;
}
