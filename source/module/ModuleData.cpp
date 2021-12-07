#include "ModuleData.h"
#include "utils/StringTools.h"

std::string ModuleData::toString() const {
    std::string res = StringTools::strfmt("Entrypoint %08X, bss: %08X (%d), bss: %08X (%d)\n", getEntrypoint(), getBSSAddr(), getBSSSize(), getSBSSAddr(), getSBSSSize());
    for (auto const &reloc: relocation_data_list) {
        res += reloc->toString();
    }
    return res;
}
