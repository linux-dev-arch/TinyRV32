build:
	riscv64-unknown-elf-gcc  -march=rv32im   -mabi=ilp32  -nostdlib  -O2 -ffreestanding  -fno-builtin  start.S main.c rvlibs.c   -T link.ld  -o main.elf
	riscv64-unknown-elf-objcopy -O binary main.elf app.bin
decompile:
	riscv64-unknown-elf-objdump -d main.elf
