# TiwutOS v3.1 - Hardware Drivers

TiwutOS v3.1 features a suite of native hardware drivers developed from the ground up to ensure high performance and standalone operation.

## 1. Storage Drivers
### AHCI (Advanced Host Controller Interface)
- **File:** `drivers/ahci.cpp`
- **Capabilities:** SATA III (6Gbps) support, DMA-based data transfers, and NCQ (Native Command Queuing) preparation.
- **Role:** Primary driver for physical Hard Disks and SSDs. Used by the Ext4 driver to read/write filesystem blocks.

### NVMe (Non-Volatile Memory Express)
- **File:** `drivers/nvme.cpp`
- **Capabilities:** PCIe-based SSD support using the NVMe 1.4+ protocol.
- **Role:** High-speed storage for modern NVMe drives, bypassing the SATA bottleneck.

## 2. Networking Drivers
### RTL8139
- **File:** `drivers/rtl8139.cpp`
- **Capabilities:** 10/100 Mbps Ethernet.
- **Role:** The primary NIC driver. It handles raw packet transmission and reception, feeding data directly into the native TCP/IP stack.

## 3. Audio & Multimedia
### Intel HD Audio
- **File:** `drivers/hdaudio.cpp`
- **Capabilities:** Supports high-definition audio streams (up to 192kHz/32-bit).
- **Role:** Handles system sounds and audio playback for media applications.

## 4. Connectivity & I/O
### XHCI (USB 3.0)
- **File:** `drivers/xhci.cpp`
- **Capabilities:** Support for USB 3.0 SuperSpeed devices.
- **Role:** Future-proofing for USB keyboards, mice, and external storage.

### PCI Express (PCIe)
- **File:** `drivers/pci.cpp`
- **Capabilities:** MMIO-based device discovery (ECAM).
- **Role:** The backbone of hardware discovery. It scans the PCIe bus to identify and initialize all other drivers.

## 5. Graphics
### UEFI GOP (Graphics Output Protocol)
- **File:** `boot/uefi_boot.cpp`
- **Capabilities:** Linear Framebuffer (LFB) support.
- **Role:** Provides the high-resolution canvas for the TiwutGUI. Supports standard RGB formats across various resolutions.
