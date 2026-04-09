#include "terminal.h"
#include "idt.h"
#include "pic.h"
#include "shell.h"
#include "memory.h"
#include "pmm.h"

void kmain(void) {
    volatile uint16_t* const vga = (volatile uint16_t*)0xB8000;
    const char msg[] = "Frewozet Kernel loaded, trying to start terminal....";

    for (int i = 0; msg[i] != '\0'; i++) {
        vga[i+1200] = ((uint16_t)0x07 << 8) | (uint8_t)msg[i];
    }

    terminal_init();
    terminal_write("Alticator Frewozet\n    Development Preview\n\n");
    terminal_write("Starting error handling system....");
    idt_init();
    terminal_write("  OK\n");
    terminal_write("Starting PIC remapping....");
    pic_remap(32, 40);
    terminal_write("  OK\n");
    terminal_write("Initializing memory....");
    memory_init();
    terminal_write("  OK\n");
    terminal_write("Initializing physical memory management....");
    pmm_init();
    terminal_write("  OK\n");
    terminal_write("Enabling interrupts....");
    __asm__ __volatile__("sti");
    terminal_write("  OK\n\n");
    terminal_write("Frewozet Kernel is ready. Starting Frewozet Shell....\n\n");
    shell_init();

    while (1) {
        __asm__ __volatile__("hlt");
    }
}