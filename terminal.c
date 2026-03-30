#include "terminal.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((volatile uint16_t*)0xB8000)

static size_t cursor_row;
static size_t cursor_col;
static uint8_t cursor_color;
static volatile uint16_t* buffer;

static inline uint16_t vga_char(unsigned char c, uint8_t color) {
    return ((uint16_t)color << 8) | (uint16_t)c;

}

void terminal_clear() {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            buffer[x + y*VGA_WIDTH] = vga_char(' ', cursor_color);
        }
    }
    cursor_row = 0;
    cursor_col = 0;
}

void terminal_init() {
    cursor_row = 0;
    cursor_col = 0;
    cursor_color = 0x07;
    buffer = VGA_MEMORY;
    terminal_clear();
}

static void terminal_update_cursor(void) {
    uint16_t pos = (uint16_t)(cursor_col + cursor_row*VGA_WIDTH);

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));

    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

static void terminal_scroll(void) {
    for (int y = 1; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            buffer[x + (y - 1)*VGA_WIDTH] = buffer[x + y*VGA_WIDTH];
        }
    }
    for (int x = 0; x < VGA_WIDTH; x++) {
        buffer[x + (VGA_HEIGHT - 1)*VGA_WIDTH] = vga_char(' ', cursor_color);
    }
    if (cursor_row > 0) cursor_row--;
}

void terminal_writechar(char c) {
    if (c == '\n') {
        cursor_row++;
        cursor_col = 0;
        if (cursor_row >= VGA_HEIGHT) {
            terminal_scroll();
        }
        return;
    }
    buffer[cursor_col + cursor_row*VGA_WIDTH] = vga_char((unsigned char)c, cursor_color);
    cursor_col++;
    if (cursor_col >= VGA_WIDTH) {
        cursor_row++;
    }
}

void terminal_write(const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        terminal_writechar(str[i]);
    }
    terminal_update_cursor();
}