OSNAME := $(shell uname -s)
QEMU = qemu-system-riscv64
ifeq ($(OSNAME), Linux)
	PRE = riscv64-linux-gnu-
# 	QEMU = /vagrant_data/riscv/riscv-qemu-4.2.0-2020.04.0-x86_64-linux-ubuntu14/bin/qemu-system-riscv64	
else
	PRE = riscv64-unknown-elf-
#	QEMU = qemu-system-riscv64
endif
GCC = $(PRE)gcc
LD = $(PRE)ld
SCRIPT = -Tkernel.ld


FLAGS+=-mcmodel=medany
# pedantic: gera o diagnóstico descrito pelo padrão C89 da linguagem C
# comentei para não ter warnings
# FLAGS += -Wall -Wextra -pedantic -O0 -g
FLAGS += -w -O0 -g
FLAGS += -march=rv64gc -mabi=lp64
FLAGS += -ffreestanding
LDFLAGS += -nostdlib 

NCPU = 4
# Dispositivos virtio 
DISK = hdd.disco
# Um id qualquer para o drive
DRIVE_ID = disco1
DRIVE = if=none,format=raw,file=$(DISK),id=$(DRIVE_ID)
HDD = virtio-blk-device,drive=$(DRIVE_ID)
RANDOM = virtio-rng-device
GPU = virtio-gpu-device
KEYBOARD = virtio-keyboard-device

RAM = 0.5G
# '\' é continuação da linha
OBJ = \
	boot.o \
	main.o \
	uart.o \
	printf.o \
	memory.o \
	string.o \
	trap_handler.o \
	trap.o \
	syscalls.o \
	plic.o \
	console.o \
	proc.o \
	programs.o \
	gpu.o \
	virtio.o \
	rng.o \
	game.o \
	keyboard.o \
	math.o \
	time.o \
	block.o \
	algorithms.o \
	shell.o \
	minix.o
#static pattern rules (http://web.mit.edu/gnu/doc/html/make_4.html#SEC37)
# $@ e $< são variáveis automáticas 	
%.o: %.c
	$(GCC) $(FLAGS) -c -o $@ $<	

%.o: %.s
	$(GCC) $(FLAGS) -c -o $@ $<	

%.o: virtio/%.c
	$(GCC) $(FLAGS) -c -o $@ $<	

%.o: game/%.c
	$(GCC) $(FLAGS) -c -o $@ $<	

%.o: libs/%.c
	$(GCC) $(FLAGS) -c -o $@ $<	

%.o: filesystems/%.c
	$(GCC) $(FLAGS) -c -o $@ $<	

kernel: $(OBJ)
	$(LD) $(LDFLAGS) $(SCRIPT) -o $@  $(OBJ)

run: kernel
	$(QEMU) -append 'console=ttyS0' -serial mon:stdio -display vnc=0.0.0.0:1 \
	-bios none \
	-cpu rv64 \
	-smp $(NCPU) \
	-machine virt -m $(RAM) \
	-drive $(DRIVE) -device $(HDD) \
	-device $(RANDOM) \
	-device $(GPU) \
	-device $(KEYBOARD) \
	-kernel kernel		
debug: kernel
	$(QEMU) -machine virt -m $(RAM) -nographic -serial mon:stdio -bios none \
	-device $(RANDOM) \
	-device $(GPU) \
	-device $(KEYBOARD) \
	-gdb tcp::1234 -S -kernel kernel
clean: 
	rm -rf kernel
	rm -rf *.o
