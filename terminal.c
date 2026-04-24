#include "terminal.h"
#include "mmu.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((volatile uint16_t*)PHYS_TO_VIRT(0x000B8000))

static size_t cursor_row;
static size_t cursor_col;
static uint8_t cursor_color;
static volatile uint16_t* buffer;
static int color_mode = 0;

static inline uint16_t vga_char(unsigned char c, uint8_t color) {
    return ((uint16_t)color << 8) | (uint16_t)c;

}

int terminal_get_color_mode() {
    return color_mode;
}

void terminal_set_color_mode(int mode) {
    color_mode = mode;
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

void terminal_color(uint8_t color) {
    cursor_color = color;
}

void terminal_write_uint(uint32_t value) {
    char buffer[32];
    int length = 0;
    if (value == 0) {
        buffer[length++] = '0';
    } else {
        uint32_t temp = value;
        while (temp > 0) {
            buffer[length++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = 0; i < length / 2; i++) {
            char tmp = buffer[i];
            buffer[i] = buffer[length - 1 - i];
            buffer[length - 1 - i] = tmp;
        }
    }
    buffer[length] = '\0';
    terminal_write(buffer);
}

void terminal_write_hex_digit(uint8_t v) {
    if (v < 10) {
        terminal_writechar('0' + v);
    } else {
        terminal_writechar('A' + (v - 10));
    }
}

void terminal_write_hex8(uint8_t v) {
    terminal_write_hex_digit((v >> 4) & 0x0F);
    terminal_write_hex_digit(v & 0x0F);
}

void terminal_write_hex32(uint32_t value) {
    for (int shift = 28; shift >= 0; shift -= 4) {
        terminal_write_hex_digit((value >> shift) & 0x0F);
    }
}

void terminal_write_hex64(uint64_t value) {
    for (int shift = 60; shift >= 0; shift -= 4) {
        terminal_write_hex_digit((value >> shift) & 0x0F);
    }
}

void terminal_write_bytesize(uint64_t value) {
    char buffer[32];
    format_bytes(value, buffer);
    terminal_write(buffer);
}

static uint8_t get_colorshell_color(const char* type) {
    uint8_t color = 0x07; // Default gray
    if (color_mode) {
        if (strings_equal(type, "info")) {
            color = 0x0B; // Light cyan
        } else if (strings_equal(type, "warning")) {
            color = 0x0E; // Yellow
        } else if (strings_equal(type, "error")) {
            color = 0x0C; // Light red
        } else if (strings_equal(type, "success")) {
            color = 0x0A; // Light green
        } else if (strings_equal(type, "prompt")) {
            color = 0x0D; // Light green for prompts
        } else if (strings_equal(type, "command")) {
            color = 0x0F; // Bright white for commands
        } else if (strings_equal(type, "output")) {
            color = 0x0F; // Bright white for command output
        }
    }
    return color;
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
}

void colorshell_writechar(char c, const char* type) {
    uint8_t color = get_colorshell_color(type);
    uint8_t previous_color = cursor_color;
    terminal_color(color);
    terminal_writechar(c);
    terminal_color(previous_color);
}

void colorshell_write(const char* str, const char* type) {
    if (!color_mode) {
        terminal_write(str);
        return;
    }
    uint8_t color = get_colorshell_color(type);
    uint8_t previous_color = cursor_color;
    terminal_color(color);
    terminal_write(str);
    terminal_color(previous_color);
}

void colorshell_write_uint(uint32_t value, const char* type) {
    if (!color_mode) {
        terminal_write_uint(value);
        return;
    }
    uint8_t color = get_colorshell_color(type);
    uint8_t previous_color = cursor_color;
    terminal_color(color);
    terminal_write_uint(value);
    terminal_color(previous_color);
}

void terminal_write_decimal(int32_t value) {
    if (value < 0) {
        terminal_writechar('-');
        value = -value;
    }
    terminal_write_uint((uint32_t)value);
}

static void terminal_writechar_optimized(char c) {
    if (c == '\n') {
        cursor_row++;
        cursor_col = 0;
        if (cursor_row >= VGA_HEIGHT) {
            terminal_scroll();
            cursor_row = VGA_HEIGHT - 1;
        }
        return;
    }
    buffer[cursor_col + cursor_row*VGA_WIDTH] = vga_char((unsigned char)c, cursor_color);
    cursor_col++;
    if (cursor_col >= VGA_WIDTH) {
        cursor_row++;
        cursor_col = 0;
    }
}

void terminal_backspace(void) {
    if (cursor_col > 0) {
        cursor_col--;
    } else if (cursor_row > 0) {
        cursor_row--;
        cursor_col = VGA_WIDTH - 1;
    }
    buffer[cursor_col + cursor_row*VGA_WIDTH] = vga_char(' ', cursor_color);
    terminal_update_cursor();
}

void terminal_writechar(char c) {
    terminal_writechar_optimized(c);
    terminal_update_cursor();
}

void terminal_write(const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        terminal_writechar_optimized(str[i]);
    }
    terminal_update_cursor();
}

size_t terminal_get_cursor_index(void) {
    return cursor_row * VGA_WIDTH + cursor_col;
}

void terminal_set_cursor_index(size_t index) {
    if (index >= VGA_WIDTH * VGA_HEIGHT) {
        index = VGA_WIDTH * VGA_HEIGHT - 1;
    }
    
    cursor_row = index / VGA_WIDTH;
    cursor_col = index % VGA_WIDTH;

    terminal_update_cursor();
}