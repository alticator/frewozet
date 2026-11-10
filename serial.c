#include "serial.h"
#include "ports.h"
#include "shell.h"

#define COM1 0x3F8
#define COM1_LINE_STATUS 5
#define COM1_DATA_READY 0x01

static int previous_character_was_cr;

void serial_handle(void) {
    while (inb(COM1 + COM1_LINE_STATUS) & COM1_DATA_READY) {
        char c = (char)inb(COM1);

        if (c == '\r') {
            shell_handle_char('\n');
            previous_character_was_cr = 1;
        } else if (c == '\n') {
            if (!previous_character_was_cr) {
                shell_handle_char('\n');
            }
            previous_character_was_cr = 0;
        } else {
            previous_character_was_cr = 0;
            shell_handle_char(c == 0x7F ? '\b' : c);
        }
    }
}
