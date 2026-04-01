#include "shell.h"

#define SHELL_BUFFER_SIZE 128

static char shell_buffer[SHELL_BUFFER_SIZE];
static size_t shell_buffer_length = 0;

static void shell_print_prompt(void) {
    colorshell_write("Frewozet >>> ", "prompt");
}

static void shell_execute_command(const char* command) {
    if (strings_equal(command, "help")) {
        colorshell_write("Frewozet Shell Help:\n", "info");
        colorshell_write("Frewozet Special Keyboard Layout:\n", "info");
        colorshell_write("  - The '+' key is located where '=' is on a standard UK QWERTY layout.\n", "info");
        colorshell_write("  - The '*' key is located where '.' is on a standard UK QWERTY layout.\n", "info");
        colorshell_write("  - The '.' key is located where ',' is on a standard UK QWERTY layout.\n", "info");
        colorshell_write("Available commands:\n", "info");
        colorshell_write("  help - Show this help message\n", "info");
        colorshell_write("  clear - Clear the terminal\n", "info");
        colorshell_write("  ticks - Show the number of timer ticks since boot\n", "info");
        colorshell_write("  echo <message> - Print the message to the terminal\n", "info");
        colorshell_write("  calc <expression> - Evaluate a simple arithmetic expression.\n", "info");
        colorshell_write("  colorshell - Enter Frewozet ColorShell mode with colored output\n", "info");
        colorshell_write("  quit colorshell - Exit ColorShell mode and return to default shell\n", "info");
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
    } else if (string_starts_with(command, "echo")) {
        terminal_write(command + 5); // Skip "echo "
        terminal_write("\n");
    } else if (string_starts_with(command, "calc")) {
        run_calc(command + 5); // Skip "calc "
    } else if (strings_equal(command, "colorshell")) {
        terminal_set_color_mode(1);
        colorshell_write("Welcome to Frewozet ColorShell\n", "info");
    } else if (strings_equal(command, "quit colorshell")) {
        terminal_set_color_mode(0);
        terminal_color(0x07);
        terminal_write("Returned to default shell.\n");
    } else if (strings_equal(command, "shutdown")) {
        colorshell_write("Shutting down...\n", "warning");
        outw(0x604, 0x2000); // QEMU shutdown command
        outw(0xB004, 0x2000); // Bochs shutdown command
        outw(0x4004, 0x3400); // VirtualBox shutdown command
        colorshell_write("FREWOZET SHELL ERROR: This system does not support shutdown.\n", "error");
        colorshell_write("The system can be halted instead with 'halt'\n", "info");
    } else if (strings_equal(command, "halt")) {
        colorshell_write("Halting the system...\n", "warning");
        for (;;) {
            __asm__ __volatile__("cli; hlt");
        }
    }else {
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
