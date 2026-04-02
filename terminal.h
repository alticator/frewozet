#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>
#include <stddef.h>
#include "ports.h"
#include "utils.h"

void terminal_init(void);
void terminal_clear(void);
void terminal_write(const char* str);
void terminal_writechar(char c);
void terminal_write_uint(uint32_t value);
void terminal_write_hex_digit(uint8_t v);
void terminal_write_hex8(uint8_t v);
void terminal_write_hex32(uint32_t value);
void colorshell_update_background(uint8_t color);
void colorshell_writechar(char c, const char* type);
void colorshell_write(const char* str, const char* type);
void colorshell_write_uint(uint32_t value, const char* type);
void terminal_color(uint8_t color);
void terminal_backspace(void);
void terminal_set_color_mode(int mode);
int terminal_get_color_mode(void);

#endif