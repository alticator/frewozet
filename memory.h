#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

void memory_init(void);
void memory_enable_pmm_backend(void);

void* kmalloc_aligned(size_t size, size_t alignment);
void* kmalloc(size_t size);
void* kcalloc(size_t num, size_t size);

uint32_t memory_get_heap_start(void);
uint32_t memory_get_heap_current(void);
uint32_t memory_get_heap_limit(void);
uint32_t memory_get_heap_remaining(void);

int memory_is_pmm_backend_enabled(void);
uint32_t memory_get_heap_committed_bytes(void);
uint32_t memory_get_heap_used_bytes(void);
uint32_t memory_get_heap_free_bytes(void);

#endif