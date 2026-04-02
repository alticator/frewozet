#include "memory.h"
#include "terminal.h"

extern uint32_t _kernel_end;

static uint32_t heap_start;
static uint32_t heap_current;

static uint32_t align_up(uint32_t addr, uint32_t alignment) {
    if (alignment == 0) {
        return addr; // No alignment needed
    }
    uint32_t mask = alignment - 1;
    return (addr + mask) & ~mask;
}

void memory_init(void) {
    heap_start = align_up((uint32_t)&_kernel_end, 16);
    heap_current = heap_start;
}

void* kmalloc_aligned(size_t size, size_t alignment) {
    if (size == 0) {
        return (void*)0;
    }
    heap_current = align_up(heap_current, alignment);
    void* addr = (void*)heap_current;
    heap_current += (uint32_t)size;
    return addr;
}

void* kmalloc(size_t size) {
    return kmalloc_aligned(size, 16);
}

uint32_t memory_get_heap_start(void) {
    return heap_start;
}

uint32_t memory_get_heap_current(void) {
    return heap_current;
}