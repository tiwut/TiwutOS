# TiwutOS
A custom, bare-metal operating system built entirely from scratch in C++, Rust, and Assembly. TiwutOS started as a simple "Hello World" kernel and has rapidly evolved into a fully graphical, network-capable environment with a hybrid architecture.

![TiwutOS Screenshot](https://github.com/Nexus-Titan/TiwutOS/blob/main/v3.1/screenshot.png?raw=true)

## Versions

TiwutOS has gone through major architectural changes. Check out the readmes for the specific versions:

### [v3.1 - x64 Standalone & Native Drivers](v3.1/README.md)
The current definitive version of TiwutOS! It completes the transition to a **full 64-bit (x64) architecture** with UEFI support. This version eliminates all host-side dependencies by implementing a **native TCP/IP stack** for real-world networking and a **native Ext4 filesystem driver** for direct disk management. Features the **Nexus V4 ULTRA** modular scripting engine and a dedicated **Disk Utility** for managing physical partitions.

### [v3.0 - Hybrid Rust/C++ Architecture](v3.0/README.md)
The precursor to v3.1 that introduced the memory-safe **Rust kernel** core. Features a stunning glassmorphism UI, smart auto-hiding dock, and a fully functional RAM-based File Manager.

### [v2.1 - Native Networking (Legacy)](C&CPP/v2.1/README.md)
The first milestone implementing a raw networking stack for the RTL8139.

---

## Current Status
- **"Hello World" Bootloader** -> Successful
- **Graphical Window Interface** -> Successful
- **Custom Native Networking Stack** -> Successful
- **Hybrid Rust Kernel Integration** -> Successful
- **64-bit Native Architecture** -> Successful / Stable

## Build Requirements
- `nasm`
- `g++-multilib` (for 32-bit compilation on x64 systems)
- `xorriso` & `grub-pc-bin`
- `qemu-system-i386` (or `qemu-system-x86_64`)
- `rustup` with `nightly` toolchain (For v3.0+)
