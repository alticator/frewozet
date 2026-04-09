#include "pmm.h"

#include "ram_mapper.h"
#include "memory.h"

extern uint8_t _kernel_end;

static uint32_t* pmm_bitmap = 0;
static uint32_t pmm_total_pages = 0;
static uint32_t pmm_total_usable_pages = 0;
static uint32_t pmm_used_pages = 0;
static uint64_t pmm_total_usable_bytes = 0;

static uint32_t align_up_u32(uint32_t addr, uint32_t alignment) {
    uint32_t mask = alignment - 1;
    return (addr + mask) & ~mask;
}

static uint32_t align_down_u32(uint32_t addr, uint32_t alignment) {
    uint32_t mask = alignment - 1;
    return addr & ~mask;
}

static void pmm_set_bit(uint32_t page_index) {
    pmm_bitmap[page_index / 32] |= (1u << (page_index % 32));
}

static void pmm_clear_bit(uint32_t page_index) {
    pmm_bitmap[page_index / 32] &= ~(1u << (page_index % 32));
}

static int pmm_test_bit(uint32_t page_index) {
    return (pmm_bitmap[page_index / 32] >> (page_index % 32)) & 1u;
}

static void pmm_mark_range_used(uint32_t start_addr, uint32_t end_addr) {
    uint32_t start = align_down_u32(start_addr, PMM_PAGE_SIZE);
    uint32_t end = align_up_u32(end_addr, PMM_PAGE_SIZE);

    for (uint32_t addr = start; addr < end; addr += PMM_PAGE_SIZE) {
        uint32_t page = addr / PMM_PAGE_SIZE;
        if (page < pmm_total_pages && !pmm_test_bit(page)) {
            pmm_set_bit(page);
            pmm_used_pages++;
        }
    }
}

static void pmm_mark_range_free(uint32_t start_addr, uint32_t end_addr) {
    uint32_t start = align_up_u32(start_addr, PMM_PAGE_SIZE);
    uint32_t end = align_down_u32(end_addr, PMM_PAGE_SIZE);

    if (end <= start) {
        return;
    }

    for (uint32_t addr = start; addr < end; addr += PMM_PAGE_SIZE) {
        uint32_t page = addr / PMM_PAGE_SIZE;
        if (page < pmm_total_pages && pmm_test_bit(page)) {
            pmm_clear_bit(page);
            pmm_used_pages--;
        } else {
        }
    }
}

void pmm_init(void) {
    if (!ram_mapper_available()) {
        return;
    }
    const struct e820_info* info = ram_mapper_get_info();

    uint64_t highest_addr = 0;
    pmm_total_usable_bytes = 0;
    pmm_total_usable_pages = 0;

    for (uint16_t i = 0; i < info->count; i++) {
        const struct e820_entry* entry = &info->entries[i];
        uint64_t end = entry->base + entry->length;
        if (end > highest_addr) {
            highest_addr = end;
        }
        if (entry->type == 1) {
            pmm_total_usable_bytes += entry->length;
        }
    }
    if (highest_addr > 0x100000000ull) {
        highest_addr = 0x100000000ull;
    }
    pmm_total_pages = (uint32_t)(highest_addr / PMM_PAGE_SIZE);
    if (pmm_total_pages == 0) {
        return;
    }

    uint32_t bitmap_words = (pmm_total_pages + 31) / 32;
    uint32_t bitmap_bytes = bitmap_words * sizeof(uint32_t);

    pmm_bitmap = (uint32_t*)kmalloc_aligned(bitmap_bytes, PMM_PAGE_SIZE);
    if (!pmm_bitmap) {
        return;
    }
    for (uint32_t i = 0; i < bitmap_words; i++) {
        pmm_bitmap[i] = 0xFFFFFFFFu;
    }
    pmm_used_pages = pmm_total_pages;
    for (uint16_t i = 0; i < info->count; i++) {
        const struct e820_entry* entry = &info->entries[i];
        if (entry->type != 1) {
            continue;
        }
        if (entry->base > 0x100000000ull) {
            continue;
        }
        uint64_t region_end_64 = entry->base + entry->length;
        if (region_end_64 > 0x100000000ull) {
            region_end_64 = 0x100000000ull;
        }
        uint32_t start = (uint32_t)entry->base;
        uint32_t end = (uint32_t)region_end_64;
        pmm_mark_range_free(start, end);
    }
    pmm_mark_range_used(0x00000000u, 0x00100000u);
    uint32_t kernel_end = (uint32_t)&_kernel_end;
    pmm_mark_range_used(0x00000000, kernel_end);
    uint32_t bitmap_start = (uint32_t)pmm_bitmap;
    pmm_mark_range_used(bitmap_start, bitmap_start + bitmap_bytes);
}

void* pmm_alloc_page(void) {
    if (!pmm_bitmap) {
        return 0;
    }
    for (uint32_t page = 0; page < pmm_total_pages; page++) {
        if (!pmm_test_bit(page)) {
            pmm_set_bit(page);
            pmm_used_pages++;
            return (void*)(page * PMM_PAGE_SIZE);
        }
    }
    return 0;
}

void pmm_free_page(void* page_ptr) {
    if (!pmm_bitmap || !page_ptr) {
        return;
    }
    uint32_t addr = (uint32_t)page_ptr;
    if (addr % PMM_PAGE_SIZE != 0) {
        return;
    }
    uint32_t page = addr / PMM_PAGE_SIZE;
    if (page >= pmm_total_pages) {
        return;
    }
    if (pmm_test_bit(page)) {
        pmm_clear_bit(page);
        pmm_used_pages--;
    }
}

uint32_t pmm_get_total_pages(void) {
    return pmm_total_pages;
}

uint32_t pmm_get_used_pages(void) {
    return pmm_used_pages;
}

uint32_t pmm_get_free_pages(void) {
    return pmm_total_pages - pmm_used_pages;
}

uint64_t pmm_get_total_usable_bytes(void) {
    return pmm_total_usable_bytes;
}

uint64_t pmm_get_total_free_bytes(void) {
    return (uint64_t)pmm_get_free_pages() * PMM_PAGE_SIZE;
}

uint64_t pmm_get_total_used_bytes(void) {
    return (uint64_t)pmm_get_used_pages() * PMM_PAGE_SIZE;
}