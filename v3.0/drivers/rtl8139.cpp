class PCI {
public:
    static uint32_t ConfigReadWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
        uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
        Port::out32(0xCF8, address);
        return (Port::in32(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF;
    }
    static uint32_t ConfigReadDWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
        uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
        Port::out32(0xCF8, address);
        return Port::in32(0xCFC);
    }
};

extern char browser_content[4096];
extern bool needs_redraw;
enum AppType { APP_TERM, APP_EDITOR, APP_SETTINGS, APP_BROWSER, APP_TASKMGR, APP_FILEMGR };
struct Window { bool active; bool minimized; int x, y, w, h; char title[32]; AppType type; char buffer[2048]; int cursor; float anim; };
extern Window wins[6];

class RTL8139 {
public:
    static bool found;
    static uint32_t io_base;
    static uint8_t mac[6];
    static uint8_t rx_buffer[8192 + 16];
    static int rx_idx;

    static void Init() {
        found = false;
        for(int bus = 0; bus < 256; bus++) {
            for(int slot = 0; slot < 32; slot++) {
                if(PCI::ConfigReadWord(bus, slot, 0, 0) == 0x10EC && PCI::ConfigReadWord(bus, slot, 0, 2) == 0x8139) {
                    found = true;
                    io_base = PCI::ConfigReadDWord(bus, slot, 0, 0x10) & ~3;
                    Port::outb(io_base + 0x52, 0x0);
                    Port::outb(io_base + 0x37, 0x10);
                    while((Port::inb(io_base + 0x37) & 0x10) != 0);
                    for(int i=0; i<6; i++) mac[i] = Port::inb(io_base + i);
                    
                    Port::out32(io_base + 0x30, (uint32_t)rx_buffer);
                    Port::out16(io_base + 0x3C, 0x0005);
                    Port::out32(io_base + 0x44, 0x0000000F | (1 << 7));
                    Port::outb(io_base + 0x37, 0x0C);
                    return;
                }
            }
        }
    }
    
    static uint16_t Checksum(uint8_t* ptr, int bytes) {
        uint32_t sum = 0; uint16_t* p = (uint16_t*)ptr;
        while (bytes > 1) { sum += *p++; bytes -= 2; }
        if (bytes > 0) sum += *(uint8_t*)p;
        while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
        return ~sum;
    }
    
    static void SendPing(uint8_t ip1, uint8_t ip2, uint8_t ip3, uint8_t ip4) {
        static uint8_t pkt[128];
        Utils::fill32(pkt, 0, 128/4);
        
        for(int i=0;i<6;i++) pkt[i] = 0xFF;
        for(int i=0;i<6;i++) pkt[i+6] = mac[i];
        pkt[12] = 0x08; pkt[13] = 0x00;
        
        uint8_t* ip = pkt + 14;
        ip[0] = 0x45; ip[1] = 0; ip[4]=0; ip[5]=0; ip[6]=0; ip[7]=0;
        ip[8] = 64; ip[9] = 1; 
        ip[12]=10; ip[13]=0; ip[14]=2; ip[15]=15;
        ip[16]=ip1; ip[17]=ip2; ip[18]=ip3; ip[19]=ip4;
        
        uint8_t* icmp = pkt + 34;
        icmp[0] = 8; 
        icmp[1] = 0; 
        icmp[4] = 0x12; icmp[5] = 0x34; 
        icmp[6] = 0x00; icmp[7] = 0x01; 
        
        int payload_len = 32;
        for(int i=0; i<payload_len; i++) icmp[8+i] = i;
        
        uint16_t icmp_cs = Checksum(icmp, 8 + payload_len);
        icmp[2] = icmp_cs & 0xFF; icmp[3] = icmp_cs >> 8;
        
        int total_len = 20 + 8 + payload_len;
        ip[2] = total_len >> 8; ip[3] = total_len & 0xFF;
        uint16_t cs = Checksum(ip, 20);
        ip[10] = cs & 0xFF; ip[11] = cs >> 8;
        
        static int tx_idx = 0;
        Port::out32(io_base + 0x20 + (tx_idx * 4), (uint32_t)pkt);
        Port::out32(io_base + 0x10 + (tx_idx * 4), 14 + total_len);
        tx_idx = (tx_idx + 1) % 4;
    }

    static void SendDNS(const char* domain) {
        static uint8_t pkt[128];
        Utils::fill32(pkt, 0, 128/4);
        
        for(int i=0;i<6;i++) pkt[i] = 0xFF;
        for(int i=0;i<6;i++) pkt[i+6] = mac[i];
        pkt[12] = 0x08; pkt[13] = 0x00;
        
        uint8_t* ip = pkt + 14;
        ip[0] = 0x45; ip[1] = 0; ip[4]=0; ip[5]=0; ip[6]=0; ip[7]=0;
        ip[8] = 64; ip[9] = 17;
        ip[12]=10; ip[13]=0; ip[14]=2; ip[15]=15;
        ip[16]=10; ip[17]=0; ip[18]=2; ip[19]=3; 
        
        uint8_t* udp = pkt + 34;
        udp[0]=0x30; udp[1]=0x39; 
        udp[2]=0x00; udp[3]=0x35; 
        udp[6]=0; udp[7]=0;
        
        uint8_t* dns = pkt + 42;
        dns[0]=0x12; dns[1]=0x34; dns[2]=0x01; dns[3]=0x00; dns[4]=0; dns[5]=1;
        dns[6]=0; dns[7]=0; dns[8]=0; dns[9]=0; dns[10]=0; dns[11]=0;
        
        int ptr = 12;
        const char* d = domain; int len_pos = ptr++; int l = 0;
        while(*d) {
            if(*d == '.') { dns[len_pos] = l; len_pos = ptr++; l = 0; }
            else { dns[ptr++] = *d; l++; }
            d++;
        }
        dns[len_pos] = l; dns[ptr++] = 0;
        dns[ptr++] = 0; dns[ptr++] = 1; dns[ptr++] = 0; dns[ptr++] = 1;
        
        int total_len = 34 + ptr;
        ip[2] = total_len >> 8; ip[3] = total_len & 0xFF;
        uint16_t cs = Checksum(ip, 20);
        ip[10] = cs & 0xFF; ip[11] = cs >> 8;
        int udp_len = 8 + ptr;
        udp[4] = udp_len >> 8; udp[5] = udp_len & 0xFF;
        
        static int tx_idx = 0;
        Port::out32(io_base + 0x20 + (tx_idx * 4), (uint32_t)pkt);
        Port::out32(io_base + 0x10 + (tx_idx * 4), 14 + total_len);
        tx_idx = (tx_idx + 1) % 4;
    }
    
    static void PollReceive() {
        uint8_t cmd = Port::inb(io_base + 0x37);
        if(cmd & 1) return;
        
        uint32_t rx_ptr = (uint32_t)rx_buffer + rx_idx;
        uint16_t pkt_len = *(uint16_t*)(rx_ptr + 2);
        uint8_t* pkt = (uint8_t*)(rx_ptr + 4);
        
        if(pkt[12] == 0x08 && pkt[13] == 0x00 && pkt[14+9] == 1) { 
            uint8_t* icmp = pkt + 34;
            if(icmp[0] == 0) { 
                uint8_t ip1 = pkt[26]; uint8_t ip2 = pkt[27]; uint8_t ip3 = pkt[28]; uint8_t ip4 = pkt[29];
                int len = Utils::strlen(wins[0].buffer);
                if(len < 1900) {
                    Utils::strcpy(wins[0].buffer + len, "\nReply from ");
                    len = Utils::strlen(wins[0].buffer);
                    Utils::itoa(ip1, wins[0].buffer+len); len = Utils::strlen(wins[0].buffer); wins[0].buffer[len++]='.';
                    Utils::itoa(ip2, wins[0].buffer+len); len = Utils::strlen(wins[0].buffer); wins[0].buffer[len++]='.';
                    Utils::itoa(ip3, wins[0].buffer+len); len = Utils::strlen(wins[0].buffer); wins[0].buffer[len++]='.';
                    Utils::itoa(ip4, wins[0].buffer+len); len = Utils::strlen(wins[0].buffer); 
                    wins[0].buffer[len++]='\n'; wins[0].buffer[len++]='#'; wins[0].buffer[len++]=' '; wins[0].buffer[len]=0;
                    needs_redraw = true;
                }
            }
        }
        else if(pkt[12] == 0x08 && pkt[13] == 0x00 && pkt[14+9] == 17) {
            uint16_t dst_port = (pkt[36] << 8) | pkt[37];
            if(dst_port == 12345) {
                uint8_t ip1 = pkt[pkt_len - 8];
                uint8_t ip2 = pkt[pkt_len - 7];
                uint8_t ip3 = pkt[pkt_len - 6];
                uint8_t ip4 = pkt[pkt_len - 5];
                
                Utils::strcpy(browser_content, "Resolved Natively via DNS!\nIP Address: ");
                int cur = Utils::strlen(browser_content);
                Utils::itoa(ip1, browser_content+cur); cur = Utils::strlen(browser_content); browser_content[cur++] = '.';
                Utils::itoa(ip2, browser_content+cur); cur = Utils::strlen(browser_content); browser_content[cur++] = '.';
                Utils::itoa(ip3, browser_content+cur); cur = Utils::strlen(browser_content); browser_content[cur++] = '.';
                Utils::itoa(ip4, browser_content+cur); cur = Utils::strlen(browser_content); browser_content[cur] = 0;
                needs_redraw = true;
            }
        }
        
        rx_idx = (rx_idx + pkt_len + 4 + 3) & ~3;
        if(rx_idx >= 8192) rx_idx -= 8192;
        int capr = rx_idx - 16;
        if(capr < 0) capr += 8192;
        Port::out16(io_base + 0x38, capr);
    }
};
bool RTL8139::found = false;
uint32_t RTL8139::io_base = 0;
uint8_t RTL8139::mac[6] = {0};
uint8_t RTL8139::rx_buffer[8192 + 16] = {0};
int RTL8139::rx_idx = 0;
