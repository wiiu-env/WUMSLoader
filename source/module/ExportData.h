#pragma  once

#include <wums.h>

class ExportData {

public:
    ExportData(wums_entry_type_t type, const std::string &name, const void *address) {
        this->type = type;
        this->name = name;
        this->address = address;
    }

    [[nodiscard]] wums_entry_type_t getType() const {
        return type;
    }

    [[nodiscard]] const void *getAddress() const {
        return address;
    }

    [[nodiscard]] const std::string getName() const {
        return name;
    }

private:
    wums_entry_type_t type;
    std::string name;
    const void *address;
};