#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>
#include <stddef.h>

#include "terminal.h"
#include "idt.h"
#include "ports.h"
#include "calc.h"
#include "memory.h"
#include "string.h"

void shell_init(void);
void shell_handle_char(char c);

void shell_history_prev(void);
void shell_history_next(void);
void shell_history_print(void);

#endif