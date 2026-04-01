#include "shell.h"

#define SHELL_BUFFER_SIZE 128

static char shell_buffer[SHELL_BUFFER_SIZE];
static size_t shell_buffer_length = 0;

static int strings_equal(const char* a, const char* b) {
    size_t i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return 0;
        }
        i++;
    }
    return a[i] == '\0' && b[i] == '\0';
}

static void shell_print_prompt(void) {
    terminal_write("Frewozet >>> ");
}

static void shell_execute_command(const char* command) {
    if (strings_equal(command, "help")) {
        terminal_write("Available commands:\n");
        terminal_write("  help - Show this help message\n");
        terminal_write("  clear - Clear the terminal\n");
        terminal_write("  ticks - Show the number of timer ticks since boot\n");
    } else if (strings_equal(command, "ticks")) {
        uint32_t ticks = get_timer_ticks();
        char buffer[32];
        int length = 0;
        if (ticks == 0) {
            buffer[length++] = '0';
        } else {
            uint32_t temp = ticks;
            while (temp > 0) {
                buffer[length++] = '0' + (temp % 10);
                temp /= 10;
            }
            for (int i = 0; i < length / 2; i++) {
                char tmp = buffer[i];
                buffer[i] = buffer[length - 1 - i];
                buffer[length - 1 - i] = tmp;
            }
        }
        buffer[length] = '\0';
        terminal_write("Timer ticks since boot: ");
        terminal_write(buffer);
        terminal_write("\n");
    } else if (strings_equal(command, "clear")) {
        terminal_clear();
    } else if (strings_equal(command, "")) {
        // Do nothing for empty command
    } else {
        terminal_write("Unknown command: ");
        terminal_write(command);
        terminal_write("\n");
    }
}

void shell_init(void) {
    shell_buffer_length = 0;
    shell_buffer[0] = '\0';
    terminal_write("\nWelcome to the Frewozet Shell!\nType 'help' for a list of commands.\n\n");
    shell_print_prompt();
}

void shell_handle_char(char c) {
    if (c == '\n') {
        terminal_write("\n");
        shell_buffer[shell_buffer_length] = '\0';
        shell_execute_command(shell_buffer);
        shell_buffer_length = 0;
        shell_buffer[0] = '\0';
        shell_print_prompt();
    } else if (c == '\b') {
        if (shell_buffer_length > 0) {
            shell_buffer_length--;
            terminal_backspace();
            shell_buffer[shell_buffer_length] = '\0';
        }
    } else if (shell_buffer_length < SHELL_BUFFER_SIZE - 1) {
        shell_buffer[shell_buffer_length++] = c;
        shell_buffer[shell_buffer_length] = '\0';
        terminal_writechar(c);
    } else {
        terminal_write("\nFREWOZET SHELL ERROR: Command buffer overflow.\n");
        shell_buffer_length = 0;
        shell_buffer[0] = '\0';
        shell_print_prompt();
    }
}
