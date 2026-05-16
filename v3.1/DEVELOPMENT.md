# TiwutOS v3.1 - Development Guide

Welcome to the development guide for TiwutOS. Whether you are contributing to the kernel or building standalone applications, this document will help you get started.

## 1. Building the OS
To build the complete OS image:
```bash
./build.sh
```
This script performs the following:
1. Compiles the **Rust Kernel** for the `x86_64-my_os` target.
2. Compiles the **C++ Monolithic Core** using a custom toolchain.
3. Compiles the **UEFI Bootloader**.
4. Packages everything into a bootable ISO using `xorriso`.

## 2. Developing Applications (TiwutAPI)
TiwutOS provides a standard C API for application development.

### Header: `api/tiwut_api.h`
This header contains syscall wrappers for:
- **Process Management:** `syscall_exit`, `syscall_yield`, `syscall_sleep`.
- **GUI:** `window_create`, `window_set_title`, `window_destroy`.
- **Graphics:** `draw_rect`, `draw_text`, `draw_pixel`.
- **Filesystem:** `fs_open`, `fs_read`, `fs_write`, `fs_close`.
- **Networking:** `net_socket`, `net_send`, `net_recv`.

### Application Template
```cpp
#include "api/tiwut_api.h"

int main() {
    int win = window_create("My App", 100, 100, 300, 200);
    draw_text(win, 10, 10, "Hello TiwutOS!", 0x00FF00);
    
    while(1) {
        syscall_yield();
    }
    return 0;
}
```

## 3. Nexus V4 ULTRA Scripting
For rapid prototyping, you can use the **Nexus V4 ULTRA** engine.

### Directives:
- `+add [Library]`: Links a native C++ library to the script context.
- `import [Module]`: Loads a Nexus module.
- `set [Var] = [Value]`: Assigns a value to a variable.
- `[Module].[Function]([Arg])`: Calls a function (e.g., `gui.alert("Hello")`).

### Running a Script:
Scripts can be executed directly within the **Nexus Terminal** or passed to the `NexusEngine` via the API.

## 4. Kernel Debugging
- **Serial Output:** The kernel pipes debug logs to `COM1`. In QEMU, use `-serial stdio` to view these logs.
- **Panic Screens:** If the kernel encounters a critical error, a Blue Screen (or Rust Panic) will display the register state and stack trace.
