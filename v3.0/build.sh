#!/bin/bash
set -e

cd /home/tiwut/Documents/Dev/TiwutOS/v3.0

echo "1. Compiling Rust Kernel..."
cd kernel
cargo +nightly build -Z build-std=core,compiler_builtins -Z build-std-features=compiler-builtins-mem -Z json-target-spec --target i386-my_os.json --release
cd ..

echo "2. Compiling C++ Components..."
CPPFLAGS="-m32 -ffreestanding -fno-rtti -fno-exceptions -Wall -Wextra -c"
g++ $CPPFLAGS sys/core.cpp -o core.o

echo "3. Assembling Bootloader..."
nasm -f elf32 boot/boot.asm -o boot.o

echo "4. Linking..."
ld -m elf_i386 -T boot/link.ld -o kernel.bin boot.o core.o kernel/target/i386-my_os/release/libtiwut_kernel.a

echo "5. Building ISO..."
mkdir -p isodir/boot/grub
cat > isodir/boot/grub/grub.cfg << EOF
set timeout=0
set default=0
menuentry "TiwutOS v3.0 (Rust+C++)" { multiboot /boot/kernel.bin; module /boot/bg.bmp; module /boot/logo.bmp; boot; }
EOF
cp kernel.bin isodir/boot/kernel.bin
python3 create_bg.py
grub-mkrescue -o TiwutOS-v3.iso isodir

echo "Build successful!"
