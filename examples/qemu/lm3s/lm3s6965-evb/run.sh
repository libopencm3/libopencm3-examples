#!/bin/sh
qemu-system-arm -semihosting -M lm3s6965evb --kernel ./qemu-lm3s.elf -serial stdio
