#include "shell.h"
#include "shell_parser.h"
#include "shell_commands.h"

#define SHELL_BUFFER_SIZE 128
#define SHELL_MAX_ARGS 16
#define SHELL_HISTORY_SIZE 16

static char shell_buffer[SHELL_BUFFER_SIZE];
static size_t shell_buffer_length = 0;
static char shell_history[SHELL_HISTORY_SIZE][SHELL_BUFFER_SIZE];
static int shell_history_count = 0;
static int shell_history_next_index = 0;
static int shell_history_view_index = -1;

static void shell_print_prompt(void) {
    colorshell_write("Frewozet >>> ", "prompt");
}

static void shell_clear_onscreen_command(void) {
    while (shell_buffer_length > 0) {
        terminal_backspace();
        shell_buffer_length--;
    }
    shell_buffer[0] = '\0';
}

static void shell_load_buffer(const char* text) {
    size_t i = 0;

    while (text[i] != '\0' && i < SHELL_BUFFER_SIZE - 1) {
        shell_buffer[i] = text[i];
        colorshell_writechar(text[i], "default");
        i++;
    }

    shell_buffer[i] = '\0';
    shell_buffer_length = i;
}

static void shell_history_add(const char* line) {
    // Don't add empty lines to history
    if (line[0] == '\0') {
        return;
    }

    // Avoid adding duplicate consecutive entries
    if (shell_history_count > 0) {
        int newest = (shell_history_next_index - 1 + SHELL_HISTORY_SIZE) % SHELL_HISTORY_SIZE;
        if (strcmp(shell_history[newest], line) == 0) {
            return;
        }
    }

    // Add the line to history
    size_t i = 0;
    while (line[i] != '\0' && i < SHELL_BUFFER_SIZE - 1) {
        shell_history[shell_history_next_index][i] = line[i];
        i++;
    }
    shell_history[shell_history_next_index][i] = '\0';

    // Update indices
    shell_history_next_index = (shell_history_next_index + 1) % SHELL_HISTORY_SIZE;

    // Update count (max is SHELL_HISTORY_SIZE)
    if (shell_history_count < SHELL_HISTORY_SIZE) {
        shell_history_count++;
    }
}

static int shell_history_physical_index(int history_offset_from_newest) {
    return (shell_history_next_index - 1 - history_offset_from_newest + SHELL_HISTORY_SIZE) % SHELL_HISTORY_SIZE;
}

void shell_history_prev(void) {
    // If history is empty, do nothing
    if (shell_history_count == 0) {
        return;
    }

    if (shell_history_view_index == -1) {
        shell_history_view_index = 0;   /* newest */
    } else if (shell_history_view_index < shell_history_count - 1) {
        shell_history_view_index++;
    } else {
        return;
    }

    shell_clear_onscreen_command();

    // Load the command from history into the buffer and display it
    int idx = shell_history_physical_index(shell_history_view_index);
    shell_load_buffer(shell_history[idx]);
}

void shell_history_next(void) {
    // If history is empty or we're not currently viewing history, do nothing
    if (shell_history_count == 0 || shell_history_view_index == -1) {
        return;
    }

    shell_clear_onscreen_command();

    if (shell_history_view_index > 0) {
        shell_history_view_index--;
        int idx = shell_history_physical_index(shell_history_view_index);
        shell_load_buffer(shell_history[idx]);
    } else {
        shell_history_view_index = -1;
        shell_buffer[0] = '\0';
        shell_buffer_length = 0;
    }
}

void shell_history_print() {
    // If history is empty, print a message, should be impossible as history command itself will always be shown
    if (shell_history_count == 0) {
        colorshell_write("No history.\n", "error");
        return;
    }

    // Calculate the starting index for the oldest command to display
    int start = (shell_history_next_index - shell_history_count + SHELL_HISTORY_SIZE) % SHELL_HISTORY_SIZE;

    colorshell_write("Frewozet Shell command history for 16 most recent commands:\n", "info");
    colorshell_write("Use Up/Down arrow keys to navigate through history.\n", "prompt");

    // Print the history entries with their relative indices
    for (int i = 0; i < shell_history_count; i++) {
        int idx = (start + i) % SHELL_HISTORY_SIZE;

        terminal_write_decimal(-(shell_history_count - i));
        terminal_write(": ");
        terminal_write(shell_history[idx]);
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
    if (c == '\n') { // Enter key, run command
        terminal_write("\n");
        shell_history_add(shell_buffer);
        char* argv[SHELL_MAX_ARGS];
        int argc = shell_tokenize(shell_buffer, argv, SHELL_MAX_ARGS);
        shell_history_view_index = -1;
        shell_execute_command(argc, argv);
        shell_buffer_length = 0;
        shell_buffer[0] = '\0';
        shell_print_prompt();
    } else if (c == '\b') { // Backspace
        if (shell_buffer_length > 0) {
            shell_buffer_length--;
            shell_history_view_index = -1;
            terminal_backspace();
            shell_buffer[shell_buffer_length] = '\0';
        }
    } else if (shell_buffer_length < SHELL_BUFFER_SIZE - 1) { // Regular character
        shell_buffer[shell_buffer_length++] = c;
        shell_buffer[shell_buffer_length] = '\0';
        shell_history_view_index = -1;
        colorshell_writechar(c, "default");
    } else { // Buffer overflow, reset buffer and print error
        terminal_write("\nFREWOZET SHELL ERROR: Command buffer overflow.\n");
        shell_buffer_length = 0;
        shell_buffer[0] = '\0';
        shell_history_view_index = -1;
        shell_print_prompt();
    }
}
