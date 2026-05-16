# TiwutOS v3.1 - System Architecture

This document provides a deep dive into the internal architecture of TiwutOS v3.1, focusing on its transition to 64-bit and its hybrid kernel design.

## 1. Hybrid Kernel Design (Rust & C++)
TiwutOS utilizes a unique hybrid architecture where performance-critical and security-sensitive core components are written in **Rust**, while the high-level graphical subsystem and application framework are built in **C++**.

- **Rust Layer:** Handles low-level interrupt management, memory allocation (heap/stack), and core driver synchronization.
- **C++ Layer:** Manages the Window Manager, the Desktop Environment (TiwutGUI), and the application lifecycle.
- **FFI (Foreign Function Interface):** The two layers communicate via a optimized C-style FFI, allowing C++ to call Rust kernel functions and vice versa with zero-cost overhead.

## 2. 64-bit Long Mode Transition
Version 3.1 marks the complete migration from 32-bit Protected Mode to 64-bit Long Mode.

- **Address Space:** The kernel now has access to the full 64-bit virtual address space, eliminating the 4GB RAM limit.
- **Registers:** Leveraging the full width of RAX-R15 registers for increased computation speed and efficiency.
- **Paging:** Implements 4-level paging (PML4, PDPT, PD, PT) for robust memory isolation.

## 3. UEFI Boot Sequence
TiwutOS now boots primarily via UEFI (Unified Extensible Firmware Interface) instead of legacy BIOS.

1. **UEFI Firmware** initializes the hardware.
2. **uefi_boot.efi** (our custom loader) is executed.
3. **Graphics Output Protocol (GOP)** is used to initialize the high-resolution framebuffer.
4. **Memory Map** is retrieved from UEFI.
5. **ExitBootServices** is called to take full control of the CPU.
6. **Long Mode** is entered, and the TiwutOS Kernel is jumped into.

## 4. Memory Management
- **Physical Memory Manager (PMM):** Uses a bitmap-based allocator to manage physical page frames.
- **Virtual Memory Manager (VMM):** Handles recursive page table mapping and identity mapping for hardware MMIO.
- **Kernel Heap:** A slab-based allocator providing `malloc`/`free` functionality to kernel modules and drivers.

## 5. Multitasking
TiwutOS uses a cooperative multitasking model combined with hardware timer interrupts for preemptive scheduling at the process level. Each window in the GUI runs in its own logical context, managed by the `SystemManager`.
