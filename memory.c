#include "memory.h"
#include "startup_error.h"
#include "ram_mapper.h"
#include "string.h"
#include "pmm.h"
#include "mmu.h"
#include "paging.h"

extern uint32_t _kernel_end;

#define MEMORY_DEFAULT_ALIGNMENT 16
#define MEMORY_HEAP_GROW_PAGES 4
#define MEMORY_MIN_SPLIT 16

#define EARLY_MAPPED_PHYS_LIMIT 0x00400000u

#define KERNEL_HEAP_START 0xC1000000u
#define KERNEL_HEAP_END   0xE0000000u

typedef struct heap_block {
    size_t size;
    uint32_t free;
    struct heap_block* next;
} heap_block_t;

static uint32_t bootstrap_heap_start = 0;
static uint32_t bootstrap_heap_current = 0;
static uint32_t bootstrap_heap_limit = 0;

static heap_block_t* heap_head = 0;
static int memory_pmm_backend_enabled = 0;

static uint32_t heap_virtual_current = KERNEL_HEAP_START;
static uint32_t heap_committed_bytes = 0;

static uint32_t align_up_u32(uint32_t value, uint32_t alignment) {
    if (alignment == 0) {
        return value;
    }

    uint32_t mask = alignment - 1;
    return (value + mask) & ~mask;
}

static size_t align_up_size(size_t value, size_t alignment) {
    size_t mask = alignment - 1;
    return (value + mask) & ~mask;
}

static int find_bootstrap_region(uint32_t kernel_end_phys, uint32_t* out_start, uint32_t* out_limit) {
    if (!ram_mapper_available()) {
        return 0;
    }

    const struct e820_info* info = ram_mapper_get_info();

    for (uint16_t i = 0; i < info->count; i++) {
        const struct e820_entry* entry = &info->entries[i];

        if (entry->type != 1 || entry->base >= 0x100000000ull) {
            continue;
        }

        uint64_t region_end_64 = entry->base + entry->length;
        if (region_end_64 > 0x100000000ull) {
            region_end_64 = 0x100000000ull;
        }

        uint32_t region_base = (uint32_t)entry->base;
        uint32_t region_end = (uint32_t)region_end_64;

        if (kernel_end_phys >= region_base && kernel_end_phys < region_end) {
            uint32_t start = align_up_u32(kernel_end_phys, MEMORY_DEFAULT_ALIGNMENT);

            if (start < region_end) {
                *out_start = start;
                *out_limit = region_end;
                return 1;
            }
        }
    }

    return 0;
}

void* bootstrap_alloc_aligned(size_t size, size_t alignment) {
    if (size == 0) {
        return 0;
    }

    if (alignment == 0) {
        alignment = MEMORY_DEFAULT_ALIGNMENT;
    }

    uint32_t aligned = align_up_u32(bootstrap_heap_current, (uint32_t)alignment);

    if (aligned < bootstrap_heap_current) {
        return 0;
    }

    if (aligned + (uint32_t)size < aligned) {
        return 0;
    }

    if (aligned + (uint32_t)size > bootstrap_heap_limit) {
        return 0;
    }

    void* result = (void*)aligned;
    bootstrap_heap_current = aligned + (uint32_t)size;
    return result;
}

static void heap_add_region(void* region, size_t region_size) {
    if (!region || region_size <= sizeof(heap_block_t)) {
        return;
    }

    heap_block_t* block = (heap_block_t*)region;
    block->size = region_size - sizeof(heap_block_t);
    block->free = 1;
    block->next = 0;

    if (!heap_head) {
        heap_head = block;
        return;
    }

    heap_block_t* current = heap_head;
    while (current->next) {
        current = current->next;
    }

    current->next = block;
}

static heap_block_t* heap_find_free_block(size_t size) {
    heap_block_t* current = heap_head;

    while (current) {
        if (current->free && current->size >= size) {
            return current;
        }

        current = current->next;
    }

    return 0;
}

static void heap_split_block(heap_block_t* block, size_t size) {
    if (!block) {
        return;
    }

    size_t aligned_size = align_up_size(size, MEMORY_DEFAULT_ALIGNMENT);

    if (block->size <= aligned_size + sizeof(heap_block_t) + MEMORY_MIN_SPLIT) {
        return;
    }

    uint8_t* raw = (uint8_t*)block;
    heap_block_t* new_block = (heap_block_t*)(raw + sizeof(heap_block_t) + aligned_size);

    new_block->size = block->size - aligned_size - sizeof(heap_block_t);
    new_block->free = 1;
    new_block->next = block->next;

    block->size = aligned_size;
    block->next = new_block;
}

static uint32_t pages_needed_for(size_t bytes) {
    return (uint32_t)((bytes + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE);
}

static int heap_grow(size_t minimum_payload_size) {
    size_t bytes_needed = minimum_payload_size + sizeof(heap_block_t);
    uint32_t pages = pages_needed_for(bytes_needed);

    if (pages < MEMORY_HEAP_GROW_PAGES) {
        pages = MEMORY_HEAP_GROW_PAGES;
    }

    uint32_t region_size = pages * PMM_PAGE_SIZE;

    if (heap_virtual_current + region_size < heap_virtual_current) {
        return 0;
    }

    if (heap_virtual_current + region_size > KERNEL_HEAP_END) {
        return 0;
    }

    uint32_t region_start = heap_virtual_current;
    uint32_t virt = region_start;

    for (uint32_t i = 0; i < pages; i++) {
        void* phys_page = pmm_alloc_page();

        if (!phys_page) {
            return 0;
        }

        if (!paging_map_page(virt, (uint32_t)phys_page, PAGE_WRITABLE)) {
            pmm_free_page(phys_page);
            return 0;
        }

        virt += PMM_PAGE_SIZE;
    }

    heap_virtual_current += region_size;
    heap_committed_bytes += region_size;

    heap_add_region((void*)region_start, region_size);
    return 1;
}

void memory_init(void) {
    uint32_t kernel_end_phys = VIRT_TO_PHYS((uint32_t)&_kernel_end);

    uint32_t heap_start_phys = 0;
    uint32_t heap_limit_phys = 0;

    if (find_bootstrap_region(kernel_end_phys, &heap_start_phys, &heap_limit_phys)) {
        if (heap_limit_phys > EARLY_MAPPED_PHYS_LIMIT) {
            heap_limit_phys = EARLY_MAPPED_PHYS_LIMIT;
        }

        if (heap_start_phys >= heap_limit_phys) {
            heap_start_phys = align_up_u32(kernel_end_phys, MEMORY_DEFAULT_ALIGNMENT);
            heap_limit_phys = EARLY_MAPPED_PHYS_LIMIT;
        }

        bootstrap_heap_start = (uint32_t)PHYS_TO_VIRT(heap_start_phys);
        bootstrap_heap_current = bootstrap_heap_start;
        bootstrap_heap_limit = (uint32_t)PHYS_TO_VIRT(heap_limit_phys);
        return;
    }

    heap_start_phys = align_up_u32(kernel_end_phys, MEMORY_DEFAULT_ALIGNMENT);

    bootstrap_heap_start = (uint32_t)PHYS_TO_VIRT(heap_start_phys);
    bootstrap_heap_current = bootstrap_heap_start;
    bootstrap_heap_limit = (uint32_t)PHYS_TO_VIRT(EARLY_MAPPED_PHYS_LIMIT);
}

void memory_enable_pmm_backend(void) {
    if (memory_pmm_backend_enabled) {
        return;
    }

    if (!heap_grow(1)) {
        logStartupError("Failed to enable PMM-backed heap. Keeping bootstrap heap.");
        return;
    }

    memory_pmm_backend_enabled = 1;
}

void* kmalloc(size_t size) {
    if (size == 0) {
        return 0;
    }

    size = align_up_size(size, MEMORY_DEFAULT_ALIGNMENT);

    if (!memory_pmm_backend_enabled) {
        return bootstrap_alloc_aligned(size, MEMORY_DEFAULT_ALIGNMENT);
    }

    heap_block_t* block = heap_find_free_block(size);

    if (!block) {
        if (!heap_grow(size)) {
            return 0;
        }

        block = heap_find_free_block(size);
        if (!block) {
            return 0;
        }
    }

    heap_split_block(block, size);
    block->free = 0;

    return (void*)((uint8_t*)block + sizeof(heap_block_t));
}

void* kmalloc_aligned(size_t size, size_t alignment) {
    if (size == 0) {
        return 0;
    }

    if (!memory_pmm_backend_enabled) {
        return bootstrap_alloc_aligned(size, alignment);
    }

    if (alignment <= MEMORY_DEFAULT_ALIGNMENT) {
        return kmalloc(size);
    }

    size_t extra = alignment + sizeof(void*);
    uint8_t* raw = (uint8_t*)kmalloc(size + extra);

    if (!raw) {
        return 0;
    }

    uintptr_t base = (uintptr_t)(raw + sizeof(void*));
    uintptr_t aligned = (base + alignment - 1) & ~(uintptr_t)(alignment - 1);

    ((void**)aligned)[-1] = raw;
    return (void*)aligned;
}

void* kcalloc(size_t num, size_t size) {
    if (num == 0 || size == 0) {
        return 0;
    }

    if (num > ((size_t)-1) / size) {
        return 0;
    }

    size_t total_size = num * size;
    void* ptr = kmalloc(total_size);

    if (!ptr) {
        return 0;
    }

    memset(ptr, 0, total_size);
    return ptr;
}

uint32_t memory_get_heap_start(void) {
    return bootstrap_heap_start;
}

uint32_t memory_get_heap_current(void) {
    return bootstrap_heap_current;
}

uint32_t memory_get_heap_limit(void) {
    return bootstrap_heap_limit;
}

uint32_t memory_get_heap_remaining(void) {
    if (bootstrap_heap_current >= bootstrap_heap_limit) {
        return 0;
    }

    return bootstrap_heap_limit - bootstrap_heap_current;
}

int memory_is_pmm_backend_enabled(void) {
    return memory_pmm_backend_enabled;
}

uint32_t memory_get_heap_committed_bytes(void) {
    if (!memory_pmm_backend_enabled) {
        return 0;
    }

    return heap_committed_bytes;
}

uint32_t memory_get_heap_used_bytes(void) {
    if (!memory_pmm_backend_enabled || !heap_head) {
        return 0;
    }

    uint32_t total = 0;
    heap_block_t* current = heap_head;

    while (current) {
        if (!current->free) {
            total += (uint32_t)current->size;
        }

        current = current->next;
    }

    return total;
}

uint32_t memory_get_heap_free_bytes(void) {
    if (!memory_pmm_backend_enabled || !heap_head) {
        return 0;
    }

    uint32_t total = 0;
    heap_block_t* current = heap_head;

    while (current) {
        if (current->free) {
            total += (uint32_t)current->size;
        }

        current = current->next;
    }

    return total;
}