#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

#define PAGE_PRESENT  0x001
#define PAGE_WRITABLE 0x002
#define PAGE_USER     0x004

#define PAGE_SIZE 4096
#define PAGE_ENTRIES 1024

void paging_remove_identity_map(void);
void paging_init(void);

int paging_map_page(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags);
void paging_unmap_page(uint32_t virt_addr);
uint32_t paging_translate(uint32_t virt_addr);

int paging_alloc_page(uint32_t virt_addr, uint32_t flags);

#endif