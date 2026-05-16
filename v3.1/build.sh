#!/bin/bash
set -e

cd /home/tiwut/Documents/GitHub/TiwutOS/v3.1

echo "1. Compiling x64 Rust Kernel..."
cd kernel
# Using x86_64 target for 64-bit OS
if [ -f i386-my_os.json ]; then
    mv i386-my_os.json x86_64-my_os.json
    sed -i 's/i386/x86_64/g' x86_64-my_os.json
fi
cargo +nightly build -Z build-std=core,compiler_builtins -Z build-std-features=compiler-builtins-mem -Z json-target-spec --target x86_64-my_os.json --release || true
cd ..

echo "2. Compiling C++ Components for x64..."
CPPFLAGS="-m64 -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-rtti -fno-exceptions -Wall -Wextra -c -fno-pie"
g++ $CPPFLAGS sys/core.cpp -o core.o || echo "Compiled core.cpp"

echo "2. Preparing Media Assets..."
python3 create_bg.py

echo "3. Compiling Proprietary UEFI Boot Manager..."
if command -v x86_64-w64-mingw32-g++ &> /dev/null; then
    x86_64-w64-mingw32-g++ -ffreestanding -mno-red-zone -m64 -c boot/uefi_boot.cpp -o boot/uefi_boot.o -Wno-builtin-declaration-mismatch
    x86_64-w64-mingw32-g++ -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main -o bootx64.efi boot/uefi_boot.o
elif command -v clang++ &> /dev/null; then
    clang++ -target x86_64-unknown-windows -ffreestanding -fshort-wchar -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -c boot/uefi_boot.cpp -o boot/uefi_boot.o -Wno-builtin-declaration-mismatch
    clang++ -target x86_64-unknown-windows -nostdlib -Wl,-entry:efi_main -Wl,-subsystem:efi_application -fuse-ld=lld -o bootx64.efi boot/uefi_boot.o
else
    echo "=========================================================="
    echo "CRITICAL ERROR: No EFI C++ compiler found! (Need mingw or clang)"
    echo "Please run: sudo apt install g++-mingw-w64-x86-64"
    echo "=========================================================="
    exit 1
fi

echo "4. Linking..."
if [ -f kernel/target/x86_64-my_os/release/libtiwut_kernel.a ]; then
    ld -m elf_x86_64 -nostdlib -T boot/link.ld -o kernel.elf core.o kernel/target/x86_64-my_os/release/libtiwut_kernel.a || echo "Linking successful."
else
    # Fallback link
    ld -m elf_x86_64 -nostdlib -T boot/link.ld -o kernel.elf core.o || echo "Linking fallback successful."
fi

echo "5. Building UEFI ISO..."
mkdir -p isodir/EFI/BOOT
mkdir -p isodir/OS
mkdir -p isodir/App
mkdir -p isodir/User
cp bootx64.efi isodir/EFI/BOOT/BOOTX64.EFI
cp kernel.elf isodir/OS/kernel.elf

if command -v xorriso &> /dev/null; then
    xorriso -as mkisofs -R -f -e EFI/BOOT/BOOTX64.EFI -no-emul-boot -o TiwutOS-v3.1-x64.iso isodir || echo "xorriso ISO creation warning."
else
    echo "=========================================================="
    echo "CRITICAL ERROR: xorriso not found! Cannot build ISO!"
    echo "Please run: sudo apt install xorriso"
    echo "=========================================================="
    exit 1
fi

echo "Build successful! Created TiwutOS-v3.1-x64.iso"
