#include "DynamicLinkingHelper.h"
#include <cstring>

dyn_linking_function_t *DynamicLinkingHelper::getOrAddFunctionEntryByName(dyn_linking_relocation_data_t *data, const char *functionName) {
    if (data == nullptr) {
        return nullptr;
    }
    if (functionName == nullptr) {
        return nullptr;
    }
    dyn_linking_function_t *result = nullptr;
    for (auto &function: data->functions) {
        dyn_linking_function_t *curEntry = &function;
        if (strlen(curEntry->functionName) == 0) {
            if (strlen(functionName) > DYN_LINK_FUNCTION_NAME_LENGTH) {
                DEBUG_FUNCTION_LINE("Failed to add function name, it's too long.\n");
                return nullptr;
            }
            strncpy(curEntry->functionName, functionName, DYN_LINK_FUNCTION_NAME_LENGTH);
            result = curEntry;
            break;
        }
        if (strncmp(curEntry->functionName, functionName, DYN_LINK_FUNCTION_NAME_LENGTH) == 0) {
            result = curEntry;
            break;
        }
    }
    return result;
}

dyn_linking_import_t *DynamicLinkingHelper::getOrAddFunctionImportByName(dyn_linking_relocation_data_t *data, const char *importName) {
    return getOrAddImport(data, importName, false);
}

dyn_linking_import_t *DynamicLinkingHelper::getOrAddDataImportByName(dyn_linking_relocation_data_t *data, const char *importName) {
    return getOrAddImport(data, importName, true);
}

dyn_linking_import_t *DynamicLinkingHelper::getOrAddImport(dyn_linking_relocation_data_t *data, const char *importName, bool isData) {
    if (importName == nullptr || data == nullptr) {
        return nullptr;
    }
    dyn_linking_import_t *result = nullptr;
    for (auto &import: data->imports) {
        dyn_linking_import_t *curEntry = &import;
        if (strlen(curEntry->importName) == 0) {
            if (strlen(importName) > DYN_LINK_IMPORT_NAME_LENGTH) {
                DEBUG_FUNCTION_LINE("Failed to add Import, it's too long.\n");
                return nullptr;
            }
            strncpy(curEntry->importName, importName, DYN_LINK_IMPORT_NAME_LENGTH);
            curEntry->isData = isData;
            result = curEntry;
            break;
        }
        if (strncmp(curEntry->importName, importName, DYN_LINK_IMPORT_NAME_LENGTH) == 0 && (curEntry->isData == isData)) {
            return curEntry;
        }
    }
    return result;
}

bool DynamicLinkingHelper::addRelocationEntry(dyn_linking_relocation_data_t *linking_data, dyn_linking_relocation_entry_t *linking_entries, uint32_t linking_entry_length,
                                              const RelocationData &relocationData) {
    return addRelocationEntry(linking_data, linking_entries, linking_entry_length, relocationData.getType(),
                              relocationData.getOffset(), relocationData.getAddend(), relocationData.getDestination(),
                              relocationData.getName(),
                              relocationData.getImportRPLInformation());
}

bool DynamicLinkingHelper::addRelocationEntry(dyn_linking_relocation_data_t *linking_data, dyn_linking_relocation_entry_t *linking_entries, uint32_t linking_entry_length, char type, size_t offset,
                                              int32_t addend, void *destination,
                                              const std::string &name, const ImportRPLInformation &rplInfo) {
    dyn_linking_import_t *importInfoGbl = DynamicLinkingHelper::getOrAddImport(linking_data, rplInfo.getName().c_str(), rplInfo.isData());
    if (importInfoGbl == nullptr) {
        DEBUG_FUNCTION_LINE("Getting import info failed. Probably maximum of %d rpl files to import reached.\n", DYN_LINK_IMPORT_LIST_LENGTH);
        return false;
    }

    dyn_linking_function_t *functionInfo = DynamicLinkingHelper::getOrAddFunctionEntryByName(linking_data, name.c_str());
    if (functionInfo == nullptr) {
        DEBUG_FUNCTION_LINE("Getting import info failed. Probably maximum of %d function to be relocated reached.\n", DYN_LINK_FUNCTION_LIST_LENGTH);
        return false;
    }

    return addRelocationEntry(linking_entries, linking_entry_length, type, offset, addend, destination, functionInfo,
                              importInfoGbl);
}

bool DynamicLinkingHelper::addRelocationEntry(dyn_linking_relocation_entry_t *linking_entries, uint32_t linking_entry_length, char type, size_t offset, int32_t addend, void *destination,
                                              dyn_linking_function_t *functionName,
                                              dyn_linking_import_t *importInfo) {
    for (uint32_t i = 0; i < linking_entry_length; i++) {
        dyn_linking_relocation_entry_t *curEntry = &(linking_entries[i]);
        if (curEntry->functionEntry != nullptr) {
            continue;
        }
        curEntry->type = type;
        curEntry->offset = offset;
        curEntry->addend = addend;
        curEntry->destination = destination;
        curEntry->functionEntry = functionName;
        curEntry->importEntry = importInfo;

        return true;
    }
    return false;
}
