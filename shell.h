#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>
#include <stddef.h>

#include "terminal.h"
#include "idt.h"
#include "calc.h"

void shell_init(void);
void shell_handle_char(char c);

#endif