#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>

#define PMM_PAGE_SIZE 4096

void pmm_init(void);

void* pmm_alloc_page(void);
void pmm_free_page(void* page_ptr);

uint32_t pmm_get_total_pages(void);
uint32_t pmm_get_total_usable_pages(void);
uint32_t pmm_get_used_pages(void);
uint32_t pmm_get_free_pages(void);

uint64_t pmm_get_total_bytes(void);
uint64_t pmm_get_total_usable_bytes(void);
uint64_t pmm_get_total_used_bytes(void);
uint64_t pmm_get_total_free_bytes(void);

#endif