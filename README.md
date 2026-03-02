# RV32I emulator

This is a basic rv32i emulator. Write, read and exit syscalls have been added for interaction with the virtual machine. `EBREAK`, `ECALL`, `FENCE` do not work currently.

## Dependencies

- C standard library
- riscv elf toolchain (optional) - for running assembly code on the emulator

## Building
```
# Build the emulator
make main

# Build and try to run the emulator (unneeded)
make all
```

## Usage

Need riscv assembly code to run for now.

```
riscv32-unknown-elf-as -march=rv32i hello.s -o hello.o
riscv32-unknown-elf-ld hello.o -o hello.elf
riscv32-unknown-elf-objcopy -O binary hello.elf hello.bin
./main test.bin
```
