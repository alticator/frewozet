#include "ram_mapper.h"

#include <stddef.h>

const struct e820_info* ram_mapper_get_info(void) {
    return (const struct e820_info*)(uintptr_t)E820_INFO_ADDR;
}

int ram_mapper_available(void) {
    return ram_mapper_get_info()->signature == E820_SIG;
}

char* ram_mapper_type_to_string(uint32_t type) {
    switch (type) {
        case 1:
            return "Usable";
        case 2:
            return "Reserved";
        case 3:
            return "ACPI Reclaimable";
        case 4:
            return "ACPI NVS";
        case 5:
            return "Bad";
        default:
            return "Unknown";
    }
}

uint32_t ram_mapper_get_total_usable_memory(void) {
    const struct e820_info* info = ram_mapper_get_info();
    if (!ram_mapper_available()) {
        return 0;
    }
    uint32_t total = 0;
    for (size_t i = 0; i < info->count; i++) {
        const struct e820_entry* entry = &info->entries[i];
        if (entry->type == 1) {
            total += info->entries[i].length;
        }
    }
    return total;
}