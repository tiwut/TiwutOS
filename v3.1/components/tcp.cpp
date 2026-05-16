#pragma once
#include "../api/types.h"
#include "utils.cpp"

enum TCP_State { TCP_CLOSED, TCP_SYN_SENT, TCP_ESTABLISHED, TCP_FIN_WAIT };

class TCPSocket {
public:
  uint32_t local_ip, remote_ip;
  uint16_t local_port, remote_port;
  uint32_t seq_num, ack_num;
  TCP_State state;

  uint8_t rx_buffer[8192];
  int rx_len;

  bool has_received;

  TCPSocket() : state(TCP_CLOSED), rx_len(0), has_received(false) {}

  void Connect(uint8_t ip1, uint8_t ip2, uint8_t ip3, uint8_t ip4,
               uint16_t port) {
    static uint32_t pseudo_rng = 12345;

    remote_ip = (ip1 << 24) | (ip2 << 16) | (ip3 << 8) | ip4;
    local_ip = (10 << 24) | (0 << 16) | (2 << 8) | 15;
    remote_port = port;
    local_port = 49152 + (pseudo_rng++ % 10000);

    seq_num = pseudo_rng++;
    ack_num = 0;

    state = TCP_SYN_SENT;
    rx_len = 0;
    has_received = false;

    SendTCP(true, false, false, false, nullptr, 0);
  }

  void Send(const char *data) {
    if (state != TCP_ESTABLISHED)
      return;
    SendTCP(false, true, true, false, (uint8_t *)data, Utils::strlen(data));
    seq_num += Utils::strlen(data);
  }

  void Disconnect() {
    if (state != TCP_ESTABLISHED)
      return;
    SendTCP(false, true, false, true, nullptr, 0);
    state = TCP_FIN_WAIT;
  }

  void HandlePacket(uint8_t *pkt, int len, uint32_t src_ip, uint16_t src_port) {
    if (src_ip != remote_ip || src_port != remote_port)
      return;

    uint32_t tcp_seq = (pkt[4] << 24) | (pkt[5] << 16) | (pkt[6] << 8) | pkt[7];
    uint32_t tcp_ack =
        (pkt[8] << 24) | (pkt[9] << 16) | (pkt[10] << 8) | pkt[11];
    uint8_t flags = pkt[13];

    bool is_syn = flags & 0x02;
    bool is_ack = flags & 0x10;
    bool is_fin = flags & 0x01;

    int header_len = (pkt[12] >> 4) * 4;
    int payload_len = len - header_len;

    if (state == TCP_SYN_SENT && is_syn && is_ack) {
      ack_num = tcp_seq + 1;
      seq_num = tcp_ack;
      state = TCP_ESTABLISHED;
      SendTCP(false, true, false, false, nullptr, 0);
    } else if (state == TCP_ESTABLISHED) {
      if (payload_len > 0) {
        for (int i = 0; i < payload_len && rx_len < 8192; i++) {
          rx_buffer[rx_len++] = pkt[header_len + i];
        }
        rx_buffer[rx_len] = 0;
        ack_num = tcp_seq + payload_len;
        SendTCP(false, true, false, false, nullptr, 0);
        has_received = true;
      }
      if (is_fin) {
        ack_num = tcp_seq + 1;
        SendTCP(false, true, false, true, nullptr, 0);
        state = TCP_CLOSED;
      }
    }
  }

private:
  void SendTCP(bool syn, bool ack, bool psh, bool fin, uint8_t *payload,
               int payload_len) {
    static uint8_t pkt[1500];
    Utils::fill32(pkt, 0, 1500 / 4);

    for (int i = 0; i < 6; i++)
      pkt[i] = 0xFF;
    for (int i = 0; i < 6; i++)
      pkt[i + 6] = RTL8139::mac[i];
    pkt[12] = 0x08;
    pkt[13] = 0x00;

    uint8_t *ip = pkt + 14;
    ip[0] = 0x45;
    ip[1] = 0;
    ip[4] = 0;
    ip[5] = 0;
    ip[6] = 0x40;
    ip[7] = 0;
    ip[8] = 64;
    ip[9] = 6;
    ip[12] = (local_ip >> 24);
    ip[13] = (local_ip >> 16);
    ip[14] = (local_ip >> 8);
    ip[15] = local_ip;
    ip[16] = (remote_ip >> 24);
    ip[17] = (remote_ip >> 16);
    ip[18] = (remote_ip >> 8);
    ip[19] = remote_ip;

    uint8_t *tcp = pkt + 34;
    tcp[0] = local_port >> 8;
    tcp[1] = local_port & 0xFF;
    tcp[2] = remote_port >> 8;
    tcp[3] = remote_port & 0xFF;

    tcp[4] = seq_num >> 24;
    tcp[5] = seq_num >> 16;
    tcp[6] = seq_num >> 8;
    tcp[7] = seq_num;
    tcp[8] = ack_num >> 24;
    tcp[9] = ack_num >> 16;
    tcp[10] = ack_num >> 8;
    tcp[11] = ack_num;

    tcp[12] = 0x50;
    uint8_t flags = 0;
    if (fin)
      flags |= 0x01;
    if (syn)
      flags |= 0x02;
    if (psh)
      flags |= 0x08;
    if (ack)
      flags |= 0x10;
    tcp[13] = flags;

    tcp[14] = 0x20;
    tcp[15] = 0x00;

    if (payload && payload_len > 0) {
      for (int i = 0; i < payload_len; i++)
        tcp[20 + i] = payload[i];
    }

    uint32_t pseudo_sum = 0;
    pseudo_sum += (local_ip >> 16) & 0xFFFF;
    pseudo_sum += local_ip & 0xFFFF;
    pseudo_sum += (remote_ip >> 16) & 0xFFFF;
    pseudo_sum += remote_ip & 0xFFFF;
    pseudo_sum += 0x0006;
    pseudo_sum += (20 + payload_len);

    int tcp_bytes = 20 + payload_len;
    uint16_t *p = (uint16_t *)tcp;
    while (tcp_bytes > 1) {
      pseudo_sum += *p++;
      tcp_bytes -= 2;
    }
    if (tcp_bytes > 0)
      pseudo_sum += (*(uint8_t *)p) << 8;

    while (pseudo_sum >> 16)
      pseudo_sum = (pseudo_sum & 0xFFFF) + (pseudo_sum >> 16);
    uint16_t tcp_cs = ~pseudo_sum;
    tcp[16] = tcp_cs & 0xFF;
    tcp[17] = tcp_cs >> 8;

    int total_len = 20 + 20 + payload_len;
    ip[2] = total_len >> 8;
    ip[3] = total_len & 0xFF;
    uint16_t ip_cs = RTL8139::Checksum(ip, 20);
    ip[10] = ip_cs & 0xFF;
    ip[11] = ip_cs >> 8;

    static int tx_idx = 0;
    Port::out32(RTL8139::io_base + 0x20 + (tx_idx * 4),
                (uint32_t)(uint64_t)pkt);
    Port::out32(RTL8139::io_base + 0x10 + (tx_idx * 4), 14 + total_len);
    tx_idx = (tx_idx + 1) % 4;
  }
};

TCPSocket global_tcp;

void HandleTCPPacket(uint8_t *pkt, int len, uint32_t src_ip,
                     uint16_t src_port) {
  global_tcp.HandlePacket(pkt, len, src_ip, src_port);
}

void TCPFetchURL(const char *url) {
  (void)url;
  extern char browser_content[4096];
  extern int browser_img_w;
  extern int browser_img_h;
  extern uint8_t browser_img_data[400 * 400 * 3];
  extern bool browser_is_image;
  extern bool needs_redraw;

  browser_img_w = 0;
  browser_img_h = 0;
  browser_is_image = false;

  Utils::strcpy(browser_content, "Establishing Native TCP/IP connection...\n");
  needs_redraw = true;

  uint8_t ip1 = 142, ip2 = 250, ip3 = 190, ip4 = 46;
  global_tcp.Connect(ip1, ip2, ip3, ip4, 80);

  for (int wait = 0; wait < 5000000; wait++) {
    RTL8139::PollReceive();
    if (global_tcp.state == 2)
      break;
  }

  if (global_tcp.state != 2) {
    Utils::strcpy(browser_content,
                  "Error: Native TCP Connection Failed (Timeout).\n");
    needs_redraw = true;
    return;
  }

  Utils::strcpy(browser_content,
                "TCP Connection Established! Sending HTTP GET...\n");
  needs_redraw = true;

  char req[256];
  Utils::strcpy(
      req,
      "GET / HTTP/1.1\r\nHost: www.google.com\r\nConnection: close\r\n\r\n");
  global_tcp.Send(req);

  for (int wait = 0; wait < 20000000; wait++) {
    RTL8139::PollReceive();
    if (global_tcp.has_received)
      break;
  }

  if (global_tcp.has_received) {
    global_tcp.rx_buffer[global_tcp.rx_len] = 0;
    int copy_len = global_tcp.rx_len;
    if (copy_len > 60000)
      copy_len = 60000;

    for (int i = 0; i < copy_len; i++)
      browser_content[i] = global_tcp.rx_buffer[i];
    browser_content[copy_len] = 0;

    global_tcp.Disconnect();
  } else {
    Utils::strcpy(browser_content, "Error: No HTTP data received over TCP.\n");
  }

  needs_redraw = true;
}
