# TiwutOS v3.1 - Filesystem & Storage

TiwutOS features a dual-layered filesystem architecture: a high-speed **Virtual RAM Disk** for system operations and a **Native Ext4 Driver** for physical disk access.

## 1. Virtual RAM Disk (VFS)
The VFS is the primary filesystem used by the OS for session-specific data and fast-access system files.
- **Location:** Resides entirely in system RAM.
- **Performance:** Near-instantaneous read/write speeds.
- **Isolation:** Protected memory regions ensure that system folders like `/OS` and `/App` are invisible to unauthorized processes.

## 2. Native Ext4 Driver
The flagship storage feature of v3.1 is the native Ext4 implementation.
- **Driver:** `drivers/ext4.cpp`
- **Interface:** Communicates with the `AHCI` driver via physical LBA (Logical Block Addressing).
- **Functionality:**
  - **Superblock Parsing:** Validates the filesystem integrity by checking the `0xEF53` magic number and reading metadata (block size, inode counts, etc.).
  - **Block Group Traversal:** Locates data blocks and inode tables across the physical disk.
  - **Mounting:** Can "Mount" a physical partition, making its content accessible to the OS.

## 3. Storage Hierarchy
1. **Application Layer:** Calls `fs_open`, `fs_read`, etc., via `TiwutAPI`.
2. **VFS Layer:** Determines if the request is for the RAM disk or a mounted physical volume.
3. **Driver Layer:** 
   - For RAM disk: Direct memory copy.
   - For Physical disk: Calls the **Ext4 Driver**.
4. **Hardware Layer:** The **Ext4 Driver** calls the **AHCI Driver** to perform DMA transfers from the SATA disk.

## 4. Disk Utility
The **Disk Utility** app (`APP_DISKUTIL`) provides a graphical interface for these operations:
- **Device Discovery:** Identifies AHCI-connected SATA drives.
- **Mounting:** Allows the user to manually trigger an Ext4 mount operation.
- **Statistics:** Displays real-time disk health and volume metrics (Total Blocks, Free Space, Feature Sets).
