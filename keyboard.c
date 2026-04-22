#include "keyboard.h"
#include "terminal.h"
#include "ports.h"
#include "shell.h"
#include "ctype.h"

#include <stdint.h>

static int shift_down = 0;
static int caps_lock_on = 0;

static const char keyboard_map[128] = {
    0,
    27,
    '1','2','3','4','5','6','7','8','9','0','-','=',
    '\b',
    '\t',
    'q','w','e','r','t','y','u','i','o','p','[',']',
    '\n',
    0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,
    '\\',
    'z','x','c','v','b','n','m',',','.','/',
    0,
    '*',
    0,
    ' ',
    0,
};

static const char keyboard_shift_map[128] = {
    0,
    27,
    '!','"','#','$','%','^','&','*','(',')','_','+',
    '\b',
    '\t',
    'Q','W','E','R','T','Y','U','I','O','P','{','}',
    '\n',
    0,
    'A','S','D','F','G','H','J','K','L',':','@','~',
    0,
    '|',
    'Z','X','C','V','B','N','M','<','>','?',
    0,
    '*',
    0,
    ' ',
    0,
};

static char keyboard_translate_scancode(uint8_t scancode) {
    char c = 0;

    if (scancode >= 128) {
        return 0;
    }

    if (shift_down) {
        c = keyboard_shift_map[scancode];
    } else {
        c = keyboard_map[scancode];
    }

    if (c == 0) return 0;

    if (isalpha(c)) {
        if (shift_down ^ caps_lock_on) {
            c = toupper(c);
        } else {
            c = tolower(c);
        }
    }

    return c;
}

void keyboard_handle(void) {
    uint8_t scancode = inb(0x60);
    
    if (scancode == 0x48) { // Up Arrow
        shell_history_prev();
        return;
    } else if (scancode == 0x50) { // Down Arrow
        shell_history_next();
        return;
    } else if (scancode == 0x4B) { // Left Arrow
        shell_move_left();
        return;
    } else if (scancode == 0x4D) { // Right Arrow
        shell_move_right();
        return;
    } else if (scancode == 0x47) { // Home Key
        shell_move_home();
        return;
    } else if (scancode == 0x4F) { // End Key
        shell_move_end();
        return;
    } else if (scancode == 0x53) { // Delete Key
        shell_delete_forward();
        return;
    } else if (scancode == 0x2A || scancode == 0x36) { // left or right shift keydown
        shift_down = 1;
        return;
    } else if (scancode == 0xAA || scancode == 0xB6) { // left or right shift keyup
        shift_down = 0;
        return;
    } else if (scancode == 0x3A) { // caps lock keydown
        caps_lock_on = !caps_lock_on;
        return;
    }

    if (scancode & 0x80) { // keyup events
        return;
    }

    char c = keyboard_translate_scancode(scancode);
    if (c) {
        shell_handle_char(c);
    } else {
        terminal_write("< [?] @ 0x");
        terminal_write_hex8(scancode);
        terminal_write(" >");
    }
}