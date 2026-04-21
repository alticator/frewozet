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
    'a','s','d','f','g','h','j','k','l','\"','\'','^',
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
    if (scancode == 0x48) { // Up
        shell_history_prev();
        return;
    } else if (scancode == 0x50) { // Down
        shell_history_next();
        return;
    } else if (scancode == 0x4B) { // Left
        shell_move_left();
        return;
    } else if (scancode == 0x4D) { // Right
        shell_move_right();
        return;
    } else if (scancode == 0x47) { // Home
        shell_move_home();
        return;
    } else if (scancode == 0x4F) { // End
        shell_move_end();
        return;
    } else if (scancode == 0x53) { // Delete
        shell_delete_forward();
        return;
    }
    if (scancode < 128) {
        char c = keyboard_map[scancode];
        if (c) {
            shell_handle_char(c);
        }
    } else {
        terminal_write("< [?] @ 0x");
        terminal_write_hex8(scancode);
        terminal_write(" >");
    }
}