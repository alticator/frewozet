#ifndef RAM_MAPPER_H
#define RAM_MAPPER_H

#include <stdint.h>

#define E820_INFO_ADDR 0x00000500
#define E820_SIG       0x324F3845u

struct __attribute__((packed)) e820_entry {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi_attrs;
};

struct __attribute__((packed)) e820_info {
    uint32_t signature;
    uint16_t count;
    uint16_t reserved;
    struct e820_entry entries[];
};

const struct e820_info* ram_mapper_get_info(void);
int ram_mapper_available(void);
char* ram_mapper_type_to_string(uint32_t type);
uint32_t ram_mapper_get_total_usable_memory(void);

#endif