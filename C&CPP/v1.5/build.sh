#!/bin/bash
set -e

echo "1. Cleaning..."
rm -f *.o kernel.bin TiwutOS.iso

echo "2. Compiling C++ Files..."
CPPFLAGS="-m32 -ffreestanding -fno-rtti -fno-exceptions -Wall -Wextra -c"

# We only compile kernel.cpp as it now contains all necessary logic (Utils, Screen, Keyboard)
# This avoids ODR violations with old screen.cpp/keyboard.cpp
g++ $CPPFLAGS kernel.cpp -o kernel.o

echo "3. Assembling Bootloader..."
nasm -f elf32 boot.asm -o boot.o

echo "4. Linking..."
# Only link boot.o and kernel.o
ld -m elf_i386 -T link.ld -o kernel.bin boot.o kernel.o

echo "5. Building ISO..."
mkdir -p isodir/boot/grub
cat > isodir/boot/grub/grub.cfg << EOF
set timeout=0
set default=0
menuentry "TiwutOS C++" { multiboot /boot/kernel.bin; boot; }
EOF
cp kernel.bin isodir/boot/kernel.bin
grub-mkrescue -o TiwutOS.iso isodir

echo "6. Running..."
qemu-system-i386 -cdrom TiwutOS.iso
