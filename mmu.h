#ifndef MMU_H
#define MMU_H

#include <stdint.h>

#define KERNEL_VIRT_BASE 0xC0000000u

#define PHYS_TO_VIRT(addr) ((void*)((uint32_t)(addr) + KERNEL_VIRT_BASE))
#define VIRT_TO_PHYS(addr) ((uint32_t)(addr) - KERNEL_VIRT_BASE)

#endif