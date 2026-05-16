class PCI {
public:
  static uint32_t ConfigReadWord(uint8_t bus, uint8_t slot, uint8_t func,
                                 uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) |
                                  (offset & 0xFC) | ((uint32_t)0x80000000));
    Port::out32(0xCF8, address);
    return (Port::in32(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF;
  }
  static uint32_t ConfigReadDWord(uint8_t bus, uint8_t slot, uint8_t func,
                                  uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) |
                                  (offset & 0xFC) | ((uint32_t)0x80000000));
    Port::out32(0xCF8, address);
    return Port::in32(0xCFC);
  }
};

extern char browser_content[4096];
extern bool needs_redraw;
extern Window wins[7];

class RTL8139 {
public:
  static bool found;
  static uint32_t io_base;
  static uint8_t mac[6];
  static uint8_t rx_buffer[8192 + 16];
  static int rx_idx;

  static void Init() {
    found = false;
    for (int bus = 0; bus < 256; bus++) {
      for (int slot = 0; slot < 32; slot++) {
        if (PCI::ConfigReadWord(bus, slot, 0, 0) == 0x10EC &&
            PCI::ConfigReadWord(bus, slot, 0, 2) == 0x8139) {
          found = true;
          io_base = PCI::ConfigReadDWord(bus, slot, 0, 0x10) & ~3;
          Port::outb(io_base + 0x52, 0x0);
          Port::outb(io_base + 0x37, 0x10);
          int timeout = 100000;
          while ((Port::inb(io_base + 0x37) & 0x10) != 0 && timeout-- > 0)
            ;
          if (timeout <= 0) {
            found = false;
            return;
          }
          for (int i = 0; i < 6; i++)
            mac[i] = Port::inb(io_base + i);

          Port::out32(io_base + 0x30, (uint32_t)(uint64_t)rx_buffer);
          Port::out16(io_base + 0x3C, 0x0005);
          Port::out32(io_base + 0x44, 0x0000000F | (1 << 7));
          Port::outb(io_base + 0x37, 0x0C);
          return;
        }
      }
    }
  }

  static uint16_t Checksum(uint8_t *ptr, int bytes) {
    uint32_t sum = 0;
    uint16_t *p = (uint16_t *)ptr;
    while (bytes > 1) {
      sum += *p++;
      bytes -= 2;
    }
    if (bytes > 0)
      sum += *(uint8_t *)p;
    while (sum >> 16)
      sum = (sum & 0xFFFF) + (sum >> 16);
    return ~sum;
  }

  static void SendPing(uint8_t ip1, uint8_t ip2, uint8_t ip3, uint8_t ip4) {
    static uint8_t pkt[128];
    Utils::fill32(pkt, 0, 128 / 4);

    for (int i = 0; i < 6; i++)
      pkt[i] = 0xFF;
    for (int i = 0; i < 6; i++)
      pkt[i + 6] = mac[i];
    pkt[12] = 0x08;
    pkt[13] = 0x00;

    uint8_t *ip = pkt + 14;
    ip[0] = 0x45;
    ip[1] = 0;
    ip[4] = 0;
    ip[5] = 0;
    ip[6] = 0;
    ip[7] = 0;
    ip[8] = 64;
    ip[9] = 1;
    ip[12] = 10;
    ip[13] = 0;
    ip[14] = 2;
    ip[15] = 15;
    ip[16] = ip1;
    ip[17] = ip2;
    ip[18] = ip3;
    ip[19] = ip4;

    uint8_t *icmp = pkt + 34;
    icmp[0] = 8;
    icmp[1] = 0;
    icmp[4] = 0x12;
    icmp[5] = 0x34;
    icmp[6] = 0x00;
    icmp[7] = 0x01;

    int payload_len = 32;
    for (int i = 0; i < payload_len; i++)
      icmp[8 + i] = i;

    uint16_t icmp_cs = Checksum(icmp, 8 + payload_len);
    icmp[2] = icmp_cs & 0xFF;
    icmp[3] = icmp_cs >> 8;

    int total_len = 20 + 8 + payload_len;
    ip[2] = total_len >> 8;
    ip[3] = total_len & 0xFF;
    uint16_t cs = Checksum(ip, 20);
    ip[10] = cs & 0xFF;
    ip[11] = cs >> 8;

    static int tx_idx = 0;
    Port::out32(io_base + 0x20 + (tx_idx * 4), (uint32_t)(uint64_t)pkt);
    Port::out32(io_base + 0x10 + (tx_idx * 4), 14 + total_len);
    tx_idx = (tx_idx + 1) % 4;
  }

  static void FetchURL(const char *url) {
    extern void HandleTCPPacket(uint8_t *, int, uint32_t, uint16_t);
    extern void TCPFetchURL(const char *url);
    TCPFetchURL(url);
  }

  static void PollReceive() {
    uint8_t cmd = Port::inb(io_base + 0x37);
    if (cmd & 1)
      return;

    uint64_t rx_ptr = (uint64_t)rx_buffer + rx_idx;
    uint16_t pkt_len = *(uint16_t *)(rx_ptr + 2);
    uint8_t *pkt = (uint8_t *)(rx_ptr + 4);

    if (pkt[12] == 0x08 && pkt[13] == 0x00 && pkt[14 + 9] == 1) {
      uint8_t *icmp = pkt + 34;
      if (icmp[0] == 0) {
        uint8_t ip1 = pkt[26];
        uint8_t ip2 = pkt[27];
        uint8_t ip3 = pkt[28];
        uint8_t ip4 = pkt[29];
        int len = Utils::strlen(wins[0].buffer);
        if (len < 1900) {
          Utils::strcpy(wins[0].buffer + len, "\nReply from ");
          len = Utils::strlen(wins[0].buffer);
          Utils::itoa(ip1, wins[0].buffer + len);
          len = Utils::strlen(wins[0].buffer);
          wins[0].buffer[len++] = '.';
          Utils::itoa(ip2, wins[0].buffer + len);
          len = Utils::strlen(wins[0].buffer);
          wins[0].buffer[len++] = '.';
          Utils::itoa(ip3, wins[0].buffer + len);
          len = Utils::strlen(wins[0].buffer);
          wins[0].buffer[len++] = '.';
          Utils::itoa(ip4, wins[0].buffer + len);
          len = Utils::strlen(wins[0].buffer);
          wins[0].buffer[len++] = '\n';
          wins[0].buffer[len++] = '#';
          wins[0].buffer[len++] = ' ';
          wins[0].buffer[len] = 0;
          needs_redraw = true;
        }
      }
    } else if (pkt[12] == 0x08 && pkt[13] == 0x00 && pkt[14 + 9] == 6) {
      extern void HandleTCPPacket(uint8_t *pkt, int len, uint32_t src_ip,
                                  uint16_t src_port);
      uint32_t src_ip = (pkt[14 + 12] << 24) | (pkt[14 + 13] << 16) |
                        (pkt[14 + 14] << 8) | pkt[14 + 15];
      uint16_t src_port = (pkt[34] << 8) | pkt[35];
      HandleTCPPacket(pkt + 14, ((pkt[14 + 2] << 8) | pkt[14 + 3]), src_ip,
                      src_port);
    } else if (pkt[12] == 0x08 && pkt[13] == 0x00 && pkt[14 + 9] == 17) {
      uint16_t dst_port = (pkt[36] << 8) | pkt[37];
      if (dst_port == 12345) {
        extern bool browser_is_image;
        extern int browser_img_w, browser_img_h;
        extern uint8_t browser_img_data[400 * 400 * 3];
        uint8_t *payload = pkt + 42;
        int payload_len = ((pkt[38] << 8) | pkt[39]) - 8;
        uint8_t type = payload[0];

        if (type == 1) {
          browser_img_w = payload[1] | (payload[2] << 8);
          browser_img_h = payload[3] | (payload[4] << 8);
          browser_is_image = true;
        } else if (type == 2) {
          int offset = payload[1] | (payload[2] << 8) | (payload[3] << 16) |
                       (payload[4] << 24);
          for (int i = 0; i < payload_len - 5; i++) {
            if (offset + i < 400 * 400 * 3)
              browser_img_data[offset + i] = payload[5 + i];
          }
        } else if (type == 3) {
          needs_redraw = true;
        } else if (type == 4) {
          browser_is_image = false;
          for (int i = 0; i < payload_len - 1 && i < 4000; i++)
            browser_content[i] = payload[1 + i];
          browser_content[payload_len - 1] = 0;
          needs_redraw = true;
        }
      }
    }

    rx_idx = (rx_idx + pkt_len + 4 + 3) & ~3;
    if (rx_idx >= 8192)
      rx_idx -= 8192;
    int capr = rx_idx - 16;
    if (capr < 0)
      capr += 8192;
    Port::out16(io_base + 0x38, capr);
  }
};
bool RTL8139::found = false;
uint32_t RTL8139::io_base = 0;
uint8_t RTL8139::mac[6] = {0};
uint8_t RTL8139::rx_buffer[8192 + 16] = {0};
int RTL8139::rx_idx = 0;
