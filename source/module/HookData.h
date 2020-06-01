#pragma once

#include <wums.h>

class HookData {
public:
    HookData(wums_hook_type_t type, const void *target) {
        this->type = type;
        this->target = target;
    }

    wums_hook_type_t getType() const {
        return type;
    }

    const void *getTarget() const {
        return target;
    }

private:
    wums_hook_type_t type;
    const void *target;
};