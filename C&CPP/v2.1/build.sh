#!/bin/bash
set -e

echo "1. Cleaning..."
rm -f *.o kernel.bin TiwutOS.iso

echo "2. Compiling C++ Files..."
CPPFLAGS="-m32 -ffreestanding -fno-rtti -fno-exceptions -Wall -Wextra -c"


g++ $CPPFLAGS kernel.cpp -o kernel.o

echo "3. Assembling Bootloader..."
nasm -f elf32 boot.asm -o boot.o

echo "4. Linking..."
ld -m elf_i386 -T link.ld -o kernel.bin boot.o kernel.o

echo "5. Building ISO..."
mkdir -p isodir/boot/grub
cat > isodir/boot/grub/grub.cfg << EOF
set timeout=0
set default=0
menuentry "TiwutOS C++" { multiboot /boot/kernel.bin; module /boot/bg.bmp; boot; }
EOF
cp kernel.bin isodir/boot/kernel.bin
python3 create_bg.py
grub-mkrescue -o TiwutOS.iso isodir

echo "6. Running..."
qemu-system-i386 -cdrom TiwutOS.iso -vga std -device rtl8139,netdev=n1 -netdev user,id=n1 -serial tcp:127.0.0.1:4444,server,nowait
