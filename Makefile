BOOT_SRC      = boot.asm
ENTRY_SRC     = entry.asm
INTERRUPT_SRC = interrupts.asm
KERNEL_SRC    = kernel.c terminal.c idt.c
LINKER_SRC    = linker.ld

BOOT_BIN      = boot.bin
ENTRY_OBJ     = entry.o
INTERRUPT_OBJ = interrupts.o
KERNEL_OBJS   = $(KERNEL_SRC:.c=.o)
KERNEL_ELF    = kernel.elf
KERNEL_BIN    = kernel.bin
OS_IMG        = os.img

NASM        = nasm
CC          = x86_64-elf-gcc
LD          = x86_64-elf-ld
OBJCOPY     = x86_64-elf-objcopy
DD          = dd

SECTOR_SIZE        = 512
IMG_SECTORS        = 2880
KERNEL_MAX_SECTORS = 16

CFLAGS  = -m32 -march=i386 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables -fno-unwind-tables -nostdlib -O2 -Wall -Wextra
LDFLAGS = -m elf_i386 -T $(LINKER_SRC) -nostdlib

all: $(OS_IMG)

$(BOOT_BIN): $(BOOT_SRC)
	$(NASM) -f bin $(BOOT_SRC) -o $(BOOT_BIN)

$(ENTRY_OBJ): $(ENTRY_SRC)
	$(NASM) -f elf32 $(ENTRY_SRC) -o $(ENTRY_OBJ)

$(INTERRUPT_OBJ): $(INTERRUPT_SRC)
	$(NASM) -f elf32 $(INTERRUPT_SRC) -o $(INTERRUPT_OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_ELF): $(ENTRY_OBJ) $(INTERRUPT_OBJ) $(KERNEL_OBJS) $(LINKER_SRC)
	$(LD) $(LDFLAGS) -o $(KERNEL_ELF) $(ENTRY_OBJ) $(INTERRUPT_OBJ) $(KERNEL_OBJS)

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $(KERNEL_ELF) $(KERNEL_BIN)
	@size=$$(wc -c < $(KERNEL_BIN)); \
	max=$$(( $(KERNEL_MAX_SECTORS) * $(SECTOR_SIZE) )); \
	if [ $$size -gt $$max ]; then \
		echo "kernel.bin is $$size bytes, max is $$max bytes"; \
		exit 1; \
	fi

$(OS_IMG): $(BOOT_BIN) $(KERNEL_BIN)
	$(DD) if=/dev/zero of=$(OS_IMG) bs=$(SECTOR_SIZE) count=$(IMG_SECTORS)
	$(DD) if=$(BOOT_BIN) of=$(OS_IMG) conv=notrunc
	$(DD) if=$(KERNEL_BIN) of=$(OS_IMG) bs=$(SECTOR_SIZE) seek=1 conv=notrunc

run: $(OS_IMG)
	qemu-system-i386 -fda $(OS_IMG) -boot a

clean:
	rm -f $(BOOT_BIN) $(ENTRY_OBJ) $(INTERRUPT_OBJ) $(KERNEL_OBJS) $(KERNEL_ELF) $(KERNEL_BIN) $(OS_IMG)