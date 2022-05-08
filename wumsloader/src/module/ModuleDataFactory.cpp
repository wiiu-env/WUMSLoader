/****************************************************************************
 * Copyright (C) 2022 Maschell
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

#include "ModuleDataFactory.h"
#include "fs/FileUtils.h"
#include "utils/ElfUtils.h"
#include "utils/OnLeavingScope.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include <coreinit/cache.h>
#include <coreinit/memdefaultheap.h>
#include <map>
#include <string>
#include <wums.h>

using namespace ELFIO;

std::optional<std::shared_ptr<ModuleData>> ModuleDataFactory::load(const std::string &path) {
    elfio reader;
    auto moduleData = make_shared_nothrow<ModuleData>();
    if (!moduleData) {
        DEBUG_FUNCTION_LINE_ERR("Failed to alloc module data");
        return {};
    }

    uint8_t *buffer = nullptr;
    uint32_t fsize  = 0;
    if (LoadFileToMem(path, &buffer, &fsize) < 0) {
        DEBUG_FUNCTION_LINE_ERR("Failed to load file");
        return {};
    }

    auto cleanupBuffer = onLeavingScope([buffer]() { MEMFreeToDefaultHeap(buffer); });

    // Load ELF data
    if (!reader.load(reinterpret_cast<char *>(buffer), fsize)) {
        DEBUG_FUNCTION_LINE_ERR("Can't find or process %s", path.c_str());
        return {};
    }
    uint32_t sec_num = reader.sections.size();

    auto destinations = make_unique_nothrow<uint8_t *[]>(sec_num);
    if (!destinations) {
        DEBUG_FUNCTION_LINE_ERR("Failed alloc memory for destinations array");
        return {};
    }

    uint32_t totalSize = 0;

    uint32_t text_size = 0;
    uint32_t data_size = 0;

    for (uint32_t i = 0; i < sec_num; ++i) {
        auto *psec = reader.sections[i];
        if (psec->get_type() == 0x80000002) {
            continue;
        }

        if ((psec->get_type() == SHT_PROGBITS || psec->get_type() == SHT_NOBITS) && (psec->get_flags() & SHF_ALLOC)) {
            uint32_t sectionSize = psec->get_size();
            auto address         = (uint32_t) psec->get_address();
            if ((address >= 0x02000000) && address < 0x10000000) {
                text_size += sectionSize;
            } else if ((address >= 0x10000000) && address < 0xC0000000) {
                data_size += sectionSize;
            }
            if (psec->get_name().rfind(".wums.", 0) == 0) {
                data_size += sectionSize;
            }
        }
    }

    auto data = make_unique_nothrow<uint8_t[]>(text_size + data_size);
    if (!data) {
        DEBUG_FUNCTION_LINE_ERR("Failed to alloc memory for the .text section (%d bytes)", text_size);
        return {};
    }

    DEBUG_FUNCTION_LINE("Allocated %d kb", (text_size + data_size) / 1024);

    void *text_data = data.get();
    void *data_data = (void *) ((uint32_t) data.get() + text_size);
    auto baseOffset = (uint32_t) data.get();

    uint32_t entrypoint = (uint32_t) text_data + (uint32_t) reader.get_entry() - 0x02000000;

    for (uint32_t i = 0; i < sec_num; ++i) {
        section *psec = reader.sections[i];
        if (psec->get_type() == 0x80000002) {
            continue;
        }

        if ((psec->get_type() == SHT_PROGBITS || psec->get_type() == SHT_NOBITS) && (psec->get_flags() & SHF_ALLOC)) {
            uint32_t sectionSize = psec->get_size();

            totalSize += sectionSize;

            auto address = (uint32_t) psec->get_address();

            destinations[psec->get_index()] = (uint8_t *) baseOffset;

            uint32_t destination = baseOffset + address;
            if ((address >= 0x02000000) && address < 0x10000000) {
                destination -= 0x02000000;
                destinations[psec->get_index()] -= 0x02000000;
                baseOffset += sectionSize;
            } else if ((address >= 0x10000000) && address < 0xC0000000) {
                destination -= 0x10000000;
                destinations[psec->get_index()] -= 0x10000000;
            } else if (address >= 0xC0000000) {
                destination -= 0xC0000000;
                destinations[psec->get_index()] -= 0xC0000000;
            } else {
                DEBUG_FUNCTION_LINE_ERR("Unhandled case");
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

            if (psec->get_name() == ".bss") {
                moduleData->setBSSLocation(destination, sectionSize);
                memset(reinterpret_cast<void *>(destination), 0, sectionSize);

            } else if (psec->get_name() == ".sbss") {
                moduleData->setSBSSLocation(destination, sectionSize);
                memset(reinterpret_cast<void *>(destination), 0, sectionSize);
            }
            auto sectionInfo = make_shared_nothrow<SectionInfo>(psec->get_name(), destination, sectionSize);
            if (!sectionInfo) {
                DEBUG_FUNCTION_LINE_ERR("Failed to allocate memory for section info");
                return {};
            }
            moduleData->addSectionInfo(sectionInfo);
            DEBUG_FUNCTION_LINE("Saved %s section info. Location: %08X size: %08X", psec->get_name().c_str(), destination, sectionSize);

            DCFlushRange((void *) destination, sectionSize);
            ICInvalidateRange((void *) destination, sectionSize);
        }
    }

    for (uint32_t i = 0; i < sec_num; ++i) {
        section *psec = reader.sections[i];
        if ((psec->get_type() == SHT_PROGBITS || psec->get_type() == SHT_NOBITS) && (psec->get_flags() & SHF_ALLOC)) {
            DEBUG_FUNCTION_LINE("Linking (%d)... %s", i, psec->get_name().c_str());
            if (!linkSection(reader, psec->get_index(), (uint32_t) destinations[psec->get_index()], (uint32_t) text_data, (uint32_t) data_data, nullptr, 0)) {
                DEBUG_FUNCTION_LINE_ERR("elfLink failed");
                return std::nullopt;
            }
        }
    }
    getImportRelocationData(moduleData, reader, destinations.get());

    auto secInfo = moduleData->getSectionInfo(".wums.exports");
    if (secInfo && secInfo.value()->getSize() > 0) {
        size_t entries_count = secInfo.value()->getSize() / sizeof(wums_entry_t);
        auto *entries        = (wums_entry_t *) secInfo.value()->getAddress();
        if (entries != nullptr) {
            for (size_t j = 0; j < entries_count; j++) {
                wums_entry_t *exp = &entries[j];
                DEBUG_FUNCTION_LINE("Saving export of type %08X, name %s, target: %08X", exp->type, exp->name, exp->address);
                auto exportData = make_unique_nothrow<ExportData>(exp->type, exp->name, exp->address);
                if (!exportData) {
                    DEBUG_FUNCTION_LINE_ERR("Failed to alloc ExportData");
                    return {};
                }
                moduleData->addExportData(std::move(exportData));
            }
        }
    }

    secInfo = moduleData->getSectionInfo(".wums.hooks");
    if (secInfo && secInfo.value()->getSize() > 0) {
        size_t entries_count = secInfo.value()->getSize() / sizeof(wums_hook_t);
        auto *hooks          = (wums_hook_t *) secInfo.value()->getAddress();
        if (hooks != nullptr) {
            for (size_t j = 0; j < entries_count; j++) {
                wums_hook_t *hook = &hooks[j];
                DEBUG_FUNCTION_LINE("Saving hook of type %08X, target: %08X", hook->type, hook->target);
                auto hookData = make_unique_nothrow<HookData>(hook->type, hook->target);
                if (!hookData) {
                    DEBUG_FUNCTION_LINE_ERR("Failed to alloc HookData");
                    return {};
                }
                moduleData->addHookData(std::move(hookData));
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
                        moduleData->setExportName(value);
                    } else if (key == "skipInitFini") {
                        if (value == "true") {
                            DEBUG_FUNCTION_LINE("skipInitFini = %s", value.c_str());
                            moduleData->setSkipInitFini(true);
                        } else {
                            moduleData->setSkipInitFini(false);
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
                            DEBUG_FUNCTION_LINE_ERR("Warning: Ignoring module - Unsupported WUMS version: %s.", value.c_str());
                            return std::nullopt;
                        }
                    }
                }
                curEntry += strlen(curEntry) + 1;
            }
        }
    }

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
                    Elf64_Addr value    = 0;
                    Elf_Xword size      = 0;
                    unsigned char bind  = 0;
                    unsigned char type  = 0;
                    Elf_Half section    = 0;
                    unsigned char other = 0;
                    if (symbols.get_symbol(j, name, value, size, bind, type, section, other)) {
                        if (type == STT_FUNC) { // We only care about functions.
                            auto sectionVal = reader.sections[section];
                            auto offsetVal  = value - sectionVal->get_address();
                            auto sectionOpt = moduleData->getSectionInfo(sectionVal->get_name());
                            if (!sectionOpt.has_value()) {
                                continue;
                            }
                            auto finalAddress       = offsetVal + sectionOpt.value()->getAddress();
                            auto functionSymbolData = make_shared_nothrow<FunctionSymbolData>(name, (void *) finalAddress, (uint32_t) size);
                            if (!functionSymbolData) {
                                DEBUG_FUNCTION_LINE_ERR("Failed to alloc FunctionSymbolData");
                                return std::nullopt;
                            } else {
                                moduleData->addFunctionSymbolData(std::move(functionSymbolData));
                            }
                        }
                    }
                }
                break;
            }
        }
    }

    DCFlushRange((void *) text_data, text_size);
    ICInvalidateRange((void *) text_data, text_size);
    DCFlushRange((void *) data_data, data_size);
    ICInvalidateRange((void *) data_data, data_size);

    if (totalSize > text_size + data_size) {
        DEBUG_FUNCTION_LINE_ERR("We didn't allocate enough memory!!");
        return std::nullopt;
    }

    moduleData->setDataPtr(std::move(data), totalSize);
    moduleData->setEntrypoint(entrypoint);
    DEBUG_FUNCTION_LINE("Saved entrypoint as %08X", entrypoint);
    DEBUG_FUNCTION_LINE("Saved startAddress as %08X", (uint32_t) data.get());
    DEBUG_FUNCTION_LINE("Saved endAddress as %08X", (uint32_t) data.get() + totalSize);

    DEBUG_FUNCTION_LINE("Loaded %s size: %d kilobytes", path.c_str(), totalSize / 1024);

    return moduleData;
}

bool ModuleDataFactory::getImportRelocationData(std::shared_ptr<ModuleData> &moduleData, ELFIO::elfio &reader, uint8_t **destinations) {
    std::map<uint32_t, std::shared_ptr<ImportRPLInformation>> infoMap;

    uint32_t sec_num = reader.sections.size();

    for (uint32_t i = 0; i < sec_num; ++i) {
        auto *psec = reader.sections[i];
        if (psec->get_type() == 0x80000002) {
            auto info = make_shared_nothrow<ImportRPLInformation>(psec->get_name());
            if (!info) {
                DEBUG_FUNCTION_LINE_ERR("Failed too allocate ImportRPLInformation");
                return false;
            }
            infoMap[i] = std::move(info);
        }
    }

    for (uint32_t i = 0; i < sec_num; ++i) {
        section *psec = reader.sections[i];
        if (psec->get_type() == SHT_RELA || psec->get_type() == SHT_REL) {
            relocation_section_accessor rel(reader, psec);
            for (uint32_t j = 0; j < (uint32_t) rel.get_entries_num(); ++j) {
                Elf64_Addr offset;
                Elf_Word type;
                Elf_Sxword addend;
                std::string sym_name;
                Elf64_Addr sym_value;
                Elf_Half sym_section_index;

                if (!rel.get_entry(j, offset, sym_value, sym_name, type, addend, sym_section_index)) {
                    DEBUG_FUNCTION_LINE_ERR("Failed to get relocation");
                    OSFatal("Failed to get relocation");
                    break;
                }

                auto adjusted_sym_value = (uint32_t) sym_value;
                if (adjusted_sym_value < 0xC0000000) {
                    continue;
                }

                uint32_t section_index = psec->get_info();
                if (!infoMap.contains(sym_section_index)) {
                    DEBUG_FUNCTION_LINE_ERR("Relocation is referencing a unknown section. %d destination: %08X sym_name %s", section_index, destinations[section_index], sym_name.c_str());
                    OSFatal("Relocation is referencing a unknown section.");
                }

                auto relocationData = make_unique_nothrow<RelocationData>(type,
                                                                          offset - 0x02000000,
                                                                          addend,
                                                                          (void *) (destinations[section_index] + 0x02000000),
                                                                          sym_name,
                                                                          infoMap[sym_section_index]);

                if (!relocationData) {
                    DEBUG_FUNCTION_LINE_ERR("Failed to alloc relocation data");
                    return false;
                }

                moduleData->addRelocationData(std::move(relocationData));
            }
        }
    }
    return true;
}

bool ModuleDataFactory::linkSection(elfio &reader, uint32_t section_index, uint32_t destination, uint32_t base_text, uint32_t base_data, relocation_trampoline_entry_t *trampoline_data,
                                    uint32_t trampoline_data_length) {
    uint32_t sec_num = reader.sections.size();

    for (uint32_t i = 0; i < sec_num; ++i) {
        section *psec = reader.sections[i];
        if (psec->get_info() == section_index) {
            DEBUG_FUNCTION_LINE_VERBOSE("Found relocation section %s", psec->get_name().c_str());
            relocation_section_accessor rel(reader, psec);
            for (uint32_t j = 0; j < (uint32_t) rel.get_entries_num(); ++j) {
                Elf64_Addr offset;
                Elf_Word type;
                Elf_Sxword addend;
                std::string sym_name;
                Elf64_Addr sym_value;
                Elf_Half sym_section_index;

                if (!rel.get_entry(j, offset, sym_value, sym_name, type, addend, sym_section_index)) {
                    DEBUG_FUNCTION_LINE_ERR("Failed to get relocation");
                    return false;
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
                    DEBUG_FUNCTION_LINE_ERR("Unhandled case %08X", adjusted_sym_value);
                    return false;
                }

                if (sym_section_index == SHN_ABS) {
                    //
                } else if (sym_section_index > SHN_LORESERVE) {
                    DEBUG_FUNCTION_LINE_ERR("NOT IMPLEMENTED: %04X", sym_section_index);
                    return false;
                }
                if (!ElfUtils::elfLinkOne(type, offset, addend, destination, adjusted_sym_value, trampoline_data, trampoline_data_length, RELOC_TYPE_FIXED)) {
                    DEBUG_FUNCTION_LINE_ERR("Link failed");
                    return false;
                }
            }
        }
    }
    return true;
}
