build:
	riscv64-unknown-elf-gcc  -march=rv32im   -mabi=ilp32   -nostdlib   -ffreestanding   start.S main.c rvlibs.c   -T link.ld  -o main.elf
	riscv64-unknown-elf-objcopy -O binary main.elf app.bin
	riscv64-unknown-elf-objdump -d main.elf
