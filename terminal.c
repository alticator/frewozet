#include "terminal.h"

#define TERMINAL_WIDTH 80
#define TERMINAL_HEIGHT 24
#define COM1 0x3F8
#define COM1_LINE_STATUS 5
#define COM1_TRANSMITTER_EMPTY 0x20

static size_t cursor_row;
static size_t cursor_col;
static uint8_t cursor_color;
static int color_mode = 0;

static void serial_writechar(char c) {
    while ((inb(COM1 + COM1_LINE_STATUS) & COM1_TRANSMITTER_EMPTY) == 0) {
    }
    outb(COM1, (uint8_t)c);
}

static void serial_write(const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        serial_writechar(str[i]);
    }
}

static void serial_set_color(uint8_t color) {
    switch (color & 0x0F) {
        case 0x0A:
            serial_write("\033[92m");
            break;
        case 0x0B:
            serial_write("\033[96m");
            break;
        case 0x0C:
            serial_write("\033[91m");
            break;
        case 0x0D:
            serial_write("\033[95m");
            break;
        case 0x0E:
            serial_write("\033[93m");
            break;
        case 0x0F:
            serial_write("\033[97m");
            break;
        default:
            serial_write("\033[37m");
            break;
    }
}

static void terminal_update_cursor(void) {
    char sequence[16];
    int length = 0;
    size_t row = cursor_row + 1;
    size_t col = cursor_col + 1;

    sequence[length++] = '\033';
    sequence[length++] = '[';
    if (row >= 10) {
        sequence[length++] = (char)('0' + row / 10);
    }
    sequence[length++] = (char)('0' + row % 10);
    sequence[length++] = ';';
    if (col >= 10) {
        sequence[length++] = (char)('0' + col / 10);
    }
    sequence[length++] = (char)('0' + col % 10);
    sequence[length++] = 'H';
    sequence[length] = '\0';
    serial_write(sequence);
}

int terminal_get_color_mode() {
    return color_mode;
}

void terminal_set_color_mode(int mode) {
    color_mode = mode;
}

void terminal_clear() {
    cursor_row = 0;
    cursor_col = 0;
    serial_write("\033[2J\033[H");
}

void terminal_init() {
    cursor_row = 0;
    cursor_col = 0;
    cursor_color = 0x07;

    /* COM1: 115200 baud, 8 data bits, no parity, one stop bit. */
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x01);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
    outb(COM1 + 1, 0x01);

    terminal_clear();
}

void terminal_color(uint8_t color) {
    cursor_color = color;
    if (color_mode) {
        serial_set_color(color);
    }
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
    serial_write("\033[1S");
    cursor_row = TERMINAL_HEIGHT - 1;
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
        serial_writechar('\r');
        serial_writechar('\n');
        cursor_row++;
        cursor_col = 0;
        if (cursor_row >= TERMINAL_HEIGHT) {
            terminal_scroll();
        }
        return;
    }
    serial_writechar(c);
    cursor_col++;
    if (cursor_col >= TERMINAL_WIDTH) {
        cursor_row++;
        cursor_col = 0;
        if (cursor_row >= TERMINAL_HEIGHT) {
            terminal_scroll();
        }
    }
}

void terminal_backspace(void) {
    if (cursor_col > 0) {
        cursor_col--;
    } else if (cursor_row > 0) {
        cursor_row--;
        cursor_col = TERMINAL_WIDTH - 1;
    }
    serial_write("\b \b");
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

void terminal_write_line(const char *str){
    terminal_write(str);
    terminal_write("\n");
}

size_t terminal_get_cursor_index(void) {
    return cursor_row * TERMINAL_WIDTH + cursor_col;
}

void terminal_set_cursor_index(size_t index) {
    if (index >= TERMINAL_WIDTH * TERMINAL_HEIGHT) {
        index = TERMINAL_WIDTH * TERMINAL_HEIGHT - 1;
    }
    
    cursor_row = index / TERMINAL_WIDTH;
    cursor_col = index % TERMINAL_WIDTH;

    terminal_update_cursor();
}