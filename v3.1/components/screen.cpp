class Screen {
public:
    static uint32_t* front;
    static uint32_t* back;
    static int w, h, pitch, size;
    static uint8_t* bg_image;
    static uint8_t* logo_image;

    static void Init(multiboot_info* mb) {
        front = (uint32_t*)(uint64_t)mb->fb_addr;
        w = mb->fb_width; h = mb->fb_height;
        pitch = mb->fb_pitch; 
        size = (pitch/4) * h;
        back = (uint32_t*)malloc(pitch * h); 
    }

    static void Swap() {
        uint8_t* f_ptr = (uint8_t*)front;
        uint32_t* b_ptr = back;
        for(int y=0; y<h; y++) {
            rep_movsd((uint32_t*)f_ptr, b_ptr, w);
            b_ptr += w;
            f_ptr += pitch;
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

    static bool ui_blur;
    static bool ui_animations;
    static int ui_corner_radius;
    static bool ui_borders;

    static void RoundedRectAlpha(int x, int y, int rw, int rh, int radius, uint32_t c, uint8_t a, bool has_border=false, uint32_t border_col=0x000000) {
        if(!ui_borders) has_border = false;
        if(x+rw > w) rw = w-x; 
        if(y+rh > h) rh = h-y; 
        if(rw<=0 || rh<=0) return;
        int r2 = radius*radius;
        int r2_inner = (radius-1)*(radius-1);
        for(int j=y; j<y+rh; j++) {
            for(int i=x; i<x+rw; i++) {
                int dx = 0, dy = 0;
                if(i < x+radius) dx = (x+radius)-i;
                else if(i > x+rw-radius) dx = i-(x+rw-radius);
                if(j < y+radius) dy = (y+radius)-j;
                else if(j > y+rh-radius) dy = j-(y+rh-radius);
                
                int dist_sq = dx*dx + dy*dy;
                if(dist_sq <= r2) {
                    bool border = has_border && (dist_sq >= r2_inner || i==x || i==x+rw-1 || j==y || j==y+rh-1);
                    if(border) {
                        PutAlpha(i, j, border_col, a < 200 ? a + 55 : 255);
                    } else {
                        PutAlpha(i, j, c, a);
                    }
                }
            }
        }
    }

    static void SetResolutionBGA(int new_w, int new_h) {
        Port::out16(0x01CE, 4); Port::out16(0x01CF, 0); 
        Port::out16(0x01CE, 1); Port::out16(0x01CF, new_w); 
        Port::out16(0x01CE, 2); Port::out16(0x01CF, new_h); 
        Port::out16(0x01CE, 3); Port::out16(0x01CF, 32); 
        Port::out16(0x01CE, 4); Port::out16(0x01CF, 0x41); 
        w = new_w; h = new_h; pitch = w * 4; size = w * h;
    }

    static void BlurRegion(int x, int y, int rw, int rh) {
        if(x+rw > w) rw = w-x; 
        if(y+rh > h) rh = h-y;
        if(rw<=0 || rh<=0) return;
        for(int j=y; j<y+rh-1; j+=2) {
            for(int i=x; i<x+rw-1; i+=2) {
                int r=0, g=0, b=0, count=0;
                for(int dy=-2; dy<=2; dy++) {
                    for(int dx=-2; dx<=2; dx++) {
                        int nx=i+dx; int ny=j+dy;
                        if(nx>=0 && nx<w && ny>=0 && ny<h) {
                            uint32_t c = back[ny*w + nx];
                            r += (c >> 16) & 0xFF; g += (c >> 8) & 0xFF; b += c & 0xFF;
                            count++;
                        }
                    }
                }
                uint32_t bc = ((r/count)<<16) | ((g/count)<<8) | (b/count);
                back[j*w + i] = bc; back[j*w + i + 1] = bc;
                back[(j+1)*w + i] = bc; back[(j+1)*w + i + 1] = bc;
            }
        }
    }

    static void DrawLogoImage(int dx, int dy, int dw, int dh) {
        if(logo_image && logo_image[0] == 'B' && logo_image[1] == 'M') {
            int offset = *(uint32_t*)(logo_image + 10);
            int bw = *(int*)(logo_image + 18);
            int bh = *(int*)(logo_image + 22);
            uint8_t* pixels = logo_image + offset;
            for(int y=0; y<dh; y++) {
                int src_y = bh - 1 - ((y * bh)/dh);
                uint8_t* src_row = pixels + (src_y * bw * 4);
                for(int x=0; x<dw; x++) {
                    int src_x = ((x * bw)/dw) * 4;
                    uint8_t pb = src_row[src_x], pg = src_row[src_x+1], pr = src_row[src_x+2], pa = src_row[src_x+3];
                    if(pa > 0) {
                        uint32_t col = (pr << 16) | (pg << 8) | pb;
                        PutAlpha(dx+x, dy+y, col, pa);
                    }
                }
            }
        }
    }
    
    static bool dark_mode;
    static int wallpaper_id;
    
    static void EraseCursorFront(int x, int y) {
        uint32_t* f_ptr = (uint32_t*)front;
        for(int r=0; r<16; r++) { 
            for(int c=0; c<12; c++) { 
                if(y+r>=0 && y+r<h && x+c>=0 && x+c<w) {
                    uint32_t bg = back[(y+r)*w + (x+c)];
                    uint32_t* dest = &f_ptr[(y+r)*(pitch/4) + (x+c)];
                    asm volatile("mov %1, %0" : "=m"(*dest) : "r"(bg) : "memory");
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
        uint32_t* f_ptr = (uint32_t*)front;
        for(int r=0; r<16; r++) { 
            for(int c=0; c<12; c++) { 
                uint8_t p = cursor[r][c]; 
                if(p!=0 && y+r>=0 && y+r<h && x+c>=0 && x+c<w) {
                    uint32_t color = (p==1) ? 0x000000 : 0xFFFFFF;
                    uint32_t* dest = &f_ptr[(y+r)*(pitch/4) + (x+c)];
                    asm volatile("mov %1, %0" : "=m"(*dest) : "r"(color) : "memory");
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
                    int src_y = bh - 1 - ((y * bh) / h);
                    if(src_y < 0) src_y = 0;
                    uint8_t* src_row = pixels + (src_y * row_bytes);
                    for(int x=0; x<w; x++) {
                        int src_x = ((x * bw) / w) * (bpp/8);
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
uint8_t* Screen::bg_image=0; uint8_t* Screen::logo_image=0;
bool Screen::dark_mode=false; int Screen::wallpaper_id=0;
bool Screen::ui_blur=true; bool Screen::ui_animations=true; int Screen::ui_corner_radius=10; bool Screen::ui_borders=true;

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
