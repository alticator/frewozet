#include "shell_commands.h"
#include "shell_parser.h"

#include "terminal.h"
#include "string.h"
#include "memory.h"
#include "ports.h"
#include "idt.h"
#include "shell.h"
#include "ram_mapper.h"
#include "pmm.h"

#include <stddef.h>
#include <stdint.h>

typedef void (*shell_command_func_t)(int argc, char** argv);

struct shell_command {
    const char* name;
    shell_command_func_t handler;
    const char* help;
};

static void shell_dump_bytes(const uint8_t* data, size_t length) {
    for (size_t i = 0; i < length; i++) {
        terminal_write_hex8(data[i]);
        terminal_write(" ");
    }
    terminal_write("\n");
}

static int shell_parse_uint32(const char* str, uint32_t* out) {
    uint32_t value = 0;
    int found_digit = 0;
    while (*str >= '0' && *str <= '9') {
        found_digit = 1;
        value = value * 10 + (uint32_t)(*str - '0');
        str++;
    }
    if (!found_digit) {
        return 0;
    }
    *out = value;
    return 1;
}

static void shell_print_string_args(int argc, char** argv) {
    for (int i = 0; i < argc; i++) {
        terminal_write("Arg ");
        terminal_write_decimal(i);
        terminal_write(": ");
        terminal_write(argv[i]);
        terminal_write("\n");
    }
}

static void shell_run_calc_argv(int argc, char** argv) {
    if (argc < 2) {
        colorshell_write("Usage: calc <a> <op> <b>\n", "error");
        colorshell_write("Example: calc 5 + 3\n", "prompt");
        return;
    }

    uint32_t left = 0;
    uint32_t right = 0;
    char op = 0;

    if (argc == 2) {
        const char* expr = argv[1];
        size_t i = 0;

        while (expr[i] == ' ' || expr[i] == '\t') {
            i++;
        }

        size_t start = i;
        while (expr[i] >= '0' && expr[i] <= '9') {
            i++;
        }

        if (i == start) {
            colorshell_write("FREWOZET CALC ERROR: Expected first number.\n", "error");
            return;
        }

        char left_buf[32];
        size_t left_len = i - start;
        if (left_len >= sizeof(left_buf)) {
            colorshell_write("FREWOZET CALC ERROR: Number too long. Left buffer overflow.\n", "error");
            return;
        }

        for (size_t j = 0; j < left_len; j++) {
            left_buf[j] = expr[start + j];
        }
        left_buf[left_len] = '\0';

        while (expr[i] == ' ' || expr[i] == '\t') {
            i++;
        }

        op = expr[i];
        if (op != '+' && op != '-' && op != '*' && op != '/' && op != '^') {
            colorshell_write("FREWOZET CALC ERROR: Expected operator (+, -, *, /, ^).\n", "error");
            return;
        }
        i++;

        while (expr[i] == ' ' || expr[i] == '\t') {
            i++;
        }

        const char* right_start = &expr[i];
        if (!shell_parse_uint32(left_buf, &left) || !shell_parse_uint32(right_start, &right)) {
            colorshell_write("FREWOZET CALC ERROR: Invalid number\n", "error");
            return;
        }
    } else if (argc == 4) {
        if (!shell_parse_uint32(argv[1], &left)) {
            colorshell_write("FREWOZET CALC ERROR: Invalid first number.\n", "error");
            return;
        }

        if (strlen(argv[2]) != 1) {
            colorshell_write("FREWOZET CALC ERROR: Operator must be one character.\n", "error");
            return;
        }

        op = argv[2][0];
        if (op != '+' && op != '-' && op != '*' && op != '/' && op != '^') {
            colorshell_write("FREWOZET CALC ERROR: Expected operator (+, -, *, /, ^).\n", "error");
            return;
        }

        if (!shell_parse_uint32(argv[3], &right)) {
            colorshell_write("FREWOZET CALC ERROR: Invalid second number.\n", "error");
            return;
        }
    } else {
        colorshell_write("Usage: calc <a> <op> <b>\n", "error");
        return;
    }

    terminal_write("= ");

    if (op == '+') {
        terminal_write_decimal((int)(left + right));
        terminal_write("\n");
        return;
    }

    if (op == '-') {
        if (left >= right) {
            terminal_write_decimal((int)(left - right));
        } else {
            terminal_write("-");
            terminal_write_decimal((int)(right - left));
        }
        terminal_write("\n");
        return;
    }

    if (op == '*') {
        terminal_write_decimal((int)(left * right));
        terminal_write("\n");
        return;
    }

    if (op == '/') {
        if (right == 0) {
            colorshell_write("FREWOZET CALC ERROR: Division by zero.\n", "error");
            return;
        }
        terminal_write_decimal((int)(left / right));
        terminal_write("\n");
        return;
    }

    if (op == '^') {
        uint32_t result = 1;
        for (uint32_t i = 0; i < right; i++) {
            result *= left;
        }
        terminal_write_decimal((int)result);
        terminal_write("\n");
        return;
    }
}

static void cmd_help(int argc, char** argv);
static void cmd_ticks(int argc, char** argv);
static void cmd_clear(int argc, char** argv);
static void cmd_echo(int argc, char** argv);
static void cmd_history(int argc, char** argv);
static void cmd_calc(int argc, char** argv);
static void cmd_colorshell(int argc, char** argv);
static void cmd_quit(int argc, char** argv);
static void cmd_shutdown(int argc, char** argv);
static void cmd_halt(int argc, char** argv);
static void cmd_debug(int argc, char** argv);
static void cmd_meminfo(int argc, char** argv);
static void cmd_heap(int argc, char** argv);
static void cmd_alloc(int argc, char** argv);
static void cmd_pmminfo(int argc, char** argv);
// static void cmd_allocpage(int argc, char** argv);
// static void cmd_freepage(int argc, char** argv);
static void cmd_memcpy(int argc, char** argv);
static void cmd_memset(int argc, char** argv);
static void cmd_strlen(int argc, char** argv);
static void cmd_strcmp(int argc, char** argv);
static void cmd_strncmp(int argc, char** argv);

static const struct shell_command shell_commands[] = {
    {"help",       cmd_help,       "help                          - Show help"},
    {"clear",      cmd_clear,      "clear                         - Clear the terminal"},
    {"ticks",      cmd_ticks,      "ticks                         - Show timer ticks"},
    {"echo",       cmd_echo,       "echo <message>                - Print text"},
    {"history",    cmd_history,    "history                       - Show command history"},
    {"calc",       cmd_calc,       "calc <a> <op> <b>             - Simple arithmetic"},
    {"colorshell", cmd_colorshell, "colorshell                    - Enter ColorShell mode"},
    {"quit",       cmd_quit,       "quit colorshell               - Exit ColorShell mode"},
    {"shutdown",   cmd_shutdown,   "shutdown                      - Attempt shutdown"},
    {"halt",       cmd_halt,       "halt                          - Halt the system"},
    {"debug",      cmd_debug,      "debug dividezero              - Trigger divide error"},
    {"meminfo",    cmd_meminfo,    "meminfo                       - Show E820 memory map"},
    {"heap",       cmd_heap,       "heap                          - Show heap pointers"},
    {"alloc",      cmd_alloc,      "alloc <?size>                 - Allocate memory from heap. Default 64 bytes"},
    {"pmminfo",    cmd_pmminfo,    "pmminfo                       - Show physical memory manager info"},
    //{"allocpage",  cmd_allocpage,  "allocpage                     - Allocate one 4 KiB physical page"},
    //{"freepage",   cmd_freepage,   "freepage <hexaddr>            - Free one 4 KiB physical page"},
    {"memcpy",     cmd_memcpy,     "memcpy                        - Test memcpy"},
    {"memset",     cmd_memset,     "memset                        - Test memset"},
    {"strlen",     cmd_strlen,     "strlen <string>               - String length"},
    {"strcmp",     cmd_strcmp,     "strcmp <a> <b>                - Compare strings"},
    {"strncmp",    cmd_strncmp,    "strncmp <a> <b> <n>           - Compare prefix"},
};

static const size_t shell_command_count = sizeof(shell_commands) / sizeof(shell_commands[0]);

static const struct shell_command* find_command(const char* name) {
    for (size_t i = 0; i < shell_command_count; i++) {
        if (strcmp(shell_commands[i].name, name) == 0) {
            return &shell_commands[i];
        }
    }
    return NULL;
}

static void cmd_help(int argc, char** argv) {
    (void)argc;
    (void)argv;

    colorshell_write("Frewozet Shell Help:\n", "info");
    colorshell_write("Frewozet Special Keyboard Layout:\n", "info");
    colorshell_write("HERE:UK_LAYOUT '+':'=', '*':'.', '.':',', '\"':';', '^':'`'\n", "info");
    colorshell_write("Available commands:\n", "info");

    for (size_t i = 0; i < shell_command_count; i++) {
        terminal_write("  ");
        colorshell_write(shell_commands[i].help, "command");
        terminal_write("\n");
    }

    // terminal_write("Examples:\n");
    // terminal_write("  strlen hello\n");
    // terminal_write("  strlen \"hello world\"\n");
    // terminal_write("  strcmp abc abd\n");
    // terminal_write("  strncmp abc abd 2\n");
    // terminal_write("  calc 12+34\n");
    // terminal_write("  calc 12 + 34\n");
}

static void cmd_clear(int argc, char** argv) {
    (void)argc;
    (void)argv;
    terminal_clear();
}

static void cmd_ticks(int argc, char** argv) {
    (void)argc;
    (void)argv;

    uint32_t ticks = get_timer_ticks();
    terminal_write("Timer ticks since boot: ");
    terminal_write_decimal(ticks);
    terminal_write("\n");
}

static void cmd_echo(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        terminal_write(argv[i]);
        if (i < argc - 1) {
            terminal_write(" ");
        }
    }
    terminal_write("\n");
}

static void cmd_history(int argc, char** argv) {
    (void)argc;
    (void)argv;

    shell_history_print();
}

static void cmd_calc(int argc, char** argv) {
    if (argc == 1) {
        colorshell_write("Usage: calc <a> <op> <b> \n", "error");
        return;
    }
    shell_run_calc_argv(argc, argv);
}

static void cmd_colorshell(int argc, char** argv) {
    (void)argc;
    (void)argv;
    if (terminal_get_color_mode()) {
        colorshell_write("Already in ColorShell mode. Run 'quit colorshell' to exit.\n", "info");
    } else {
        terminal_set_color_mode(1);
        colorshell_write("Welcome to Frewozet ColorShell\nRun 'quit colorshell' to exit.\n", "info");
    }
}

static void cmd_quit(int argc, char** argv) {
    (void)argc;
    (void)argv;
    if (argc >= 2 && strcmp(argv[1], "colorshell") == 0) {
        if (!terminal_get_color_mode()) {
            colorshell_write("Cannot quit ColorShell. Not in ColorShell mode.\n", "warning");
        } else {
            terminal_set_color_mode(0);
            colorshell_write("Returned to the default shell.\n", "info");
        }
    } else {
        colorshell_write("Usage: quit <arg>[colorshell]\n", "error");
    }
}

static void cmd_shutdown(int argc, char** argv) {
    (void)argc;
    (void)argv;
    colorshell_write("Shutting down...\n", "warning");
    outw(0x604, 0x2000); // QEMU shutdown command
    outw(0xB004, 0x2000); // Bochs shutdown command
    outw(0x4004, 0x3400); // VirtualBox shutdown command
    colorshell_write("FREWOZET SHELL ERROR: This system does not support shutdown.\n", "error");
    colorshell_write("The system can be halted instead with 'halt'\n", "info");
}

static void cmd_halt(int argc, char** argv) {
    (void)argc;
    (void)argv;
    colorshell_write("FREWOZET SHELL: Halt command invoked. Halting the system...\n", "warning");
    for (;;) {
        __asm__ __volatile__("cli; hlt");
    }
}

static void cmd_debug(int argc, char** argv) {
    if (argc >= 2 && strcmp(argv[1], "dividezero") == 0) {
        colorshell_write("FREWOZET SHELL: Debug command invoked. Triggering divide by zero...\n", "warning");
         __asm__ __volatile__("movl $1, %eax; movl $0, %ebx; div %ebx"); // This will cause a divide error exception
    } else {
        colorshell_write("Usage: debug <arg>[dividezero]\n", "error");
    }
}

static void cmd_meminfo(int argc, char** argv) {
    (void)argc;
    (void)argv;

    if (!ram_mapper_available()) {
        colorshell_write("E820 memory map not available.\n", "error");
        return;
    }

    const struct e820_info* info = ram_mapper_get_info();

    uint64_t usable_total = 0;
    uint64_t reserved_total = 0;
    uint64_t reclaim_total = 0;
    uint64_t nvs_total = 0;
    uint64_t bad_total = 0;
    uint64_t unknown_total = 0;

    terminal_write("E820 entries: ");
    terminal_write_decimal((int)info->count);
    terminal_write("\n");

    for (uint16_t i = 0; i < info->count; i++) {
        const struct e820_entry* e = &info->entries[i];

        terminal_write("[");
        terminal_write_decimal((int)i);
        terminal_write("] base=0x");
        terminal_write_hex64(e->base);

        terminal_write(" length=0x");
        terminal_write_hex64(e->length);

        terminal_write(" type=");
        terminal_write(ram_mapper_type_to_string(e->type));
        terminal_write(" (");
        terminal_write_decimal((int)e->type);
        terminal_write(")\n");

        switch (e->type) {
            case 1:
                usable_total += e->length;
                break;
            case 2:
                reserved_total += e->length;
                break;
            case 3:
                reclaim_total += e->length;
                break;
            case 4:
                nvs_total += e->length;
                break;
            case 5:
                bad_total += e->length;
                break;
            default:
                unknown_total += e->length;
                break;
        }
    }

    terminal_write("\nMemory totals:\n");

    terminal_write("Total RAM:    ");
    uint64_t total_ram = usable_total + reserved_total + reclaim_total + nvs_total + bad_total + unknown_total;
    terminal_write_bytesize(total_ram);
    terminal_write("\n");

    terminal_write("Usable:       ");
    terminal_write_bytesize(usable_total);
    terminal_write("\n");

    terminal_write("Reserved:     ");
    terminal_write_bytesize(reserved_total);
    terminal_write("\n");

    terminal_write("ACPI Reclaim: ");
    terminal_write_bytesize(reclaim_total);
    terminal_write("\n");

    terminal_write("ACPI NVS:     ");
    terminal_write_bytesize(nvs_total);
    terminal_write("\n");

    terminal_write("Bad:          ");
    terminal_write_bytesize(bad_total);
    terminal_write("\n");

    terminal_write("Unknown:      ");
    terminal_write_bytesize(unknown_total);
    terminal_write("\n");
}

static void cmd_heap(int argc, char** argv) {
    (void)argc;
    (void)argv;
    if (!memory_is_pmm_backend_enabled()) {
        uint32_t heap_start = memory_get_heap_start();
        uint32_t heap_current = memory_get_heap_current();
        colorshell_write("Using Backend: Bootstrap\n", "info");
        colorshell_write("Heap Start: 0x", "info");
        terminal_write_hex32(heap_start);
        colorshell_write("\nHeap Current: 0x", "info");
        terminal_write_hex32(heap_current);
        colorshell_write("\nHeap Limit: 0x", "info");
        terminal_write_hex32(memory_get_heap_limit());
        colorshell_write("\nHeap Remaining: ", "info");
        terminal_write_bytesize(memory_get_heap_remaining());
        terminal_write("\n");
        return;
    }
    uint32_t committed = memory_get_heap_committed_bytes();
    uint32_t used = memory_get_heap_used_bytes();
    uint32_t free = memory_get_heap_free_bytes();

    colorshell_write("Using backend: ", "info");
    colorshell_write("Frewozet Physical Memory Manager\n", "command");
    colorshell_write("Committed Heap Memory: ", "info");
    terminal_write_bytesize(committed);
    colorshell_write("\nUsed Heap Memory: ", "info");
    terminal_write_bytesize(used);
    colorshell_write("\nFree Heap Memory: ", "info");
    terminal_write_bytesize(free);
    terminal_write("\n");
}

static void cmd_alloc(int argc, char** argv) {
    (void)argc;
    (void)argv;
    size_t alloc_size = 64;
    if (argc >= 2) {
        uint32_t parsed_size;
        if (!shell_parse_uint32(argv[1], &parsed_size)) {
            colorshell_write("Invalid size argument. Using default 64 bytes.\n", "warning");
        } else {
            alloc_size = (size_t)parsed_size;
        }
    }
    void *ptr1 = kmalloc(alloc_size);
    if (!ptr1) {
        colorshell_write("Allocation failed\n", "error");
        return;
    }
    terminal_write("Allocated ");
    terminal_write_decimal((int)alloc_size);
    terminal_write(" bytes at 0x");
    terminal_write_hex32((uint32_t)ptr1);
    terminal_write("\n");
}

static void cmd_pmminfo(int argc, char** argv) {
    (void)argc;
    (void)argv;

    colorshell_write("Frewozet Physcal Memory Manager\n", "prompt");
    colorshell_write("Tracked Pages: ", "info");
    terminal_write_decimal((int)pmm_get_total_pages());
    colorshell_write("\nUsable Pages: ", "info");
    terminal_write_decimal((int)pmm_get_total_usable_pages());
    colorshell_write("\nUsed Pages: ", "info");
    terminal_write_decimal((int)pmm_get_used_pages());
    colorshell_write("\nFree Pages: ", "info");
    terminal_write_decimal((int)pmm_get_free_pages());
    colorshell_write("\n\nTracked address space: ", "info");
    terminal_write_bytesize(pmm_get_total_bytes());
    colorshell_write("\nUsable RAM: ", "info");
    terminal_write_bytesize(pmm_get_total_usable_bytes());
    colorshell_write("\nUsed RAM (tracked pages): ", "info");
    terminal_write_bytesize(pmm_get_total_used_bytes());
    colorshell_write("\nFree RAM (tracked pages): ", "info");
    terminal_write_bytesize(pmm_get_total_free_bytes());
    terminal_write("\n");
}

static void cmd_memcpy(int argc, char** argv) {
    (void)argc;
    (void)argv;
    colorshell_write("Allocating source and destination buffers and copying 16 bytes...\n", "info");
    uint8_t* src = kmalloc(16);
    uint8_t* dest = kmalloc(16);
    for (int i = 0; i < 16; i++) {
        src[i] = (uint8_t)i;
    }
    memcpy(dest, src, 16);
    colorshell_write("SRC: ", "output");
    shell_dump_bytes((const uint8_t*)src, 16);
    colorshell_write("DST: ", "output");
    shell_dump_bytes((const uint8_t*)dest, 16);
    terminal_write("\n");
}

static void cmd_memset(int argc, char** argv) {
    (void)argc;
    (void)argv;
    colorshell_write("Allocating buffer and setting all bytes to 0xAB...\n", "info");
    uint8_t* buffer = (uint8_t*)kmalloc(16);
    memset(buffer, 0xAB, 16);
    colorshell_write("Buffer after memset: ", "output");
    shell_dump_bytes(buffer, 16);
    terminal_write("\n");
}

static void cmd_strlen(int argc, char** argv) {
    if (argc < 2) {
        colorshell_write("Usage: strlen <string>\n", "error");
        return;
    }
    const char* s = argv[1];
    size_t len = strlen(s);
    terminal_write("Length: ");
    terminal_write_decimal((int)len);
    terminal_write("\n");
}

static void cmd_strcmp(int argc, char** argv) {
    if (argc < 3) {
        colorshell_write("Usage: strcmp <string1> <string2>\n", "error");
        return;
    }
    const char* s1 = argv[1];
    const char* s2 = argv[2];
    int result = strcmp(s1, s2);
    terminal_write("strcmp result: ");
    terminal_write_decimal(result);
    terminal_write("\n");
}

static void cmd_strncmp(int argc, char** argv) {
    if (argc < 4) {
        colorshell_write("Usage: strncmp <string1> <string2> <n>\n", "error");
        return;
    }
    const char* s1 = argv[1];
    const char* s2 = argv[2];
    uint32_t n;
    if (!shell_parse_uint32(argv[3], &n)) {
        colorshell_write("Invalid number for n\n", "error");
        return;
    }
    int result = strncmp(s1, s2, n);
    terminal_write("strncmp result: ");
    terminal_write_decimal(result);
    terminal_write("\n");
}

void shell_execute_command(int argc, char** argv) {
    if (argc == 0) {
        return;
    }
    const struct shell_command* cmd = find_command(argv[0]);
    if (cmd) {
        cmd->handler(argc, argv);
    } else {
        colorshell_write("Unknown command: ", "error");
        terminal_write(argv[0]);
        terminal_write("\n");
    }
}