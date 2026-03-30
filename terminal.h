#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>
#include <stddef.h>
#include "ports.h"

void terminal_init(void);
void terminal_clear(void);
void terminal_write(const char* str);
void terminal_writechar(char c);
void terminal_color(uint8_t color);

#endif