typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

class Utils {
public:
    static int strlen(const char* s) { int i=0; while(s[i])i++; return i; }
    static void strcpy(char* d, const char* s) { while(*s) *d++ = *s++; *d=0; }
    static void fill32(void* dest, uint32_t val, int count) {
        uint32_t* d = (uint32_t*)dest; while(count--) *d++ = val;
    }
    static void copy32(void* dest, void* src, int count) {
        uint32_t* d = (uint32_t*)dest; uint32_t* s = (uint32_t*)src;
        while(count--) *d++ = *s++;
    }
};

class Port {
public:
    static inline void outb(uint16_t p, uint8_t d) { __asm__ volatile("outb %0, %1"::"a"(d),"Nd"(p)); }
    static inline uint8_t inb(uint16_t p) { uint8_t r; __asm__ volatile("inb %1, %0":"=a"(r):"Nd"(p)); return r; }
};

struct multiboot_info {
    uint32_t flags, mem_l, mem_u, boot_dev, cmdline, mods_c, mods_a;
    uint32_t syms[4], mmap_l, mmap_a, drives_l, drives_a, config;
    uint32_t loader, apm, vbe_ctrl, vbe_mode_inf;
    uint16_t vbe_mode, vbe_seg, vbe_off, vbe_len;
    uint64_t fb_addr; uint32_t fb_pitch, fb_width, fb_height;
    uint8_t fb_bpp, fb_type;
} __attribute__((packed));

class Screen {
public:
    static uint32_t* front;
    static uint32_t* back;
    static int w, h, size;

    static void Init(multiboot_info* mb) {
        front = (uint32_t*)(uint32_t)mb->fb_addr;
        w = mb->fb_width; h = mb->fb_height;
        size = w * h;
        back = (uint32_t*)0x1000000;
    }

    static void Clear(uint32_t color) { Utils::fill32(back, color, size); }
    static void Swap() { Utils::copy32(front, back, size); }

    static inline void Put(int x, int y, uint32_t c) {
        if(x>=0 && x<w && y>=0 && y<h) back[y*w+x] = c;
    }

    static void Rect(int x, int y, int rw, int rh, uint32_t c) {
        if(x+rw > w) rw = w-x; if(y+rh > h) rh = h-y;
        if(rw<=0 || rh<=0) return;
        for(int j=y; j<y+rh; j++) {
            uint32_t* line = back + j*w + x;
            for(int i=0; i<rw; i++) line[i] = c;
        }
    }

    static void RoundedRect(int x, int y, int rw, int rh, int r, uint32_t c) {
        Rect(x+r, y, rw-2*r, rh, c);
        Rect(x, y+r, r, rh-2*r, c);
        Rect(x+rw-r, y+r, r, rh-2*r, c);
        Rect(x+1, y+1, r, r, c); 
        Rect(x+rw-r-1, y+1, r, r, c);
        Rect(x+1, y+rh-r-1, r, r, c);
        Rect(x+rw-r-1, y+rh-r-1, r, r, c);
    }

    static void GradientBackground() {
        for(int y=0; y<h; y++) {
            uint8_t r = 30 + (y * 20 / h);
            uint8_t g = 30 + (y * 20 / h);
            uint8_t b = 60 + (y * 40 / h);
            uint32_t col = (r<<16) | (g<<8) | b;
            uint32_t* line = back + y*w;
            for(int x=0; x<w; x++) line[x] = col;
        }
    }
};
uint32_t* Screen::front=0; uint32_t* Screen::back=0; int Screen::w=0; int Screen::h=0; int Screen::size=0;

uint8_t font[128][8] = {
    {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, 
    {0,0,0,0,0,0,0,0},
    {24,60,60,24,24,0,24,0},{34,34,34,0,0,0,0,0},{108,108,254,108,254,108,108,0},
    {24,62,96,60,6,124,24,0},{0,98,100,8,16,38,70,0},{56,108,56,118,220,204,118,0},{24,24,12,0,0,0,0,0},
    {12,24,48,48,48,24,12,0},{48,24,12,12,12,24,48,0},{0,102,60,255,60,102,0,0},{0,24,24,126,24,24,0,0},
    {0,0,0,0,0,24,24,48},{0,0,0,126,0,0,0,0},{0,0,0,0,0,24,24,0},{0,6,12,24,48,96,192,0},
    {60,102,102,102,102,102,60,0},{24,56,24,24,24,24,126,0},{60,102,6,12,48,96,126,0},{60,102,6,28,6,102,60,0},
    {12,28,60,108,254,12,12,0},{126,96,124,6,6,102,60,0},{56,96,124,102,102,102,60,0},{126,6,12,24,48,48,48,0},
    {60,102,60,60,102,60,0,0},{60,102,102,62,6,12,56,0},{0,24,24,0,24,24,0,0},{0,24,24,0,24,24,48,0},
    {12,24,48,96,48,24,12,0},{0,0,126,0,126,0,0,0},{96,48,24,12,24,48,96,0},{60,102,6,12,24,0,24,0},
    {60,102,110,110,96,62,0,0},{24,60,102,102,126,102,102,0},{124,102,102,124,102,102,124,0},{60,102,96,96,96,102,60,0},
    {120,108,102,102,102,108,120,0},{126,96,96,124,96,96,126,0},{126,96,96,124,96,96,96,0},{60,102,96,110,102,102,60,0},
    {102,102,102,126,102,102,102,0},{60,24,24,24,24,24,60,0},{30,12,12,12,12,108,56,0},{102,108,120,112,120,108,102,0},
    {96,96,96,96,96,96,126,0},{99,119,127,107,99,99,99,0},{102,118,126,126,110,102,102,0},{60,102,102,102,102,102,60,0},
    {124,102,102,124,96,96,96,0},{60,102,102,102,106,100,58,0},{124,102,102,124,112,108,102,0},{60,102,96,60,6,102,60,0},
    {126,24,24,24,24,24,24,0},{102,102,102,102,102,102,60,0},{102,102,102,102,102,60,24,0},{99,99,99,107,127,119,99,0},
    {102,102,60,24,60,102,102,0},{102,102,102,60,24,24,24,0},{126,6,12,24,48,96,126,0},{60,48,48,48,48,48,60,0},
    {0,192,96,48,24,12,6,0},{60,12,12,12,12,12,60,0},{16,56,124,254,0,0,0,0},{0,0,0,0,0,0,0,255},
    {24,12,0,0,0,0,0,0},{0,0,56,68,68,68,56,0},{128,128,240,136,136,136,240,0},{0,0,56,64,64,64,56,0},
    {8,8,120,136,136,136,120,0},{0,0,56,68,124,64,56,0},{16,40,32,124,32,32,32,0},{0,0,120,132,132,124,4,120},
    {128,128,240,136,136,136,136,0},{24,0,96,32,32,32,112,0},{12,0,12,12,12,12,12,56},{128,128,144,160,192,160,144,0},
    {96,32,32,32,32,32,112,0},{0,0,240,168,168,168,168,0},{0,0,240,136,136,136,136,0},{0,0,56,68,68,68,56,0},
    {0,0,240,136,136,240,128,128},{0,0,120,136,136,120,8,8},{0,0,240,136,128,128,128,0},{0,0,56,64,56,4,120,0},
    {32,124,32,32,32,32,16,0},{0,0,136,136,136,136,120,0},{0,0,136,136,136,80,32,0},{0,0,136,136,168,168,80,0},
    {0,0,136,80,32,80,136,0},{0,0,136,136,120,8,120,0},{0,0,124,8,16,32,124,0},{0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0}
};

void DrawChar(int x, int y, char c, uint32_t color) {
    int idx = (unsigned char)c;
    if(idx > 127) idx = 0;
    for(int r=0; r<8; r++) {
        for(int k=0; k<8; k++) {
            if((font[idx][r] >> (7-k)) & 1) Screen::Put(x+k, y+r, color);
        }
    }
}

void DrawText(int x, int y, const char* s, uint32_t color) {
    while(*s) { DrawChar(x, y, *s, color); x+=8; s++; }
}

int mx=512, my=384;
uint8_t m_cyc=0; int8_t m_pkt[3];
bool m_left = false;
bool m_prev_left = false;

void MouseUpdate() {
    uint8_t status = Port::inb(0x64);
    
    if(status & 1) {
        uint8_t b = Port::inb(0x60);
        
        if(status & 0x20) {
            
            if(m_cyc == 0 && !(b & 0x08)) {
                return;
            }

            if(m_cyc==0) { m_pkt[0]=b; m_cyc++; }
            else if(m_cyc==1) { m_pkt[1]=b; m_cyc++; }
            else {
                m_pkt[2]=b; m_cyc=0;
                
                m_prev_left = m_left;
                m_left = (m_pkt[0] & 1);
                
                mx += m_pkt[1];
                my -= m_pkt[2];
                
                if(mx < 0) mx = 0;
                if(mx >= Screen::w) mx = Screen::w - 1;
                if(my < 0) my = 0;
                if(my >= Screen::h) my = Screen::h - 1;
            }
        }
    }
}

struct Window { bool active; int x, y, w, h; char title[16]; int anim_y; };
Window win_term;
bool start_menu_open = false;

void OS_Init() {
    Port::outb(0x64, 0xA8);
    Port::outb(0x64, 0xD4);
    Port::outb(0x60, 0xF4); 
    Port::inb(0x60);
    
    win_term.active = true;
    win_term.x = 200; win_term.y = 200; win_term.w = 500; win_term.h = 350;
    win_term.anim_y = 200;
    Utils::strcpy(win_term.title, "Terminal");
}

void OS_Update() {
    MouseUpdate();
    
    if(m_left && !m_prev_left) {
        
        if(my > 700) {
            if(mx > 470 && mx < 530) start_menu_open = !start_menu_open;
            if(mx > 550 && mx < 600) { win_term.active=true; win_term.anim_y=win_term.y; }
            if(mx > 610 && mx < 680) Port::outb(0x64, 0xFE);
        }
        
        if(win_term.active) {
            if(mx > win_term.x+10 && mx < win_term.x+22 && my > win_term.y+10 && my < win_term.y+22) {
                win_term.active = false;
            }
        }
    }

    if(m_left && mx > win_term.x && mx < win_term.x+win_term.w && my > win_term.y && my < win_term.y+30) {
        win_term.x = mx - (win_term.w/2);
        win_term.y = my - 15;
    }
}

void OS_Draw() {
    Screen::GradientBackground();
    
    Screen::RoundedRect(200, 700, 624, 60, 15, 0x44475a); 
    DrawText(480, 725, "START", 0xFFFFFF);
    DrawText(560, 725, "APPS", 0xFFFFFF);
    DrawText(620, 725, "SHUTDOWN", 0xFFFFFF);

    if(start_menu_open) {
        Screen::RoundedRect(470, 590, 150, 100, 5, 0xF8F8F2);
        DrawText(480, 600, "1. Terminal", 0x000000);
        DrawText(480, 620, "2. Settings", 0x000000);
        DrawText(480, 640, "3. Logout", 0x000000);
    }

    if(win_term.active) {
        int wx = win_term.x; int wy = win_term.y;
        Screen::RoundedRect(wx, wy, win_term.w, win_term.h, 10, 0xf8f8f2);
        Screen::RoundedRect(wx, wy, win_term.w, 30, 10, 0x6272a4);
        
        Screen::RoundedRect(wx+10, wy+10, 12, 12, 5, 0xFF5555);
        Screen::RoundedRect(wx+30, wy+10, 12, 12, 5, 0xF1FA8C);
        Screen::RoundedRect(wx+50, wy+10, 12, 12, 5, 0x50FA7B);
        
        DrawText(wx+70, wy+10, win_term.title, 0xFFFFFF);
        DrawText(wx+15, wy+40, "root@tiwut:~$ ", 0x282a36);
    }

    Screen::Rect(mx, my, 8, 8, 0xFFFFFF);
    Screen::Rect(mx+1, my+1, 6, 6, 0x000000);
    
    Screen::Swap();
}

extern "C" void kernel_main(uint32_t magic, multiboot_info* mb) {
    if(mb->fb_addr == 0) return;
    Screen::Init(mb);
    OS_Init();

    while(true) {
        OS_Update();
        OS_Draw();
    }
}