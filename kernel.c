#include "kernel.h"
#include "terminal.h"
#include "idt.h"
#include "pic.h"
#include "shell.h"
#include "memory.h"
#include "pmm.h"
#include "timer.h"
#include "paging.h"
#include "startup_error.h"
#include "gdt.h"

void kmain(void) {
    terminal_init();

    terminal_write("Alticator Frewozet\n    Development Preview\n\n");

    terminal_write("Installing kernel GDT....");
    gdt_init();
    terminal_write("  OK\n");

    terminal_write("Starting error handling system....");
    idt_init();
    terminal_write("  OK\n");

    uint32_t esp;
    __asm__ __volatile__("mov %%esp, %0" : "=r"(esp));

    terminal_write("ESP: 0x");
    terminal_write_hex32(esp);
    terminal_write("\n");

    terminal_write("Removing paging identity map");
    paging_remove_identity_map();
    terminal_write("  OK\n");

    terminal_write("Initializing paging subsystem");
    paging_init();
    terminal_write("  OK\n");

    terminal_write("Starting PIC remapping....");
    pic_remap(32, 40);
    terminal_write("  OK\n");

    terminal_write("Initializing memory....");
    memory_init();
    terminal_write("  OK\n");

    terminal_write("Initializing physical memory management....");
    pmm_init();
    terminal_write("  OK\n");

    terminal_write("Enabling PMM heap backend....");
    memory_enable_pmm_backend();
    terminal_write("  OK\n");

    terminal_write("Initializing timer at 100 Hz....");
    timer_init(100);
    terminal_write("  OK\n");
    
    terminal_write("Enabling interrupts....");
    __asm__ __volatile__("sti");
    terminal_write("  OK\n\n");

    char* startupErrors = getStartupErrors();
    if (startupErrors) {
        terminal_write("\nFREWOZET SYSTEM ERROR: The following errors occured during startup:\n");
        terminal_write(startupErrors);
        terminal_write("\n\nWARNING: Some features may not work properly due to the above errors.\n");
    }

    terminal_write("Frewozet Kernel is ready. Starting Frewozet Shell....\n\n");
    shell_init();

    while (1) {
        __asm__ __volatile__("hlt");
    }
}