#include "pmm.h"
#include "ram_mapper.h"
#include "memory.h"
#include "startup_error.h"
#include "mmu.h"

extern uint8_t _kernel_end;

static uint32_t* pmm_bitmap = 0;
static uint32_t pmm_total_pages = 0;
static uint32_t pmm_total_usable_pages = 0;
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
        }
    }
}

void pmm_init(void) {
    if (!ram_mapper_available()) {
        logStartupError("Could not map RAM. Physical Memory Manager was not started.");
        return;
    }

    const struct e820_info* info = ram_mapper_get_info();

    uint64_t highest_addr = 0;
    pmm_total_usable_bytes = 0;
    pmm_total_usable_pages = 0;

    for (uint16_t i = 0; i < info->count; i++) {
        const struct e820_entry* entry = &info->entries[i];
        uint64_t region_end = entry->base + entry->length;

        if (region_end > highest_addr) {
            highest_addr = region_end;
        }

        if (entry->type == 1) {
            pmm_total_usable_bytes += entry->length;

            if (entry->base < 0x100000000ull) {
                if (region_end > 0x100000000ull) {
                    region_end = 0x100000000ull;
                }

                uint32_t usable_start = align_up_u32((uint32_t)entry->base, PMM_PAGE_SIZE);
                uint32_t usable_end = align_down_u32((uint32_t)region_end, PMM_PAGE_SIZE);

                if (usable_end > usable_start) {
                    pmm_total_usable_pages += (usable_end - usable_start) / PMM_PAGE_SIZE;
                }
            }
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
        logStartupError("PMM bitmap allocation failed.");
        return;
    }

    for (uint32_t i = 0; i < bitmap_words; i++) {
        pmm_bitmap[i] = 0xFFFFFFFFu;
    }

    for (uint16_t i = 0; i < info->count; i++) {
        const struct e820_entry* entry = &info->entries[i];

        if (entry->type != 1 || entry->base >= 0x100000000ull) {
            continue;
        }

        uint64_t region_end = entry->base + entry->length;
        if (region_end > 0x100000000ull) {
            region_end = 0x100000000ull;
        }

        pmm_mark_range_free((uint32_t)entry->base, (uint32_t)region_end);
    }

    pmm_mark_range_used(0x00000000u, 0x00100000u);

    uint32_t kernel_end_phys = VIRT_TO_PHYS((uint32_t)&_kernel_end);
    pmm_mark_range_used(0x00000000u, kernel_end_phys);

    uint32_t bitmap_start_phys = VIRT_TO_PHYS((uint32_t)pmm_bitmap);
    pmm_mark_range_used(bitmap_start_phys, bitmap_start_phys + bitmap_bytes);
}

void* pmm_alloc_contiguous(uint32_t page_count) {
    if (!pmm_bitmap || page_count == 0) {
        return 0;
    }

    uint32_t run_start = 0;
    uint32_t run_len = 0;

    for (uint32_t page = 0; page < pmm_total_pages; page++) {
        if (!pmm_test_bit(page)) {
            if (run_len == 0) {
                run_start = page;
            }

            run_len++;

            if (run_len == page_count) {
                for (uint32_t i = 0; i < page_count; i++) {
                    pmm_set_bit(run_start + i);
                }

                return (void*)(run_start * PMM_PAGE_SIZE);
            }
        } else {
            run_len = 0;
        }
    }

    return 0;
}

void* pmm_alloc_page_below(uint32_t limit) {
    if (!pmm_bitmap) {
        return 0;
    }

    uint32_t max_page = limit / PMM_PAGE_SIZE;
    if (max_page > pmm_total_pages) {
        max_page = pmm_total_pages;
    }

    for (uint32_t page = 0; page < max_page; page++) {
        if (!pmm_test_bit(page)) {
            pmm_set_bit(page);
            return (void*)(page * PMM_PAGE_SIZE);
        }
    }

    return 0;
}

void* pmm_alloc_page(void) {
    return pmm_alloc_contiguous(1);
}

void pmm_free_contiguous(void* start_page, uint32_t page_count) {
    if (!pmm_bitmap || !start_page || page_count == 0) {
        return;
    }

    uint32_t start_addr = (uint32_t)start_page;

    if (start_addr % PMM_PAGE_SIZE != 0) {
        return;
    }

    uint32_t start_index = start_addr / PMM_PAGE_SIZE;

    if (start_index >= pmm_total_pages) {
        return;
    }

    for (uint32_t i = 0; i < page_count; i++) {
        uint32_t page = start_index + i;

        if (page >= pmm_total_pages) {
            break;
        }

        if (pmm_test_bit(page)) {
            pmm_clear_bit(page);
        }
    }
}

void pmm_free_page(void* page_ptr) {
    pmm_free_contiguous(page_ptr, 1);
}

uint32_t pmm_get_total_pages(void) {
    return pmm_total_pages;
}

uint32_t pmm_get_total_usable_pages(void) {
    return pmm_total_usable_pages;
}

uint32_t pmm_get_free_pages(void) {
    if (!pmm_bitmap) {
        return 0;
    }

    uint32_t free_pages = 0;

    for (uint32_t page = 0; page < pmm_total_pages; page++) {
        if (!pmm_test_bit(page)) {
            free_pages++;
        }
    }

    return free_pages;
}

uint32_t pmm_get_used_pages(void) {
    uint32_t free_pages = pmm_get_free_pages();

    if (pmm_total_usable_pages >= free_pages) {
        return pmm_total_usable_pages - free_pages;
    }

    return 0;
}

uint64_t pmm_get_total_bytes(void) {
    return (uint64_t)pmm_total_pages * PMM_PAGE_SIZE;
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