/****************************************************************************
 * Copyright (C) 2018 Maschell
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/

#include <string>
#include <vector>
#include <map>
#include <coreinit/cache.h>
#include <wums.h>
#include "ModuleDataFactory.h"
#include "utils/utils.h"
#include "ElfUtils.h"
#include "FunctionSymbolData.h"

using namespace ELFIO;

std::optional<std::shared_ptr<ModuleData>>
ModuleDataFactory::load(const std::string &path, uint32_t *destination_address_ptr, uint32_t maximum_size, relocation_trampolin_entry_t *trampolin_data, uint32_t trampolin_data_length) {
    elfio reader;
    std::shared_ptr<ModuleData> moduleData = std::make_shared<ModuleData>();

    // Load ELF data
    if (!reader.load(path)) {
        DEBUG_FUNCTION_LINE("Can't find or process %s", path.c_str());
        return std::nullopt;
    }

    uint32_t sec_num = reader.sections.size();

    auto **destinations = (uint8_t **) malloc(sizeof(uint8_t *) * sec_num);

    uint32_t baseOffset = *destination_address_ptr;

    uint32_t offset_text = baseOffset;
    uint32_t offset_data = offset_text;

    uint32_t entrypoint = offset_text + (uint32_t) reader.get_entry() - 0x02000000;

    uint32_t totalSize = 0;
    uint32_t endAddress = 0;

    for (uint32_t i = 0; i < sec_num; ++i) {
        section *psec = reader.sections[i];
        if (psec->get_type() == 0x80000002) {
            continue;
        }

        if ((psec->get_type() == SHT_PROGBITS || psec->get_type() == SHT_NOBITS) && (psec->get_flags() & SHF_ALLOC)) {
            uint32_t sectionSize = psec->get_size();

            totalSize += sectionSize;
            if (totalSize > maximum_size) {
                DEBUG_FUNCTION_LINE("Couldn't load setup module because it's too big.");
                return {};
            }

            auto address = (uint32_t) psec->get_address();

            destinations[psec->get_index()] = (uint8_t *) baseOffset;

            uint32_t destination = baseOffset + address;
            if ((address >= 0x02000000) && address < 0x10000000) {
                destination -= 0x02000000;
                destinations[psec->get_index()] -= 0x02000000;
                baseOffset += sectionSize;
                offset_data += sectionSize;
            } else if ((address >= 0x10000000) && address < 0xC0000000) {
                destination -= 0x10000000;
                destinations[psec->get_index()] -= 0x10000000;
            } else if (address >= 0xC0000000) {
                destination -= 0xC0000000;
                destinations[psec->get_index()] -= 0xC0000000;
            } else {
                DEBUG_FUNCTION_LINE("Unhandled case");
                free(destinations);
                return std::nullopt;
            }

            const char *p = reader.sections[i]->get_data();

            if (psec->get_type() == SHT_NOBITS) {
                DEBUG_FUNCTION_LINE("memset section %s %08X to 0 (%d bytes)", psec->get_name().c_str(), destination, sectionSize);
                memset((void *) destination, 0, sectionSize);
            } else if (psec->get_type() == SHT_PROGBITS) {
                DEBUG_FUNCTION_LINE("Copy section %s %08X -> %08X (%d bytes)", psec->get_name().c_str(), p, destination, sectionSize);
                memcpy((void *) destination, p, sectionSize);
            }

            //nextAddress = ROUNDUP(destination + sectionSize, 0x100);
            if (psec->get_name() == ".bss") {
                moduleData->setBSSLocation(destination, sectionSize);
                memset(reinterpret_cast<void *>(destination), 0, sectionSize);
            } else if (psec->get_name() == ".sbss") {
                moduleData->setSBSSLocation(destination, sectionSize);
                memset(reinterpret_cast<void *>(destination), 0, sectionSize);
            }
            auto sectionInfo = std::make_shared<SectionInfo>(psec->get_name(), destination, sectionSize);
            moduleData->addSectionInfo(sectionInfo);
            DEBUG_FUNCTION_LINE("Saved %s section info. Location: %08X size: %08X", psec->get_name().c_str(), destination, sectionSize);

            if (endAddress < destination + sectionSize) {
                endAddress = destination + sectionSize;
            }

            DCFlushRange((void *) destination, sectionSize);
            ICInvalidateRange((void *) destination, sectionSize);
        }
    }

    for (uint32_t i = 0; i < sec_num; ++i) {
        section *psec = reader.sections[i];
        if ((psec->get_type() == SHT_PROGBITS || psec->get_type() == SHT_NOBITS) && (psec->get_flags() & SHF_ALLOC)) {
            DEBUG_FUNCTION_LINE("Linking (%d)... %s", i, psec->get_name().c_str());
            if (!linkSection(reader, psec->get_index(), (uint32_t) destinations[psec->get_index()], offset_text, offset_data, trampolin_data, trampolin_data_length)) {
                DEBUG_FUNCTION_LINE("elfLink failed");
                free(destinations);
                return std::nullopt;
            }
        }
    }
    auto relocationData = getImportRelocationData(reader, destinations);

    for (auto const &reloc: relocationData) {
        moduleData->addRelocationData(reloc);
    }

    auto secInfo = moduleData->getSectionInfo(".wums.exports");
    if (secInfo && secInfo.value()->getSize() > 0) {
        size_t entries_count = secInfo.value()->getSize() / sizeof(wums_entry_t);
        auto *entries = (wums_entry_t *) secInfo.value()->getAddress();
        if (entries != nullptr) {
            for (size_t j = 0; j < entries_count; j++) {
                wums_entry_t *exp = &entries[j];
                DEBUG_FUNCTION_LINE("Saving export of type %08X, name %s, target: %08X"/*,pluginData.getPluginInformation()->getName().c_str()*/, exp->type, exp->name, (void *) exp->address);
                auto exportData = std::make_shared<ExportData>(exp->type, exp->name, exp->address);
                moduleData->addExportData(exportData);
            }
        }
    }

    secInfo = moduleData->getSectionInfo(".wums.hooks");
    if (secInfo && secInfo.value()->getSize() > 0) {
        size_t entries_count = secInfo.value()->getSize() / sizeof(wums_hook_t);
        auto *hooks = (wums_hook_t *) secInfo.value()->getAddress();
        if (hooks != nullptr) {
            for (size_t j = 0; j < entries_count; j++) {
                wums_hook_t *hook = &hooks[j];
                DEBUG_FUNCTION_LINE("Saving hook of type %08X, target: %08X"/*,pluginData.getPluginInformation()->getName().c_str()*/, hook->type, hook->target);
                auto hookData = std::make_shared<HookData>(hook->type, hook->target);
                moduleData->addHookData(hookData);
            }
        }
    }

    secInfo = moduleData->getSectionInfo(".wums.meta");
    if (secInfo && secInfo.value()->getSize() > 0) {
        auto *entries = (wums_entry_t *) secInfo.value()->getAddress();
        if (entries != nullptr) {

            char *curEntry = (char *) secInfo.value()->getAddress();
            while ((uint32_t) curEntry < (uint32_t) secInfo.value()->getAddress() + secInfo.value()->getSize()) {
                if (*curEntry == '\0') {
                    curEntry++;
                    continue;
                }

                auto firstFound = std::string(curEntry).find_first_of('=');
                if (firstFound != std::string::npos) {
                    curEntry[firstFound] = '\0';
                    std::string key(curEntry);
                    std::string value(curEntry + firstFound + 1);

                    if (key == "export_name") {
                        DEBUG_FUNCTION_LINE("export_name = %s", value.c_str());
                        moduleData->setExportName(value);
                    } else if (key == "skipEntrypoint") {
                        if (value == "true") {
                            DEBUG_FUNCTION_LINE("skipEntrypoint = %s", value.c_str());
                            moduleData->setSkipEntrypoint(true);
                        } else {
                            moduleData->setSkipEntrypoint(false);
                        }
                    } else if (key == "initBeforeRelocationDoneHook") {
                        if (value == "true") {
                            DEBUG_FUNCTION_LINE("initBeforeRelocationDoneHook = %s", value.c_str());
                            moduleData->setInitBeforeRelocationDoneHook(true);
                        } else {
                            moduleData->setInitBeforeRelocationDoneHook(false);
                        }
                    } else if (key == "wums") {
                        if (value != "0.3") {
                            DEBUG_FUNCTION_LINE("Warning: Ignoring module - Unsupported WUMS version: %s.\n", value.c_str());
                            return std::nullopt;
                        }
                    }
                }
                curEntry += strlen(curEntry) + 1;
            }
        }
    }

    char *strTable = (char *) endAddress;
    uint32_t strOffset = 0;

    // Get the symbol for functions.
    Elf_Half n = reader.sections.size();
    for (Elf_Half i = 0; i < n; ++i) {
        section *sec = reader.sections[i];
        if (SHT_SYMTAB == sec->get_type()) {
            symbol_section_accessor symbols(reader, sec);
            auto sym_no = (uint32_t) symbols.get_symbols_num();
            if (sym_no > 0) {
                for (Elf_Half j = 0; j < sym_no; ++j) {
                    std::string name;
                    Elf64_Addr value = 0;
                    Elf_Xword size = 0;
                    unsigned char bind = 0;
                    unsigned char type = 0;
                    Elf_Half section = 0;
                    unsigned char other = 0;
                    if (symbols.get_symbol(j, name, value, size, bind, type, section, other)) {
                        if (type == STT_FUNC) { // We only care about functions.
                            auto sectionVal = reader.sections[section];
                            auto offsetVal = value - sectionVal->get_address();
                            auto sectionOpt = moduleData->getSectionInfo(sectionVal->get_name());
                            if (!sectionOpt.has_value()) {
                                continue;
                            }
                            auto finalAddress = offsetVal + sectionOpt.value()->getAddress();

                            uint32_t stringSize = name.size() + 1;
                            memcpy(strTable + strOffset, name.c_str(), stringSize);
                            moduleData->addFunctionSymbolData(std::make_shared<FunctionSymbolData>(strTable + strOffset, (void *) finalAddress, (uint32_t) size));
                            strOffset += stringSize;
                            totalSize += stringSize;
                            endAddress += stringSize;
                        }
                    }
                }
                break;
            }
        }
    }

    DCFlushRange((void *) *destination_address_ptr, totalSize);
    ICInvalidateRange((void *) *destination_address_ptr, totalSize);

    free(destinations);

    moduleData->setEntrypoint(entrypoint);
    moduleData->setStartAddress(*destination_address_ptr);
    moduleData->setEndAddress(endAddress);
    DEBUG_FUNCTION_LINE("Saved entrypoint as %08X", entrypoint);
    DEBUG_FUNCTION_LINE("Saved startAddress as %08X", *destination_address_ptr);
    DEBUG_FUNCTION_LINE("Saved endAddress as %08X", endAddress);

    *destination_address_ptr = (*destination_address_ptr + totalSize + 0x100) & 0xFFFFFF00;

    return moduleData;
}

std::vector<std::shared_ptr<RelocationData>> ModuleDataFactory::getImportRelocationData(elfio &reader, uint8_t **destinations) {
    std::vector<std::shared_ptr<RelocationData>> result;
    std::map<uint32_t, std::string> infoMap;

    uint32_t sec_num = reader.sections.size();

    for (uint32_t i = 0; i < sec_num; ++i) {
        section *psec = reader.sections[i];
        if (psec->get_type() == 0x80000002) {
            infoMap[i] = psec->get_name();
        }
    }

    for (uint32_t i = 0; i < sec_num; ++i) {
        section *psec = reader.sections[i];
        if (psec->get_type() == SHT_RELA || psec->get_type() == SHT_REL) {
            DEBUG_FUNCTION_LINE("Found relocation section %s", psec->get_name().c_str());
            relocation_section_accessor rel(reader, psec);
            for (uint32_t j = 0; j < (uint32_t) rel.get_entries_num(); ++j) {
                Elf64_Addr offset;
                Elf_Word type;
                Elf_Sxword addend;
                std::string sym_name;
                Elf64_Addr sym_value;
                Elf_Half sym_section_index;

                if (!rel.get_entry(j, offset, sym_value, sym_name, type, addend, sym_section_index)) {
                    DEBUG_FUNCTION_LINE("Failed to get relocation");
                    break;
                }

                // uint32_t adjusted_sym_value = (uint32_t) sym_value;
                if (infoMap.count(sym_section_index) == 0) {
                    continue;
                }
                auto rplInfo = ImportRPLInformation::createImportRPLInformation(infoMap[sym_section_index]);
                if (!rplInfo) {
                    DEBUG_FUNCTION_LINE("Failed to create import information");
                    break;
                }

                uint32_t section_index = psec->get_info();

                // When these relocations are performed, we don't need the 0xC0000000 offset anymore.
                auto relocationData = std::make_shared<RelocationData>(type, offset - 0x02000000, addend, (void *) (destinations[section_index] + 0x02000000), sym_name, rplInfo.value());
                //relocationData->printInformation();
                result.push_back(relocationData);
            }
        }
    }
    return result;
}

bool ModuleDataFactory::linkSection(elfio &reader, uint32_t section_index, uint32_t destination, uint32_t base_text, uint32_t base_data, relocation_trampolin_entry_t *trampolin_data,
                                    uint32_t trampolin_data_length) {
    uint32_t sec_num = reader.sections.size();

    for (uint32_t i = 0; i < sec_num; ++i) {
        section *psec = reader.sections[i];
        if (psec->get_info() == section_index) {
            DEBUG_FUNCTION_LINE("Found relocation section %s", psec->get_name().c_str());
            relocation_section_accessor rel(reader, psec);
            for (uint32_t j = 0; j < (uint32_t) rel.get_entries_num(); ++j) {
                Elf64_Addr offset;
                Elf_Word type;
                Elf_Sxword addend;
                std::string sym_name;
                Elf64_Addr sym_value;
                Elf_Half sym_section_index;

                if (!rel.get_entry(j, offset, sym_value, sym_name, type, addend, sym_section_index)) {
                    DEBUG_FUNCTION_LINE("Failed to get relocation");
                    break;
                }

                auto adjusted_sym_value = (uint32_t) sym_value;
                if ((adjusted_sym_value >= 0x02000000) && adjusted_sym_value < 0x10000000) {
                    adjusted_sym_value -= 0x02000000;
                    adjusted_sym_value += base_text;
                } else if ((adjusted_sym_value >= 0x10000000) && adjusted_sym_value < 0xC0000000) {
                    adjusted_sym_value -= 0x10000000;
                    adjusted_sym_value += base_data;
                } else if (adjusted_sym_value >= 0xC0000000) {
                    // Skip imports
                    continue;
                } else if (adjusted_sym_value == 0x0) {
                    //
                } else {
                    DEBUG_FUNCTION_LINE("Unhandled case %08X", adjusted_sym_value);
                    return false;
                }

                if (sym_section_index == SHN_ABS) {
                    //
                } else if (sym_section_index > SHN_LORESERVE) {
                    DEBUG_FUNCTION_LINE("NOT IMPLEMENTED: %04X", sym_section_index);
                    return false;
                }

                if (!ElfUtils::elfLinkOne(type, offset, addend, destination, adjusted_sym_value, trampolin_data, trampolin_data_length, RELOC_TYPE_FIXED)) {
                    DEBUG_FUNCTION_LINE("Link failed");
                    return false;
                }
            }
            DEBUG_FUNCTION_LINE("done");
        }
    }
    return true;
}
