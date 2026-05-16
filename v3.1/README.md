# TiwutOS v3.1 - The x64 Native Era

Version 3.1 is the definitive standalone release of TiwutOS. It finalizes the transition to a **pure 64-bit (x64) architecture** with UEFI support, removes all proxy dependencies for networking, and implements native hardware drivers for storage and file management.

![TiwutOS v3.1 Screenshot](screenshot.png)

## What's New in v3.1!

### 🚀 64-bit Native Core & UEFI Support
- **Full x64 Migration:** The system now runs natively in 64-bit Long Mode, unlocking massive memory addressability and modern instruction sets.
- **UEFI Boot Manager:** A custom-built UEFI bootloader ensures compatibility with modern hardware and seamless high-resolution graphics initialization via GOP.

### 🌐 Native TCP/IP Networking (No Proxy)
- **Standalone Stack:** A custom, bare-metal TCP/IP stack built into the kernel. No more Python proxies required.
- **Direct HTTP Requests:** The web browser now performs native 3-way TCP handshakes and sends direct HTTP GET requests to real-world servers (e.g., google.com).
- **RTL8139 Driver Integration:** High-speed raw packet handling directly over the RTL8139 hardware interface.

### 🗄️ Native Ext4 Filesystem Support
- **Direct Disk Mounting:** A native Ext4 filesystem driver capable of parsing Superblocks and Inode tables directly from AHCI SATA storage.
- **Disk Utility App:** A brand-new GUI application for managing physical disks. View partition health, mount Ext4 volumes, and inspect filesystem metadata in real-time.
- **AHCI DMA Storage:** High-performance storage access utilizing advanced DMA for disk I/O.

### 🧠 Nexus V4 ULTRA Scripting
- **Modular Language Engine:** The Nexus engine now supports multi-line script execution and native module importing.
- **Native Directives:** Support for `+add` and `import` syntax for loading system-level libraries and cross-language modules.

### 🎨 Refined Graphical Experience
- **Optimized UI Engine:** Improved performance for glassmorphism effects and window animations.
- **Expanded App Suite:** Native File Manager, Task Manager, Settings, Browser, and the new Disk Utility.

---

## Build & Run Instructions

To compile and run TiwutOS v3.1:

1. **Install Dependencies:**
   ```bash
   sudo apt install nasm g++-multilib xorriso grub-pc-bin qemu-system-x86_64
   ```

2. **Run the Build Script:**
   ```bash
   ./build.sh
   ```

3. **Launch in QEMU:**
   ```bash
   ./run.sh
   ```

*(Note: The build system automatically handles the UEFI image creation and Rust kernel compilation for the x86-64 target).*
