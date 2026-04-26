#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>

#define PMM_PAGE_SIZE 4096

void pmm_init(void);

void* pmm_alloc_page(void);
void pmm_free_page(void* page_ptr);
void* pmm_alloc_page_below(uint32_t limit);

void* pmm_alloc_contiguous(uint32_t page_count);
void pmm_free_contiguous(void* start_page, uint32_t page_count);

uint32_t pmm_get_total_pages(void);
uint32_t pmm_get_total_usable_pages(void);
uint32_t pmm_get_used_pages(void);
uint32_t pmm_get_free_pages(void);

uint64_t pmm_get_total_bytes(void);
uint64_t pmm_get_total_usable_bytes(void);
uint64_t pmm_get_total_used_bytes(void);
uint64_t pmm_get_total_free_bytes(void);

#endif