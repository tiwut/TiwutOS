# TiwutOS v3.1 - Standalone Networking Stack

TiwutOS v3.1 features a completely native, standalone TCP/IP networking stack. This removes the need for any external proxy tools and allows the OS to communicate directly with the real internet from within the kernel.

## 1. Architectural Shift
In previous versions, TiwutOS relied on a Python-based proxy on the host machine to handle SSL/TLS and high-level protocols.
- **V3.0 and older:** Guest OS -> RTL8139 -> Host Proxy -> Real Internet.
- **V3.1 (Current):** Guest OS -> RTL8139 -> Real Internet.

## 2. Supported Protocols
The native stack currently implements:
- **Ethernet (IEEE 802.3):** Raw frame handling and MAC addressing.
- **ARP (Address Resolution Protocol):** IP-to-MAC translation for local network discovery.
- **IPv4:** Standard IP header processing, fragmentation support, and checksum verification.
- **ICMP:** Native `Ping` capabilities.
- **UDP:** Connectionless transport used for high-speed data streams and DNS.
- **TCP (Transmission Control Protocol):** 
  - Full 3-way handshake (SYN, SYN-ACK, ACK).
  - Sequence and Acknowledgement number tracking.
  - Window scaling and flow control.
  - Persistent connections for HTTP.

## 3. Real Internet Interaction
The native **Web Browser** and **Terminal** can now interact with real-world IP addresses.
- **Direct HTTP:** The browser sends raw HTTP/1.1 GET requests directly to servers (e.g., Google at `142.250.190.46`).
- **Response Parsing:** The kernel parses the incoming TCP streams to extract HTML content and display it in the GUI.

## 4. Hardware Interaction (RTL8139)
The `drivers/rtl8139.cpp` driver handles the physical layer.
- **DMA Ring Buffer:** Uses a circular buffer in memory for receiving packets without CPU intervention.
- **Transmit Status Descriptors:** Manages four independent transmit slots for parallelizing outgoing traffic.
- **Poll-based Receive:** The kernel polls the RTL8139 interrupt status bit in the main OS loop to handle incoming traffic with sub-millisecond latency.

## 5. Security & Isolation
Networking is isolated from the rest of the kernel via the `SystemManager`. Applications must request `NET_ACCESS` permissions before the kernel will allow them to open sockets or send packets, preventing unauthorized data exfiltration.
