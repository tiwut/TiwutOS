#!/bin/bash
# TiwutOS V3.1 - QEMU Run Script

echo "Starting TiwutOS V3.1 in QEMU..."

# QEMU's direct -kernel flag doesn't support 64-bit ELF natively.
# Instead, we boot our native UEFI bootloader directly from the 'isodir' folder using OVMF!
qemu-system-x86_64 \
    -bios /usr/share/ovmf/OVMF.fd \
    -drive file=fat:rw:isodir,format=raw \
    -m 256M \
    -vga std \
    -serial stdio \
    -netdev user,id=u1 -device rtl8139,netdev=u1

# If you specifically want to test the UEFI ISO Boot instead of the direct kernel:
# You will need OVMF installed (e.g. sudo apt install ovmf)
# qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd -cdrom TiwutOS-v3.1-x64.iso -m 256M -vga std -serial stdio -netdev user,id=u1 -device rtl8139,netdev=u1
