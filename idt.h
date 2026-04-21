#ifndef IDT_H
#define IDT_H

#include <stdint.h>

struct interrupt_frame {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t interrupt_number, error_code;
    uint32_t eip, cs, eflags;
};

void idt_init(void);
void interrupt_handler(struct interrupt_frame* frame);

#endif