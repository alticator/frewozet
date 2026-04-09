#include "memory.h"
#include "terminal.h"
#include "ram_mapper.h"

extern uint32_t _kernel_end;

static uint32_t heap_start = 0;
static uint32_t heap_current = 0;
static uint32_t heap_limit = 0;

static uint32_t align_up(uint32_t addr, uint32_t alignment) {
    if (alignment == 0) {
        return addr; // No alignment needed
    }
    uint32_t mask = alignment - 1;
    return (addr + mask) & ~mask;
}

static int find_heap_region(uint32_t kernel_end_addr, uint32_t* out_start, uint32_t* out_limit) {
    if (!ram_mapper_available()) {
        return 0;
    }
    const struct e820_info* info = ram_mapper_get_info();
    for (uint16_t i = 0; i < info->count; i++) {
        const struct e820_entry* entry = &info->entries[i];
        if (entry->type != 1 || entry->base > 0xFFFFFFFFull) {
            continue; // Only consider usable memory regions that start within 4GB address space for 32-bit mode
        }
        uint64_t region_base_64 = entry->base;
        uint64_t region_end_64 = entry->base + entry->length;
        if (region_end_64 > 0x100000000ull) {
            region_end_64 = 0x100000000ull; // Clamp to 4GB limit for 32-bit mode
        }
        if (region_end_64 <= region_base_64) {
            continue; // Invalid region
        }
        uint32_t region_base = (uint32_t)region_base_64;
        uint32_t region_end = (uint32_t)region_end_64;
        if (kernel_end_addr >= region_base && kernel_end_addr < region_end) {
            // The kernel occupies part of this region, so we can only use the memory after the kernel
            uint32_t candidate_start = align_up(kernel_end_addr, 16);
            // Ensure the candidate start is within the region
            if (candidate_start < region_end) {
                *out_start = candidate_start;
                *out_limit = region_end;
                return 1; // Found a suitable region
            }
        }
    }
    return 0; // No suitable region found
}

void memory_init(void) {
    uint32_t kernel_end_addr = (uint32_t)&_kernel_end;
    if (find_heap_region(kernel_end_addr, &heap_start, &heap_limit)) {
        heap_current = heap_start;
    } else {
        // No suitable region found, fallback to a default heap start (e.g., 1MB)
        heap_start = align_up(kernel_end_addr, 16);
        heap_current = heap_start;
        heap_limit = 0x90000; // Kernel at 0x10000, stack at 0x90000
    }
}

void* kmalloc_aligned(size_t size, size_t alignment) {
    if (size == 0) {
        return (void*)0;
    }
    if (alignment == 0) {
        alignment = 16; // Default alignment
    }
    uint32_t aligned = align_up(heap_current, (uint32_t)alignment);
    if (aligned < heap_current) {
        return (void*)0; // Overflow
    }
    if (aligned + (uint32_t)size > heap_limit) {
        return (void*)0; // Not enough space
    }
    void* addr = (void*)aligned;
    heap_current = aligned + (uint32_t)size;
    return addr;
}

void* kmalloc(size_t size) {
    return kmalloc_aligned(size, 16);
}

void* kcalloc(size_t num, size_t size) {
    if (num == 0 || size == 0) {
        return (void*)0;
    }
    if (num > (size_t)-1 / size) {
        return (void*)0; // Overflow
    }
    size_t total_size = num * size;
    void* ptr = kmalloc(total_size);
    if (!ptr) {
        return (void*)0; // Allocation failed
    }
    for (size_t i = 0; i < total_size; i++) {
        ((uint8_t*)ptr)[i] = 0;
    }
    return ptr;
}

uint32_t memory_get_heap_start(void) {
    return heap_start;
}

uint32_t memory_get_heap_current(void) {
    return heap_current;
}

uint32_t memory_get_heap_limit(void) {
    return heap_limit;
}

uint32_t memory_get_heap_remaining(void) {
    if (heap_current >= heap_limit) {
        return 0;
    }
    return heap_limit - heap_current;
}