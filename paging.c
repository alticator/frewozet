#include "paging.h"

#include "pmm.h"
#include "mmu.h"
#include "string.h"
#include <stdint.h>

#define PAGE_DIR_PHYS  0x00009000

static uint32_t* paging_get_page_directory(void) {
    return (uint32_t*)PHYS_TO_VIRT(PAGE_DIR_PHYS);
}

static inline uint32_t paging_dir_index(uint32_t virt_addr) {
    return virt_addr >> 22;
}

static inline uint32_t paging_table_index(uint32_t virt_addr) {
    return (virt_addr >> 12) & 0x3FF;
}

static inline uint32_t paging_align_down(uint32_t addr) {
    return addr & 0xFFFFF000u;
}

static void paging_invalidate_page(uint32_t virt_addr) {
    __asm__ __volatile__("invlpg (%0)" : : "r"((void*)virt_addr) : "memory");
}

static uint32_t* paging_get_page_table(uint32_t virt_addr, int create, uint32_t table_flags) {
    uint32_t* page_directory = paging_get_page_directory();
    uint32_t pd_index = paging_dir_index(virt_addr);
    uint32_t pde = page_directory[pd_index];

    if (pde & PAGE_PRESENT) {
        uint32_t table_phys = pde & 0xFFFFF000u;
        return (uint32_t*)PHYS_TO_VIRT(table_phys);
    }

    if (!create) {
        return 0;
    }

    void* new_table_phys_ptr = pmm_alloc_page();
    if (!new_table_phys_ptr) {
        return 0;
    }

    uint32_t new_table_phys = (uint32_t)new_table_phys_ptr;
    uint32_t* new_table_virt = (uint32_t*)PHYS_TO_VIRT(new_table_phys);

    memset(new_table_virt, 0, PAGE_SIZE);

    page_directory[pd_index] = (new_table_phys & 0xFFFFF000u) | (table_flags & 0xFFFu) | PAGE_PRESENT;
    paging_invalidate_page((uint32_t)&page_directory[pd_index]);

    return new_table_virt;
}

void paging_remove_identity_map(void) {
    uint32_t* page_directory = paging_get_page_directory();
    page_directory[0] = 0;

    uint32_t cr3;
    __asm__ __volatile__("mov %%cr3, %0" : "=r"(cr3));
    __asm__ __volatile__("mov %0, %%cr3" : : "r"(cr3));
}

void paging_init(void) {
    /* Nothing major yet: bootloader already enabled paging.
       This function exists to make paging a formal subsystem entry point. */
}

int paging_map_page(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags) {
    virt_addr = paging_align_down(virt_addr);
    phys_addr = paging_align_down(phys_addr);

    uint32_t table_flags = PAGE_WRITABLE;
    if (flags & PAGE_USER) {
        table_flags |= PAGE_USER;
    }

    uint32_t* page_table = paging_get_page_table(virt_addr, 1, table_flags);
    if (!page_table) {
        return 0;
    }

    uint32_t pt_index = paging_table_index(virt_addr);
    page_table[pt_index] = phys_addr | (flags & 0xFFFu) | PAGE_PRESENT;

    paging_invalidate_page(virt_addr);
    return 1;
}

void paging_unmap_page(uint32_t virt_addr) {
    virt_addr = paging_align_down(virt_addr);

    uint32_t* page_table = paging_get_page_table(virt_addr, 0, 0);
    if (!page_table) {
        return;
    }

    uint32_t pt_index = paging_table_index(virt_addr);
    page_table[pt_index] = 0;

    paging_invalidate_page(virt_addr);
}

uint32_t paging_translate(uint32_t virt_addr) {
    uint32_t* page_directory = paging_get_page_directory();
    uint32_t pd_index = paging_dir_index(virt_addr);
    uint32_t pde = page_directory[pd_index];

    if (!(pde & PAGE_PRESENT)) {
        return 0;
    }

    uint32_t table_phys = pde & 0xFFFFF000u;
    uint32_t* page_table = (uint32_t*)PHYS_TO_VIRT(table_phys);

    uint32_t pt_index = paging_table_index(virt_addr);
    uint32_t pte = page_table[pt_index];

    if (!(pte & PAGE_PRESENT)) {
        return 0;
    }

    uint32_t phys_base = pte & 0xFFFFF000u;
    return phys_base | (virt_addr & 0xFFFu);
}

int paging_alloc_page(uint32_t virt_addr, uint32_t flags) {
    void* phys_page = pmm_alloc_page();
    if (!phys_page) {
        return 0;
    }

    if (!paging_map_page(virt_addr, (uint32_t)phys_page, flags)) {
        pmm_free_page(phys_page);
        return 0;
    }

    uint8_t* virt_page = (uint8_t*)virt_addr;
    memset(virt_page, 0, PAGE_SIZE);
    return 1;
}