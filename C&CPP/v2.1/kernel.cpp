typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

struct multiboot_info {
    uint32_t flags, mem_l, mem_u, boot_dev, cmdline, mods_c, mods_a;
    uint32_t syms[4], mmap_l, mmap_a, drives_l, drives_a, config;
    uint32_t loader, apm, vbe_ctrl, vbe_mode_inf;
    uint16_t vbe_mode, vbe_seg, vbe_off, vbe_len;
    uint64_t fb_addr; uint32_t fb_pitch, fb_width, fb_height;
    uint8_t fb_bpp, fb_type;
} __attribute__((packed));

extern "C" void kernel_main(uint32_t magic, multiboot_info* mb);

class Utils {
public:
    static int strlen(const char* s) { int i=0; while(s[i])i++; return i; }
    static void strcpy(char* d, const char* s) { while(*s) *d++ = *s++; *d=0; }
    static int strcmp(const char* s1, const char* s2) {
        while(*s1 && (*s1 == *s2)) { s1++; s2++; }
        return *(const unsigned char*)s1 - *(const unsigned char*)s2;
    }
    static int strncmp(const char* s1, const char* s2, int n) {
        while(n && *s1 && (*s1 == *s2)) { s1++; s2++; n--; }
        if(n == 0) return 0;
        return *(const unsigned char*)s1 - *(const unsigned char*)s2;
    }
    static void fill32(void* dest, uint32_t val, int count) {
        uint32_t* d = (uint32_t*)dest; while(count--) *d++ = val;
    }
    static void itoa(int n, char* buf) {
        if(n==0) { buf[0]='0'; buf[1]=0; return; }
        char t[16]; int i=0;
        while(n>0) { t[i++] = (n%10)+'0'; n/=10; }
        int j=0; while(i>0) buf[j++] = t[--i]; buf[j]=0;
    }
};

class Port {
public:
    static inline void outb(uint16_t p, uint8_t d) { __asm__ volatile("outb %0, %1"::"a"(d),"Nd"(p)); }
    static inline uint8_t inb(uint16_t p) { uint8_t r; __asm__ volatile("inb %1, %0":"=a"(r):"Nd"(p)); return r; }
    static inline void out16(uint16_t p, uint16_t d) { __asm__ volatile("outw %0, %1"::"a"(d),"Nd"(p)); }
    static inline uint16_t in16(uint16_t p) { uint16_t r; __asm__ volatile("inw %1, %0":"=a"(r):"Nd"(p)); return r; }
    static inline void out32(uint16_t p, uint32_t d) { __asm__ volatile("outl %0, %1"::"a"(d),"Nd"(p)); }
    static inline uint32_t in32(uint16_t p) { uint32_t r; __asm__ volatile("inl %1, %0":"=a"(r):"Nd"(p)); return r; }
    static inline void WaitWrite() { int t=10000; while((inb(0x64) & 2) && t--); }
    static inline void WaitRead() { int t=10000; while(!(inb(0x64) & 1) && t--); }
};

class Serial {
public:
    static void Init() {
        Port::outb(0x3F8 + 1, 0x00);
        Port::outb(0x3F8 + 3, 0x80);
        Port::outb(0x3F8 + 0, 0x03);
        Port::outb(0x3F8 + 1, 0x00);
        Port::outb(0x3F8 + 3, 0x03);
        Port::outb(0x3F8 + 2, 0xC7);
        Port::outb(0x3F8 + 4, 0x0B);
    }
    static int HasReceived() { return Port::inb(0x3F8 + 5) & 1; }
    static char Read() { while (HasReceived() == 0); return Port::inb(0x3F8); }
    static int IsTransmitEmpty() { return Port::inb(0x3F8 + 5) & 0x20; }
    static void WriteChar(char a) { while (IsTransmitEmpty() == 0); Port::outb(0x3F8, a); }
    static void Write(const char* str) { while(*str) WriteChar(*str++); }
};

class Screen {
public:
    static uint32_t* front;
    static uint32_t* back;
    static int w, h, pitch, size;
    static uint8_t* bg_image;

    static void Init(multiboot_info* mb) {
        front = (uint32_t*)(uint32_t)mb->fb_addr;
        w = mb->fb_width; h = mb->fb_height;
        pitch = mb->fb_pitch; 
        size = (pitch/4) * h;
        back = (uint32_t*)0x1000000; 
    }

    static void Swap() {
        uint8_t* f_row = (uint8_t*)front;
        uint32_t* b_ptr = back;
        for(int y=0; y<h; y++) {
            uint32_t* f_ptr = (uint32_t*)f_row;
            for(int x=0; x<w; x++) f_ptr[x] = *b_ptr++;
            f_row += pitch;
        }
    }

    static inline void Put(int x, int y, uint32_t c) {
        if(x>=0 && x<w && y>=0 && y<h) back[y*w+x] = c;
    }

    static uint32_t Blend(uint32_t bg, uint32_t fg, uint8_t a) {
        if(a == 0) return bg;
        if(a == 255) return fg;
        uint32_t rb = bg & 0xFF00FF; uint32_t g = bg & 0x00FF00;
        uint32_t rf = fg & 0xFF00FF; uint32_t gf = fg & 0x00FF00;
        rb += ((rf - rb) * a) >> 8;
        g  += ((gf - g) * a) >> 8;
        return (rb & 0xFF00FF) | (g & 0x00FF00);
    }
    
    static void PutAlpha(int x, int y, uint32_t c, uint8_t a) {
        if(x>=0 && x<w && y>=0 && y<h) back[y*w+x] = Blend(back[y*w+x], c, a);
    }

    static void Rect(int x, int y, int rw, int rh, uint32_t c) {
        if(x+rw > w) rw = w-x; 
        if(y+rh > h) rh = h-y; 
        if(rw<=0 || rh<=0) return;
        for(int j=y; j<y+rh; j++) {
            uint32_t* line = back + j*w + x;
            for(int i=0; i<rw; i++) line[i] = c;
        }
    }

    static void RectAlpha(int x, int y, int rw, int rh, uint32_t c, uint8_t a) {
        if(x+rw > w) rw = w-x; 
        if(y+rh > h) rh = h-y; 
        if(rw<=0 || rh<=0) return;
        for(int j=y; j<y+rh; j++) {
            for(int i=x; i<x+rw; i++) PutAlpha(i, j, c, a);
        }
    }
    
    static bool dark_mode;
    static int wallpaper_id;
    
    static void EraseCursorFront(int x, int y) {
        for(int r=0; r<16; r++) { 
            for(int c=0; c<12; c++) { 
                if(y+r>=0 && y+r<h && x+c>=0 && x+c<w) {
                    uint32_t bg = back[(y+r)*w + (x+c)];
                    front[(y+r)*(pitch/4) + (x+c)] = bg;
                }
            } 
        }
    }
    static void DrawCursorFront(int x, int y) {
        const uint8_t cursor[16][12] = {
            {1,0,0,0,0,0,0,0,0,0,0,0}, {1,1,0,0,0,0,0,0,0,0,0,0}, {1,2,1,0,0,0,0,0,0,0,0,0}, {1,2,2,1,0,0,0,0,0,0,0,0},
            {1,2,2,2,1,0,0,0,0,0,0,0}, {1,2,2,2,2,1,0,0,0,0,0,0}, {1,2,2,2,2,2,1,0,0,0,0,0}, {1,2,2,2,2,2,2,1,0,0,0,0},
            {1,2,2,2,2,2,2,2,1,0,0,0}, {1,2,2,2,2,2,2,2,2,1,0,0}, {1,2,2,2,2,1,1,1,1,1,1,0}, {1,2,1,2,1,0,0,0,0,0,0,0},
            {1,1,0,1,2,1,0,0,0,0,0,0}, {0,0,0,1,2,1,0,0,0,0,0,0}, {0,0,0,0,1,2,1,0,0,0,0,0}, {0,0,0,0,0,1,1,0,0,0,0,0}
        }; 
        for(int r=0; r<16; r++) { 
            for(int c=0; c<12; c++) { 
                uint8_t p = cursor[r][c]; 
                if(p!=0 && y+r>=0 && y+r<h && x+c>=0 && x+c<w) {
                    uint32_t color = (p==1) ? 0x000000 : 0xFFFFFF;
                    front[(y+r)*(pitch/4) + (x+c)] = color;
                }
            } 
        }
    }

    static void DrawBackground() {
        if(bg_image && bg_image[0] == 'B' && bg_image[1] == 'M') {
            int offset = *(uint32_t*)(bg_image + 10);
            int bw = *(int*)(bg_image + 18);
            int bh = *(int*)(bg_image + 22);
            short bpp = *(short*)(bg_image + 28);
            if(bpp == 24 || bpp == 32) {
                uint8_t* pixels = bg_image + offset;
                int row_bytes = ((bw * bpp + 31) / 32) * 4;
                for(int y=0; y<h; y++) {
                    int src_y = bh - 1 - (y % bh);
                    if(src_y < 0) src_y = 0;
                    uint8_t* src_row = pixels + (src_y * row_bytes);
                    for(int x=0; x<w; x++) {
                        int src_x = (x % bw) * (bpp/8);
                        uint32_t col = src_row[src_x] | (src_row[src_x+1] << 8) | (src_row[src_x+2] << 16);
                        if(dark_mode) col = Blend(col, 0x000000, 100);
                        back[y*w+x] = col;
                    }
                }
                return;
            }
        }
        
        for(int y=0; y<h; y++) {
            for(int x=0; x<w; x++) {
                uint32_t col = 0;
                if(wallpaper_id==0) { 
                    if(y>h/2+(x/4)) col=0xC026D3; else if(y>h/3-(x/4)) col=0x7E22CE; else col=0x3B0764;
                } else col=(x^y)&0xFF;
                if(dark_mode) col = Blend(col, 0x000000, 100);
                back[y*w+x] = col;
            }
        }
    }
};

uint32_t* Screen::front=0; uint32_t* Screen::back=0; int Screen::w=0; int Screen::h=0; int Screen::pitch=0; int Screen::size=0;
uint8_t* Screen::bg_image=0;
bool Screen::dark_mode=false; int Screen::wallpaper_id=0;

uint8_t font[128][8] = {
    {0,0,0,0,0,0,0,0}, {126,129,165,129,189,153,129,126}, {126,255,219,255,195,231,255,126}, {108,254,254,254,124,56,16,0},
    {16,56,124,254,254,254,108,0}, {24,60,60,231,231,60,60,24}, {24,60,126,255,255,126,60,24}, {24,126,153,153,24,24,24,24},
    {24,24,24,24,153,153,126,24}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0}, {24,60,60,24,24,0,24,0}, {102,102,102,0,0,0,0,0}, {108,108,254,108,254,108,108,0},
    {24,62,96,60,6,124,24,0}, {0,102,140,146,144,124,108,100}, {56,108,56,118,220,204,118,0}, {24,24,12,0,0,0,0,0},
    {12,24,48,48,48,24,12,0}, {48,24,12,12,12,24,48,0}, {0,102,60,255,60,102,0,0}, {0,24,24,126,24,24,0,0},
    {0,0,0,0,0,24,24,48}, {0,0,0,126,0,0,0,0}, {0,0,0,0,0,24,24,0}, {0,6,12,24,48,96,192,0},
    {60,102,102,102,102,102,60,0}, {24,56,24,24,24,24,126,0}, {60,102,6,12,48,96,126,0}, {60,102,6,28,6,102,60,0},
    {12,28,60,108,254,12,12,0}, {126,96,124,6,6,102,60,0}, {56,96,124,102,102,102,60,0}, {126,6,12,24,48,48,48,0},
    {60,102,60,60,102,60,0,0}, {60,102,102,62,6,12,56,0}, {0,24,24,0,24,24,0,0}, {0,24,24,0,24,24,48,0},
    {12,24,48,96,48,24,12,0}, {0,0,126,0,126,0,0,0}, {96,48,24,12,24,48,96,0}, {60,102,6,12,24,0,24,0},
    {60,102,110,110,96,62,0,0}, {24,60,102,102,126,102,102,0}, {124,102,102,124,102,102,124,0}, {60,102,96,96,96,102,60,0},
    {120,108,102,102,102,108,120,0}, {126,96,96,124,96,96,126,0}, {126,96,96,124,96,96,96,0}, {60,102,96,110,102,102,60,0},
    {102,102,102,126,102,102,102,0}, {60,24,24,24,24,24,60,0}, {30,12,12,12,12,108,56,0}, {102,108,120,112,120,108,102,0},
    {96,96,96,96,96,96,126,0}, {99,119,127,107,99,99,99,0}, {102,118,126,126,110,102,102,0}, {60,102,102,102,102,102,60,0},
    {124,102,102,124,96,96,96,0}, {60,102,102,102,106,100,58,0}, {124,102,102,124,112,108,102,0}, {60,102,96,60,6,102,60,0},
    {126,24,24,24,24,24,24,0}, {102,102,102,102,102,102,60,0}, {102,102,102,102,102,60,24,0}, {99,99,99,107,127,119,99,0},
    {102,102,60,24,60,102,102,0}, {102,102,102,60,24,24,24,0}, {126,6,12,24,48,96,126,0}, {60,48,48,48,48,48,60,0},
    {0,192,96,48,24,12,6,0}, {60,12,12,12,12,12,60,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,255},
    {24,12,0,0,0,0,0,0}, {0,0,56,68,68,68,56,0}, {128,128,240,136,136,136,240,0}, {0,0,56,64,64,64,56,0},
    {8,8,120,136,136,136,120,0}, {0,0,56,68,124,64,56,0}, {16,40,32,124,32,32,32,0}, {0,0,120,132,132,124,4,120},
    {128,128,240,136,136,136,136,0}, {24,0,96,32,32,32,112,0}, {12,0,12,12,12,12,12,56}, {128,128,144,160,192,160,144,0},
    {96,32,32,32,32,32,112,0}, {0,0,240,168,168,168,168,0}, {0,0,240,136,136,136,136,0}, {0,0,56,68,68,68,56,0},
    {0,0,240,136,136,240,128,128}, {0,0,120,136,136,120,8,8}, {0,0,240,136,128,128,128,0}, {0,0,56,64,56,4,120,0},
    {32,124,32,32,32,32,16,0}, {0,0,136,136,136,136,120,0}, {0,0,136,136,136,80,32,0}, {0,0,136,136,168,168,80,0},
    {0,0,136,80,32,80,136,0}, {0,0,136,136,120,8,120,0}, {0,0,124,8,16,32,124,0}, {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}
};

void DrawChar(int x, int y, char c, uint32_t color, int scale=1) {
    int idx = (unsigned char)c; if(idx>127) idx=0;
    for(int r=0; r<8; r++) {
        for(int k=0; k<8; k++) {
            if((font[idx][r] >> (7-k)) & 1) {
                if(scale==1) Screen::Put(x+k, y+r, color);
                else Screen::Rect(x+k*scale, y+r*scale, scale, scale, color);
            }
        }
    }
}
void DrawText(int x, int y, const char* s, uint32_t color, int scale=1) {
    while(*s) { DrawChar(x, y, *s, color, scale); x+=(8*scale)+1; s++; }
}



void DrawIcon(int x, int y, int type) {
    Screen::Rect(x, y, 32, 32, 0xFFFFFF);
    if(type==0) { Screen::Rect(x+1, y+1, 30, 30, 0x00AEEF); Screen::Rect(x+8, y+20, 16, 2, 0); Screen::Rect(x+8, y+8, 4, 8, 0); Screen::Rect(x+20, y+8, 4, 8, 0); }
    else if(type==1) { Screen::Rect(x, y, 32, 32, 0x333333); DrawText(x+4, y+4, ">_", 0x00FF00); }
    else if(type==2) { Screen::Rect(x+4, y+4, 24, 1, 0x999999); Screen::Rect(x+4, y+8, 24, 1, 0x999999); DrawText(x+20, y+20, "A", 0); }
    else if(type==3) { Screen::Rect(x, y, 32, 32, 0xDDDDDD); Screen::Rect(x+10, y+4, 12, 24, 0x555555); Screen::Rect(x+4, y+10, 24, 12, 0x555555); }
    else if(type==4) { Screen::Rect(x, y, 32, 32, 0x87CEEB); Screen::Rect(x+4, y+4, 24, 24, 0xFFFFFF); DrawText(x+5, y+12, "WWW", 0x0000FF); }
    else if(type==5) { Screen::Rect(x, y, 32, 32, 0xFF9900); Screen::Rect(x+4, y+8, 24, 4, 0); Screen::Rect(x+4, y+16, 24, 4, 0); Screen::Rect(x+4, y+24, 24, 4, 0); }
}

struct File { char name[32]; char content[512]; int size; bool used; };
File fs_files[16];

void FS_Init() {
    for(int i=0; i<16; i++) { fs_files[i].used = false; }
    Utils::strcpy(fs_files[0].name, "readme.txt"); Utils::strcpy(fs_files[0].content, "Welcome to TiwutOS!\nThis is a virtual FS in RAM."); fs_files[0].size=Utils::strlen(fs_files[0].content); fs_files[0].used=true;
    Utils::strcpy(fs_files[1].name, "todo.list"); Utils::strcpy(fs_files[1].content, "- Buy milk\n- Write OS kernel\n- Sleep"); fs_files[1].size=Utils::strlen(fs_files[1].content); fs_files[1].used=true;
}

int FS_Find(const char* name) {
    for(int i=0; i<16; i++) {
        if(fs_files[i].used && Utils::strcmp(fs_files[i].name, name) == 0) return i;
    }
    return -1;
}

void FS_Remove(const char* name) {
    int idx = FS_Find(name);
    if(idx != -1) fs_files[idx].used = false;
}

void FS_Create(const char* name) {
    if(FS_Find(name) != -1) return;
    for(int i=0; i<16; i++) { if(!fs_files[i].used) { 
        Utils::strcpy(fs_files[i].name, name); fs_files[i].content[0]=0; fs_files[i].size=0; fs_files[i].used=true; return; 
    }}
}

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

enum AppType { APP_TERM, APP_EDITOR, APP_SETTINGS, APP_BROWSER, APP_TASKMGR };
struct Window { bool active; int x, y, w, h; char title[32]; AppType type; char buffer[2048]; int cursor; };
extern Window wins[5];

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

char browser_content[4096] = "Welcome to the Native Real Internet!\nType a URL (e.g. google.com) and press Enter.\nTiwutOS will perform a native UDP DNS lookup!";

void DrawBrowser(Window* w, int cx, int cy) {
    Screen::Rect(w->x, w->y+30, w->w, 30, 0xDDDDDD);
    Screen::Rect(w->x+5, w->y+35, w->w-10, 20, 0xFFFFFF);
    DrawText(w->x+10, w->y+41, w->buffer, 0x000000);
    if(w->active) DrawText(w->x+10 + (Utils::strlen(w->buffer)*9), w->y+41, "|", 0);
    
    int content_y = cy + 30;
    Screen::Rect(w->x, content_y, w->w, w->h-60, 0xFFFFFF);
    
    int ly = content_y + 5;
    char* p = browser_content; char line[128]; int li=0;
    while(*p) {
        if(*p=='\n' || li>=(w->w/8 - 2)) { 
            line[li]=0; DrawText(cx, ly, line, 0x000000); ly+=12; li=0; 
            if(*p=='\n') p++; 
            if(ly > w->y + w->h - 15) break;
        }
        else { line[li++] = *p++; }
    }
    line[li]=0; DrawText(cx, ly, line, 0x000000);
}
Window wins[5]; Window* focused = nullptr; bool show_launchpad = false;
Window* drag_win = nullptr; bool resizing=false; int drag_off_x, drag_off_y;
int win_order[5] = {0, 1, 2, 3, 4};

int mx=400, my=300; uint8_t m_cyc=0; int8_t m_pkt[3]; bool m_left=false, m_prev_left=false;
bool shift_pressed = false;
bool caps_lock = false;

char kbd_map[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' '
};

char kbd_map_shift[128] = {
    0, 27, '!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,'A','S','D','F','G','H','J','K','L',':','"','~',
    0,'|','Z','X','C','V','B','N','M','<','>','?',0,'*',0,' '
};
bool kbd_e0 = false;
bool needs_redraw = true;

void MouseInit() {
    while(Port::inb(0x64)&1) Port::inb(0x60);
    Port::WaitWrite(); Port::outb(0x64, 0xA8);
    Port::WaitWrite(); Port::outb(0x64, 0x20); Port::WaitRead(); uint8_t status = Port::inb(0x60) | 2;
    Port::WaitWrite(); Port::outb(0x64, 0x60); Port::WaitWrite(); Port::outb(0x60, status);
    Port::WaitWrite(); Port::outb(0x64, 0xD4); Port::outb(0x60, 0xFF); Port::WaitRead(); Port::inb(0x60); Port::WaitRead(); Port::inb(0x60); Port::WaitRead(); Port::inb(0x60);
    Port::WaitWrite(); Port::outb(0x64, 0xD4); Port::outb(0x60, 0xF4); Port::WaitRead(); Port::inb(0x60);
}

void MouseUpdate() {
   while(Port::inb(0x64) & 1) {
       uint8_t status = Port::inb(0x64);
       if(status & 0x20) { 
           uint8_t b = Port::inb(0x60);
           if(m_cyc==0) { if((b&8)==8){ m_pkt[0]=b; m_cyc++; } }
           else if(m_cyc==1) { m_pkt[1]=b; m_cyc++; }
           else {
               m_pkt[2]=b; m_cyc=0; m_prev_left=m_left; m_left=(m_pkt[0]&1);
               int dx=m_pkt[1]; if(m_pkt[0]&0x10) dx|=0xFFFFFF00; int dy=m_pkt[2]; if(m_pkt[0]&0x20) dy|=0xFFFFFF00;
               mx+=dx; my-=dy; if(mx<0) mx=0; if(mx>=Screen::w) mx=Screen::w-1; if(my<0) my=0; if(my>=Screen::h) my=Screen::h-1;
           }
       } else {
           break;
       }
   }
}

void ExecCmd(char* cmd) {
    int len = Utils::strlen(wins[0].buffer);
    auto append = [&](const char* txt) {
        Utils::strcpy(wins[0].buffer + len, txt);
        len = Utils::strlen(wins[0].buffer);
    };

    if(Utils::strncmp(cmd, "ls", 2) == 0) {
        append("\n");
        for(int i=0; i<16; i++) {
            if(fs_files[i].used) { append(fs_files[i].name); append("  "); }
        }
    } else if(Utils::strncmp(cmd, "clear", 5) == 0) {
         Utils::strcpy(wins[0].buffer, "TiwutOS v2.1\n");
         len = Utils::strlen(wins[0].buffer);
    } else if(Utils::strncmp(cmd, "touch ", 6) == 0) {
        FS_Create(cmd+6);
    } else if(Utils::strncmp(cmd, "rm ", 3) == 0) {
        FS_Remove(cmd+3);
    } else if(Utils::strncmp(cmd, "cat ", 4) == 0) {
        int idx = FS_Find(cmd+4);
        if(idx != -1) { append("\n"); append(fs_files[idx].content); }
        else append("\ncat: No such file or directory");
    } else if(Utils::strncmp(cmd, "echo ", 5) == 0) {
        append("\n"); append(cmd+5);
    } else if(Utils::strncmp(cmd, "whoami", 6) == 0) {
        append("\nroot");
    } else if(Utils::strncmp(cmd, "uname", 5) == 0) {
        append("\nTiwutOS v2.1");
    } else if(Utils::strncmp(cmd, "ping ", 5) == 0) {
        int ip[4] = {0,0,0,0}; int p=0;
        for(int i=5; cmd[i]; i++) {
            if(cmd[i]=='.') p++;
            else if(cmd[i]>='0' && cmd[i]<='9' && p<4) ip[p] = ip[p]*10 + (cmd[i]-'0');
        }
        RTL8139::SendPing(ip[0], ip[1], ip[2], ip[3]);
        append("\nPinging... ");
        return; 
    } else if(Utils::strncmp(cmd, "neofetch", 8) == 0) {
        append("\n");
        append("   TTTTTTTTT    OS: TiwutOS v2.1\n");
        append("       T        Kernel: C++ Custom Monolithic\n");
        append("       T        Uptime: 10 mins\n");
        append("       T        Memory: 16 MB / 128 MB\n");
        append("       T        Shell: Tiwut Shell\n");
        append("       T        Resolution: "); 
        char res[16]; Utils::itoa(Screen::w, res); append(res); append("x"); Utils::itoa(Screen::h, res); append(res); append("\n");
        append("       T        Network: RTL8139 Native\n");
    } else if(Utils::strncmp(cmd, "pwd", 3) == 0) {
        append("\n/root");
    } else if(Utils::strncmp(cmd, "help", 4) == 0) {
        append("\nCommands: ls, clear, touch, rm, cat, echo, whoami, uname, pwd, ping, neofetch, help");
    } else if(Utils::strlen(cmd) > 0) {
        append("\nUnknown command: "); append(cmd);
    }
    
    if(Utils::strncmp(cmd, "clear", 5) != 0) append("\nroot@tiwut:~# ");
    else append("root@tiwut:~# ");
}

void KeyboardUpdate() {
    uint8_t status = Port::inb(0x64);
    if((status & 1) && !(status & 0x20)) {
        uint8_t sc=Port::inb(0x60);
        needs_redraw = true;
        if(sc==0xE0) { kbd_e0=true; return; }
        if(kbd_e0) { kbd_e0=false; if(sc==0x5B) show_launchpad=!show_launchpad; return; }
        if(sc == 0x2A || sc == 0x36) shift_pressed = true;
        else if(sc == 0xAA || sc == 0xB6) shift_pressed = false;
        else if(sc == 0x3A) caps_lock = !caps_lock;

        if(!(sc&0x80)) {
            char c = shift_pressed ? kbd_map_shift[sc] : kbd_map[sc];
            if(caps_lock && c >= 'a' && c <= 'z') c -= 32;
            else if(caps_lock && c >= 'A' && c <= 'Z' && !shift_pressed) c += 32;

            if(c && !show_launchpad && focused && focused->active) {
                if(focused->type == APP_TERM) {
                   int len = Utils::strlen(focused->buffer);
                   int prompt_idx = 0;
                   for(int k=0; k<len; k++) if(focused->buffer[k]=='#'&&focused->buffer[k+1]==' ') prompt_idx = k+2;
                   
                   if(c=='\b') { if(len > prompt_idx) focused->buffer[len-1]=0; }
                   else if(c=='\n') {
                       char cmd[64]; int ci=0; 
                       for(int k=prompt_idx; k<len; k++) cmd[ci++] = focused->buffer[k]; 
                       cmd[ci]=0;
                       ExecCmd(cmd);
                   }
                   else { if(len<2000) { focused->buffer[len]=c; focused->buffer[len+1]=0; } }
                } else if(focused->type == APP_EDITOR) {
                    if(c=='\b') { if(focused->cursor>0) focused->buffer[--focused->cursor]=0; }
                    else if(focused->cursor<1000) { focused->buffer[focused->cursor++]=c; focused->buffer[focused->cursor]=0; }
                } else if(focused->type == APP_BROWSER) {
                    if(c=='\b') { if(focused->cursor>0) focused->buffer[--focused->cursor]=0; }
                    else if(c=='\n') {
                        Utils::strcpy(browser_content, "Querying DNS natively...\n");
                        RTL8139::SendDNS(focused->buffer);
                    }
                    else if(focused->cursor<120) { focused->buffer[focused->cursor++]=c; focused->buffer[focused->cursor]=0; }
                }
            }
        }
    }
}

void OS_Init() { 
    MouseInit(); FS_Init(); Serial::Init();
    wins[0].active=false; wins[0].type=APP_TERM; Utils::strcpy(wins[0].title,"Terminal"); wins[0].x=100; wins[0].y=100; wins[0].w=400; wins[0].h=300; Utils::strcpy(wins[0].buffer,"TiwutOS v2.1\nroot@tiwut:~# ");
    wins[1].active=false; wins[1].type=APP_EDITOR; Utils::strcpy(wins[1].title,"Notes"); wins[1].x=150; wins[1].y=150; wins[1].w=400; wins[1].h=300; wins[1].cursor=0; wins[1].buffer[0]=0;
    wins[2].active=false; wins[2].type=APP_SETTINGS; Utils::strcpy(wins[2].title,"Settings"); wins[2].x=200; wins[2].y=200; wins[2].w=300; wins[2].h=200;
    wins[3].active=false; wins[3].type=APP_BROWSER; Utils::strcpy(wins[3].title,"Web Browser"); wins[3].x=250; wins[3].y=100; wins[3].w=500; wins[3].h=350;
    wins[4].active=false; wins[4].type=APP_TASKMGR; Utils::strcpy(wins[4].title,"Task Manager"); wins[4].x=300; wins[4].y=150; wins[4].w=400; wins[4].h=250;
    RTL8139::Init();
}

void DrawWindow(Window* w) {
    if(!w->active) return;
    
    Screen::RectAlpha(w->x+5, w->y+5, w->w, w->h, 0x000000, 80);
    
    uint32_t bg_color = Screen::dark_mode ? 0x222222 : 0xFFFFFF;
    uint32_t title_color = Screen::dark_mode ? 0x333333 : 0xEBEBEB;
    uint32_t text_color = Screen::dark_mode ? 0xFFFFFF : 0x000000;
    
    Screen::Rect(w->x, w->y, w->w, w->h, bg_color);
    Screen::Rect(w->x, w->y, w->w, 30, title_color);
    
    Screen::Rect(w->x+10, w->y+8, 12, 12, 0xFF5F56); Screen::Rect(w->x+30, w->y+8, 12, 12, 0xFFBD2E); Screen::Rect(w->x+50, w->y+8, 12, 12, 0x27C93F);
    Screen::Rect(w->x+w->w-12, w->y+w->h-12, 10, 10, 0x888888);
    
    int tx = w->x + (w->w/2) - (Utils::strlen(w->title)*4); DrawText(tx, w->y+8, w->title, Screen::dark_mode ? 0xDDDDDD : 0x555555);
    int cx = w->x+10; int cy = w->y+40;
    
    if(w->type==APP_TERM) { 
        Screen::Rect(w->x, w->y+30, w->w, w->h-30, 0x1E1E1E); 
        int ly=cy; char* p = w->buffer; char line[128]; int li=0;
        while(*p) {
            if(*p=='\n' || li>=(w->w/8 - 2)) { line[li]=0; DrawText(cx, ly, line, 0x00FF00); ly+=12; li=0; if(*p=='\n') p++; }
            else { line[li++] = *p++; }
        }
        line[li]=0; DrawText(cx, ly, line, 0x00FF00);
        DrawText(cx+(li*8), ly, "_", 0x00FF00);
    }
    else if(w->type==APP_EDITOR) { DrawText(cx, cy, w->buffer, text_color); DrawText(cx+(Utils::strlen(w->buffer)*8), cy, "|", text_color); }
    else if(w->type==APP_SETTINGS) { Screen::Rect(cx, cy, 20, 20, Screen::dark_mode?0x00FF00:0x888888); DrawText(cx+30, cy+6, "Dark Mode", text_color); Screen::Rect(cx, cy+40, 20, 20, 0x0000FF); DrawText(cx+30, cy+46, "Next Wallpaper", text_color); }
    else if(w->type==APP_BROWSER) { 
        DrawBrowser(w, cx, cy); 
    }
    else if(w->type==APP_TASKMGR) {
        Screen::Rect(cx, cy, w->w-20, 20, Screen::dark_mode ? 0x444444 : 0xEEEEEE);
        DrawText(cx+5, cy+5, "PID   PROCESS         RAM    STATUS", text_color);
        int ly = cy + 25;
        const char* names[] = {"Terminal", "Notes", "Settings", "Browser", "TaskMgr"};
        for(int i=0; i<5; i++) {
            char line[64];
            Utils::strcpy(line, "00"); Utils::itoa(i+1, line+2); 
            Utils::strcpy(line+4, "  "); Utils::strcpy(line+6, names[i]);
            int len = Utils::strlen(line); while(len < 22) line[len++] = ' '; line[len] = 0;
            Utils::strcpy(line+22, "1.2 MB ");
            if(wins[i].active) Utils::strcpy(line+29, " RUNNING");
            else Utils::strcpy(line+29, " SLEEPING");
            
            DrawText(cx+5, ly, line, text_color);
            ly += 20;
        }
        Screen::Rect(cx, ly+10, w->w-20, 1, 0x888888);
        DrawText(cx+5, ly+20, "CPU: 2%   MEM: 16MB / 128MB   TASKS: 5", text_color);
    }
}

void OS_Draw() {
    Screen::DrawBackground(); Screen::RectAlpha(0, 0, Screen::w, 28, 0xFFFFFF, 180);
    DrawText(10, 8, "TiwutOS", 0); DrawText(Screen::w-80, 8, "22:07", 0);
    for(int i=0; i<5; i++) DrawWindow(&wins[win_order[i]]);
    
    int dX=(Screen::w-400)/2; int dY=Screen::h-80;
    Screen::RectAlpha(dX, dY, 400, 64, 0xFFFFFF, 100);
    DrawIcon(dX+20, dY+16, 5); DrawIcon(dX+80, dY+16, 1); DrawIcon(dX+140, dY+16, 2); DrawIcon(dX+200, dY+16, 3); DrawIcon(dX+270, dY+16, 4);
    if(show_launchpad) { Screen::RectAlpha(0, 0, Screen::w, Screen::h, 0, 150); int cx=Screen::w/2-200; int cy=Screen::h/2-100; DrawIcon(cx,cy,5); DrawText(cx,cy+40,"TaskMgr",-1); DrawIcon(cx+100,cy,1); DrawText(cx+100,cy+40,"Terminal",-1); DrawIcon(cx+200,cy,2); DrawText(cx+200,cy+40,"Notes",-1); DrawIcon(cx+300,cy,3); DrawText(cx+300,cy+40,"Settings",-1); DrawIcon(cx,cy+100,4); DrawText(cx,cy+140,"Browser",-1); }
    Screen::Swap();
}

void BringToFront(int idx) {
    int pos = -1;
    for(int i=0; i<5; i++) if(win_order[i] == idx) pos=i;
    for(int i=pos; i<4; i++) win_order[i] = win_order[i+1];
    win_order[4] = idx;
}

void OS_Update() {
   MouseUpdate(); KeyboardUpdate();
   if(RTL8139::found) RTL8139::PollReceive();
   while(Serial::HasReceived()) {
       char c = Serial::Read();
       int l = Utils::strlen(browser_content);
       if(Utils::strcmp(browser_content, "Loading...\n") == 0) { browser_content[0] = 0; l = 0; }
       if(l < 4000 && (c == '\n' || (c >= 32 && c <= 126))) {
           browser_content[l] = c;
           browser_content[l+1] = 0;
           needs_redraw = true;
       }
   }
   if(m_left) {
       if(!drag_win && !m_prev_left) {
           if(show_launchpad) { show_launchpad=false; }
           else {
               bool hit = false;
               for(int k=4; k>=0; k--) {
                   int i = win_order[k];
                   if(wins[i].active) {
                       if(mx >= wins[i].x && mx <= wins[i].x+wins[i].w && my >= wins[i].y && my <= wins[i].y+wins[i].h) {
                           BringToFront(i); focused = &wins[i]; hit = true;
                           if(mx >= wins[i].x+wins[i].w-15 && my >= wins[i].y+wins[i].h-15) { drag_win = &wins[i]; resizing = true; drag_off_x = mx - wins[i].w; drag_off_y = my - wins[i].h; }
                           else if(my <= wins[i].y+30) { drag_win = &wins[i]; resizing = false; drag_off_x = mx - wins[i].x; drag_off_y = my - wins[i].y; }
                           if(my <= wins[i].y+30 && mx <= wins[i].x+25) wins[i].active = false;
                           return;
                       }
                   }
               }
               if(!hit) {
                   int dX=(Screen::w-400)/2; int dY=Screen::h-80;
                   if(my>dY) {
                       if(mx>dX+20 && mx<dX+52) { wins[4].active=!wins[4].active; if(wins[4].active) BringToFront(4); focused=&wins[4]; }
                       if(mx>dX+80 && mx<dX+112) { wins[0].active=!wins[0].active; if(wins[0].active) BringToFront(0); focused=&wins[0]; }
                       if(mx>dX+140 && mx<dX+172) { wins[1].active=!wins[1].active; if(wins[1].active) BringToFront(1); focused=&wins[1]; }
                       if(mx>dX+200 && mx<dX+232) { wins[2].active=!wins[2].active; if(wins[2].active) BringToFront(2); focused=&wins[2]; }
                       if(mx>dX+270 && mx<dX+302) { wins[3].active=!wins[3].active; if(wins[3].active) BringToFront(3); focused=&wins[3]; }
                   }
               }
           }
       }
       else if(drag_win) {
           if(resizing) { drag_win->w = mx - drag_off_x; drag_win->h = my - drag_off_y; if(drag_win->w < 100) drag_win->w = 100; if(drag_win->h < 100) drag_win->h = 100; } 
           else { drag_win->x = mx - drag_off_x; drag_win->y = my - drag_off_y; }
       }
   } else { drag_win = nullptr; resizing = false; }
}

struct multiboot_module { uint32_t mod_start, mod_end, string, reserved; };

extern "C" void kernel_main(uint32_t magic, multiboot_info* mb) {
     (void)magic; if(mb->fb_addr==0)return;
     if((mb->flags & 8) && mb->mods_c > 0) {
         multiboot_module* mod = (multiboot_module*)mb->mods_a;
         Screen::bg_image = (uint8_t*)mod->mod_start;
     }
     Screen::Init(mb); OS_Init();
     
     int old_mx = -1, old_my = -1;
     while(true) { 
         OS_Update(); 
         
         if (m_left) needs_redraw = true;
         
         if (needs_redraw) {
             OS_Draw();
             Screen::DrawCursorFront(mx, my);
             old_mx = mx; old_my = my;
             needs_redraw = false;
         } else {
             if (mx != old_mx || my != old_my) {
                 if (old_mx != -1) {
                     Screen::EraseCursorFront(old_mx, old_my);
                 }
                 Screen::DrawCursorFront(mx, my);
                 old_mx = mx; old_my = my;
             }
         }
     }
}