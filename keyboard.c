#include "keyboard.h"
#include "terminal.h"
#include "ports.h"
#include "shell.h"

#include <stdint.h>

static const char keyboard_map[128] = {
    0,
    27,
    '1','2','3','4','5','6','7','8','9','0','-','+',
    '\b',
    '\t',
    'q','w','e','r','t','y','u','i','o','p','[',']',
    '\n',
    0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,
    '\\',
    'z','x','c','v','b','n','m','.','*','/',
    0,
    '*',
    0,
    ' ',
    0,
};

void keyboard_handle(void) {
    uint8_t scancode = inb(0x60);
    if (scancode & 0x80) {
        return;
    }
    if (scancode < 128) {
        char c = keyboard_map[scancode];
        if (c) {
            shell_handle_char(c);
        }
    } else {
        terminal_write("< [?] @ 0x");
        write_hex8(scancode);
        terminal_write(" >");
    }
}