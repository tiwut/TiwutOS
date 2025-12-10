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
    static void fill32(void* dest, uint32_t val, int count) {
        uint32_t* d = (uint32_t*)dest; while(count--) *d++ = val;
    }
};

class Port {
public:
    static inline void outb(uint16_t p, uint8_t d) { __asm__ volatile("outb %0, %1"::"a"(d),"Nd"(p)); }
    static inline uint8_t inb(uint16_t p) { uint8_t r; __asm__ volatile("inb %1, %0":"=a"(r):"Nd"(p)); return r; }
    static inline void WaitWrite() { int t=10000; while((inb(0x64) & 2) && t--); }
    static inline void WaitRead() { int t=10000; while(!(inb(0x64) & 1) && t--); }
};

class Screen {
public:
    static uint32_t* front;
    static uint32_t* back;
    static int w, h, pitch, size;

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
    
    static void DrawBackground() {
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

void DrawChar(int x, int y, char c, uint32_t color) {
    int idx = (unsigned char)c; if(idx>127) idx=0;
    for(int r=0; r<8; r++) for(int k=0; k<8; k++) 
        if((font[idx][r] >> (7-k)) & 1) Screen::Put(x+k, y+r, color);
}
void DrawText(int x, int y, const char* s, uint32_t color) {
    while(*s) { DrawChar(x, y, *s, color); x+=8; s++; }
}

void DrawCursor(int x, int y) {
    const uint8_t cursor[16][12] = {
        {1,0,0,0,0,0,0,0,0,0,0,0}, {1,1,0,0,0,0,0,0,0,0,0,0}, {1,2,1,0,0,0,0,0,0,0,0,0}, {1,2,2,1,0,0,0,0,0,0,0,0},
        {1,2,2,2,1,0,0,0,0,0,0,0}, {1,2,2,2,2,1,0,0,0,0,0,0}, {1,2,2,2,2,2,1,0,0,0,0,0}, {1,2,2,2,2,2,2,1,0,0,0,0},
        {1,2,2,2,2,2,2,2,1,0,0,0}, {1,2,2,2,2,2,2,2,2,1,0,0}, {1,2,2,2,2,1,1,1,1,1,1,0}, {1,2,1,2,1,0,0,0,0,0,0,0},
        {1,1,0,1,2,1,0,0,0,0,0,0}, {0,0,0,1,2,1,0,0,0,0,0,0}, {0,0,0,0,1,2,1,0,0,0,0,0}, {0,0,0,0,0,1,1,0,0,0,0,0}
    }; 
    for(int r=0; r<16; r++) { for(int c=0; c<12; c++) { uint8_t p = cursor[r][c]; if(p==1) Screen::Put(x+c, y+r, 0x000000); else if(p==2) Screen::Put(x+c, y+r, 0xFFFFFF); } }
}

void DrawIcon(int x, int y, int type) {
    Screen::Rect(x, y, 32, 32, 0xFFFFFF);
    if(type==0) { Screen::Rect(x+1, y+1, 30, 30, 0x00AEEF); Screen::Rect(x+8, y+20, 16, 2, 0); Screen::Rect(x+8, y+8, 4, 8, 0); Screen::Rect(x+20, y+8, 4, 8, 0); }
    else if(type==1) { Screen::Rect(x, y, 32, 32, 0x333333); DrawText(x+4, y+4, ">_", 0x00FF00); }
    else if(type==2) { Screen::Rect(x+4, y+4, 24, 1, 0x999999); Screen::Rect(x+4, y+8, 24, 1, 0x999999); DrawText(x+20, y+20, "A", 0); }
    else if(type==3) { Screen::Rect(x, y, 32, 32, 0xDDDDDD); Screen::Rect(x+10, y+4, 12, 24, 0x555555); Screen::Rect(x+4, y+10, 24, 12, 0x555555); }
    else if(type==4) { Screen::Rect(x, y, 32, 32, 0x87CEEB); Screen::Rect(x+2, y+10, 28, 20, 0x4682B4); }
}

struct File { char name[32]; char content[512]; int size; bool used; };
File fs_files[16];

void FS_Init() {
    for(int i=0; i<16; i++) { fs_files[i].used = false; }
    Utils::strcpy(fs_files[0].name, "readme.txt"); Utils::strcpy(fs_files[0].content, "Welcome to TiwutOS!\nThis is a virtual FS in RAM."); fs_files[0].size=Utils::strlen(fs_files[0].content); fs_files[0].used=true;
    Utils::strcpy(fs_files[1].name, "todo.list"); Utils::strcpy(fs_files[1].content, "- Buy milk\n- Write OS kernel\n- Sleep"); fs_files[1].size=Utils::strlen(fs_files[1].content); fs_files[1].used=true;
}

void FS_Create(char* name) {
    for(int i=0; i<16; i++) { if(!fs_files[i].used) { 
        Utils::strcpy(fs_files[i].name, name); fs_files[i].content[0]=0; fs_files[i].size=0; fs_files[i].used=true; return; 
    }}
}

enum AppType { APP_TERM, APP_EDITOR, APP_SETTINGS, APP_FILES };
struct Window { bool active; int x, y, w, h; char title[32]; AppType type; char buffer[2048]; int cursor; };
Window wins[4]; Window* focused = nullptr; bool show_launchpad = false;
Window* drag_win = nullptr; bool resizing=false; int drag_off_x, drag_off_y;
int win_order[4] = {0, 1, 2, 3};

int mx=400, my=300; uint8_t m_cyc=0; int8_t m_pkt[3]; bool m_left=false, m_prev_left=false;
char kbd_map[128] = { 0, 27, '1','2','3','4','5','6','7','8','9','0','-','=','\b','\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' ' };
bool kbd_e0 = false;

void MouseInit() {
    while(Port::inb(0x64)&1) Port::inb(0x60);
    Port::WaitWrite(); Port::outb(0x64, 0xA8);
    Port::WaitWrite(); Port::outb(0x64, 0x20); Port::WaitRead(); uint8_t status = Port::inb(0x60) | 2;
    Port::WaitWrite(); Port::outb(0x64, 0x60); Port::WaitWrite(); Port::outb(0x60, status);
    Port::WaitWrite(); Port::outb(0x64, 0xD4); Port::outb(0x60, 0xFF); Port::WaitRead(); Port::inb(0x60); Port::WaitRead(); Port::inb(0x60); Port::WaitRead(); Port::inb(0x60);
    Port::WaitWrite(); Port::outb(0x64, 0xD4); Port::outb(0x60, 0xF4); Port::WaitRead(); Port::inb(0x60);
}

void MouseUpdate() {
   if(Port::inb(0x64) & 1) {
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
       }
   }
}

void ExecCmd(char* cmd) {
    if(cmd[0]=='l' && cmd[1]=='s') {
        int l = Utils::strlen(wins[0].buffer);
        Utils::strcpy(wins[0].buffer+l, "\nFILES:\n"); l=Utils::strlen(wins[0].buffer);
        for(int i=0; i<16; i++) {
            if(fs_files[i].used) {
                Utils::strcpy(wins[0].buffer+l, fs_files[i].name); l=Utils::strlen(wins[0].buffer);
                Utils::strcpy(wins[0].buffer+l, "  "); l=Utils::strlen(wins[0].buffer);
            }
        }
        Utils::strcpy(wins[0].buffer+l, "\n>_");
    } else if(cmd[0]=='c' && cmd[1]=='l' && cmd[2]=='e' && cmd[3]=='a' && cmd[4]=='r') {
         Utils::strcpy(wins[0].buffer, "TiwutOS v1.5\n>_");
    } else if(cmd[0]=='t' && cmd[1]=='o' && cmd[2]=='u' && cmd[3]=='c' && cmd[4]=='h') {
        if(cmd[5]==' ') FS_Create(cmd+6);
        int l = Utils::strlen(wins[0].buffer); Utils::strcpy(wins[0].buffer+l, "\n>_");
    } else {
        int l = Utils::strlen(wins[0].buffer); Utils::strcpy(wins[0].buffer+l, "\nUnknown Cmd\n>_");
    }
}

void KeyboardUpdate() {
    uint8_t status = Port::inb(0x64);
    if((status & 1) && !(status & 0x20)) {
        uint8_t sc=Port::inb(0x60);
        if(sc==0xE0) { kbd_e0=true; return; }
        if(kbd_e0) { kbd_e0=false; if(sc==0x5B) show_launchpad=!show_launchpad; return; }
        if(!(sc&0x80)) {
            char c = kbd_map[sc];
            if(c && !show_launchpad && focused && focused->active) {
                if(focused->type == APP_TERM) {
                   int len = Utils::strlen(focused->buffer);
                   if(c=='\b') { if(len>0 && focused->buffer[len-1]!='>' && focused->buffer[len-1]!='\n') focused->buffer[len-1]=0; }
                   else if(c=='\n') {
                       char* last_line = focused->buffer;
                       int i=0; while(focused->buffer[i]) { if(focused->buffer[i]=='>' && focused->buffer[i+1]=='_') last_line = focused->buffer+i+2; i++; }
                       char cmd[64]; int ci=0; 
                       int start=0; for(int k=0; k<len; k++) if(focused->buffer[k]=='>'&&focused->buffer[k+1]=='_') start=k+2;
                       for(int k=start; k<len; k++) cmd[ci++] = focused->buffer[k]; cmd[ci]=0;
                       ExecCmd(cmd);
                   }
                   else { if(len<2000) { focused->buffer[len]=c; focused->buffer[len+1]=0; } }
                } else if(focused->type == APP_EDITOR) {
                    if(c=='\b') { if(focused->cursor>0) focused->buffer[--focused->cursor]=0; }
                    else if(focused->cursor<1000) { focused->buffer[focused->cursor++]=c; focused->buffer[focused->cursor]=0; }
                }
            }
        }
    }
}

void OS_Init() { 
    MouseInit(); FS_Init();
    wins[0].active=false; wins[0].type=APP_TERM; Utils::strcpy(wins[0].title,"Terminal"); wins[0].x=100; wins[0].y=100; wins[0].w=400; wins[0].h=300; Utils::strcpy(wins[0].buffer,"TiwutOS\n>_");
    wins[1].active=false; wins[1].type=APP_EDITOR; Utils::strcpy(wins[1].title,"Notes"); wins[1].x=150; wins[1].y=150; wins[1].w=400; wins[1].h=300; wins[1].cursor=0; wins[1].buffer[0]=0;
    wins[2].active=false; wins[2].type=APP_SETTINGS; Utils::strcpy(wins[2].title,"Settings"); wins[2].x=200; wins[2].y=200; wins[2].w=300; wins[2].h=200;
    wins[3].active=false; wins[3].type=APP_FILES; Utils::strcpy(wins[3].title,"Finder"); wins[3].x=250; wins[3].y=100; wins[3].w=500; wins[3].h=350;
}

void DrawWindow(Window* w) {
    if(!w->active) return;
    Screen::RectAlpha(w->x+10, w->y+10, w->w, w->h, 0x000000, 50);
    Screen::Rect(w->x, w->y, w->w, w->h, 0xFFFFFF);
    Screen::Rect(w->x, w->y, w->w, 30, 0xEBEBEB);
    Screen::Rect(w->x+10, w->y+8, 12, 12, 0xFF5F56); Screen::Rect(w->x+30, w->y+8, 12, 12, 0xFFBD2E); Screen::Rect(w->x+50, w->y+8, 12, 12, 0x27C93F);
    Screen::Rect(w->x+w->w-10, w->y+w->h-10, 10, 10, 0xAAAAAA);
    int tx = w->x + (w->w/2) - (Utils::strlen(w->title)*4); DrawText(tx, w->y+8, w->title, 0x555555);
    int cx = w->x+10; int cy = w->y+40;
    if(w->type==APP_TERM) { 
        int ly=cy; char* p = w->buffer; char line[64]; int li=0;
        while(*p) {
            if(*p=='\n' || li>=50) { line[li]=0; DrawText(cx, ly, line, 0); ly+=10; li=0; if(*p=='\n') p++; }
            else { line[li++] = *p++; }
        }
        line[li]=0; DrawText(cx, ly, line, 0);
    }
    else if(w->type==APP_EDITOR) { DrawText(cx, cy, w->buffer, 0x000000); DrawText(cx+(Utils::strlen(w->buffer)*8), cy, "|", 0); }
    else if(w->type==APP_SETTINGS) { Screen::Rect(cx, cy, 20, 20, Screen::dark_mode?0x00FF00:0x888888); DrawText(cx+30, cy+6, "Dark Mode", 0); Screen::Rect(cx, cy+40, 20, 20, 0x0000FF); DrawText(cx+30, cy+46, "Next Wallpaper", 0); }
    else if(w->type==APP_FILES) { 
        int fx=0, fy=0;
        for(int i=0; i<16; i++) { 
            if(fs_files[i].used) {
                DrawIcon(cx+fx*70, cy+fy*70, 4); DrawText(cx+fx*70, cy+fy*70+40, fs_files[i].name, 0); 
                fx++; if(fx>5) { fx=0; fy++; }
            }
        } 
    }
}

void OS_Draw() {
    Screen::DrawBackground(); Screen::RectAlpha(0, 0, Screen::w, 28, 0xFFFFFF, 180);
    DrawText(10, 8, "TiwutOS", 0); DrawText(Screen::w-80, 8, "22:07", 0);
    for(int i=0; i<4; i++) DrawWindow(&wins[win_order[i]]);
    
    int dX=(Screen::w-400)/2; int dY=Screen::h-80;
    Screen::RectAlpha(dX, dY, 400, 64, 0xFFFFFF, 100);
    DrawIcon(dX+20, dY+16, 0); DrawIcon(dX+80, dY+16, 1); DrawIcon(dX+140, dY+16, 2); DrawIcon(dX+200, dY+16, 3); DrawIcon(dX+270, dY+16, 4);
    if(show_launchpad) { Screen::RectAlpha(0, 0, Screen::w, Screen::h, 0, 150); int cx=Screen::w/2-200; int cy=Screen::h/2-100; DrawIcon(cx,cy,0); DrawText(cx,cy+40,"Finder",-1); DrawIcon(cx+100,cy,1); DrawText(cx+100,cy+40,"Terminal",-1); DrawIcon(cx+200,cy,2); DrawText(cx+200,cy+40,"Notes",-1); DrawIcon(cx+300,cy,3); DrawText(cx+300,cy+40,"Settings",-1); DrawIcon(cx,cy+100,4); DrawText(cx,cy+140,"Files",-1); }
    DrawCursor(mx, my); Screen::Swap();
}

void BringToFront(int idx) {
    int pos = -1;
    for(int i=0; i<4; i++) if(win_order[i] == idx) pos=i;
    for(int i=pos; i<3; i++) win_order[i] = win_order[i+1];
    win_order[3] = idx;
}

void OS_Update() {
   MouseUpdate(); KeyboardUpdate();
   if(m_left) {
       if(!drag_win && !m_prev_left) {
           if(show_launchpad) { show_launchpad=false; }
           else {
               bool hit = false;
               for(int k=3; k>=0; k--) {
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
                       if(mx>dX+20 && mx<dX+52) { wins[3].active=!wins[3].active; if(wins[3].active) BringToFront(3); focused=&wins[3]; }
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

extern "C" void kernel_main(uint32_t magic, multiboot_info* mb) {
     (void)magic; if(mb->fb_addr==0)return;
     Screen::Init(mb); OS_Init();
     while(true) { OS_Update(); OS_Draw(); }
}