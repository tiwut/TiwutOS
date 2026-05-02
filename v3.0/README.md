# TiwutOS v3.0

Version 3.0 represents the next evolutionary step for TiwutOS, transitioning into a **hybrid Rust & C++ architecture**. This massive update focuses on security, system infrastructure, and a drastically improved graphical user experience.

![TiwutOS Screenshot](screenshot.png) 

## What's New in v3.0!

### Hybrid Rust & C++ Architecture
- **Rust Kernel Core:** The core kernel and system utilities have been migrated to memory-safe Rust using an `x86-unknown-none` target setup.
- **Cross-Language FFI:** Seamless integration between the new Rust kernel and the existing C++ components via Foreign Function Interfaces (FFI). 

### Mac-Style Modern Graphical Experience
- **Auto-Hiding Smart Dock:** A brand new animated dock that dynamically hides when a window overlaps it, and smoothly rises when hovered.
- **Frosted Glass Blur:** Native real-time blur algorithms (`BlurRegion`) applied to windows, menus, and the Launchpad, giving a stunning glassmorphism effect.
- **Animated Application States:** Apps now scale and minimize dynamically. 
- **Dark Mode Support:** The entire UI (dock, windows, text) instantly respects a global Dark Mode toggle.
- **HD & WQHD Resolution Support:** Instantly change screen resolution natively using Bochs Graphics Adapter (BGA) hardware ports, featuring dynamic real-time scaling of backgrounds to seamlessly fit 1920x1080 (HD) and 2560x1440 (WQHD).

### System Management Applications
- **Settings App Redesign:** A completely overhauled, beautiful Settings app with a sidebar, categorized navigation (Display, Theme & UI, System), and functional toggle buttons.
- **File Manager App:** A fully functional GUI File Manager. Create files, create folders, open files directly into the Notes editor, delete files, and format the RAM disk directly via clickable UI actions.
- **Real Task Manager:** View running PIDs and actively kill tasks or minimize apps straight from the UI.
- **System Menu:** A top-left Apple-style frosted glass menu featuring fully functional Sleep (`hlt`), Restart (keyboard controller), and Shutdown (QEMU APM) commands.

---

## Build Instructions
To compile and run the hybrid v3.0 kernel:

```bash
./build.sh
```

*(Note: Ensure you have the `nightly` rust toolchain installed with `cargo`, along with `nasm`, `g++` (32-bit multilib), `xorriso`, `grub-pc-bin`, and `qemu-system-i386`).*
