#include "terminal.h"
#include "idt.h"

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
    terminal_write("Frewozet is ready.\n\n");
    terminal_write("Frewozet >>>");

    __asm__ __volatile__("ud2");
    


    for (;;) {
        __asm__ __volatile__("hlt");
    }
}