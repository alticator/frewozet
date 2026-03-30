#ifndef IDT_H
#define IDT_H

#include <stdint.h>
#include "terminal.h"

void idt_init(void);
void isr_handler(uint32_t interrupt);

#endif