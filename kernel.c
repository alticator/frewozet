#include <stdint.h>

void kmain(void) {
    volatile uint16_t* vga = (uint16_t*)0xB8000;
    const char* msg = "Hello, World!";

    vga[3] = 0x074B;   // 'K'

    for (int i = 0; msg[i] != '\0'; i++) {
        vga[i] = ((uint16_t)0x07 << 8) | (uint8_t)msg[i];
    }

    for (;;) {
        __asm__ __volatile__("hlt");
    }
}