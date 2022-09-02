.PHONY: clean 

ASM_SRCS = 
C_SRCS   = main.c wav_player.c 
OUT      := wav_test.elf

ASMFLAGS = -Og -g
CFLAGS   = -Og -g
LDFLAGS  = -lpulse -lasound -lpthread -lm
 
#LDFLAGS  = -nostdlib

OBJS := $(ASM_SRCS:.S=.o) $(C_SRCS:.c=.o)


CC   ?= gcc -std=c99
CC_gcc   = gcc -std=c99
GDB	 = gdb

#-L.. -lbbl -lmachine  -lutil
$(OUT): $(OBJS) 
	$(CC_gcc) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $(OUT)

clean:
	rm $(OBJS) $(OUT) 

run: $(OUT) 
	$(QEMU) -machine $(MACH) -cpu $(CPU) -smp $(CPUS) -m $(MEM)  -nographic -serial mon:stdio \
	-bios none -kernel payload.elf 

run-gdb: $(OUT) 
	$(QEMU) -machine $(MACH) -cpu $(CPU) -smp $(CPUS) -m $(MEM)  -nographic -serial mon:stdio \
	-bios none -kernel payload.elf -S -s

gdb: $(OUT)
	$(GDB) -q -tui -x $(GDB_START)

gdb-mdk: $(OUT)
	$(GDB) -q -tui -x $(GDB_START_MDK)

.S.o:
	$(CC) $(ASMFLAGS) -c $< -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@
