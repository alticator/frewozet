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
        colorshell_write("  colorshell <color>[blue, default]\n    - Change color scheme. Can only be used when ColorShell is active", "info");
        colorshell_write("  quit colorshell - Exit ColorShell mode and return to default shell\n", "info");
        colorshell_write("  shutdown - Attempt to shut down the system. May not work on all hardware\n", "info");
        colorshell_write("  halt - Halt the system. Alternative to shutdown on unsupported hardware\n", "info");
        colorshell_write("  debug dividezero - For debug. Trigger a divide error. Will crash the system.\n", "info");
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
    } else if (string_starts_with(command, "colorshell")) {
        if (terminal_get_color_mode()) {
            if (strings_equal(command + 11, "blue")) {
                colorshell_update_background(0x10); // Blue background
                colorshell_write("ColorShell Blue enabled.\n", "info");
            } else if (strings_equal(command + 11, "default") || strings_equal(command + 11, "black")) {
                colorshell_update_background(0x00);
                colorshell_write("ColorShell default enabled\n", "info");
            } else {
                colorshell_write("Already in ColorShell mode. Run 'colorshell <color>[blue, default]' to change color scheme.\n", "info");
            }
        } else {
            terminal_set_color_mode(1);
            colorshell_write("Welcome to Frewozet ColorShell\nRun 'colorshell <color>[blue, default]' to change color scheme.\n", "info");
        }
        // terminal_set_color_mode(1);
        // colorshell_write("Welcome to Frewozet ColorShell\n", "info");
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
    } else if (strings_equal(command, "debug dividezero")) {
        colorshell_write("FREWOZET SHELL WARNING: This command will trigger a division by\nzero exception, which will crash the system.\n", "warning");
        __asm__ __volatile__("movl $1, %eax; movl $0, %ebx; div %ebx"); // This will cause a divide error exception
    } else if (strings_equal(command, "heap")) {
        uint32_t heap_start = memory_get_heap_start();
        uint32_t heap_current = memory_get_heap_current();
        terminal_write("Heap Start: 0x");
        terminal_write_hex32(heap_start);
        terminal_write("\nHeap Current: 0x");
        terminal_write_hex32(heap_current);
        terminal_write("\n");
    } else if (strings_equal(command, "alloc")) {
        void *ptr1 = kmalloc(64);
        terminal_write("Allocated 64 bytes at 0x");
        terminal_write_hex32((uint32_t)ptr1);
        terminal_write("\n");
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
        colorshell_writechar(c, "default");
    } else {
        terminal_write("\nFREWOZET SHELL ERROR: Command buffer overflow.\n");
        shell_buffer_length = 0;
        shell_buffer[0] = '\0';
        shell_print_prompt();
    }
}
