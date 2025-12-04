#ifndef OS_H
#define OS_H

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

struct AppBuffer { char lines[10][40]; int row, col; };
struct Window { bool active; int x, y, w, h; char title[16]; AppBuffer buffer; };

void outb(uint16_t p, uint8_t d);
uint8_t inb(uint16_t p);
int strlen(const char* s);
void strcpy(char* d, const char* s);
void itoa(int n, char* b);

static inline void fast_memcpy(void* dst, void* src, int count) {
    __asm__ volatile("cld; rep movsl" : : "S"(src), "D"(dst), "c"(count));
}

void gfx_init(multiboot_info* mb);
void gfx_clear(uint32_t color);
void gfx_swap();
void draw_rect(int x, int y, int w, int h, uint32_t color);
void draw_text(int x, int y, const char* s, uint32_t color);
void mouse_init();
void mouse_update();
char kbd_read();
void gui_init();
void gui_update();
void gui_draw();
void app_terminal_input(AppBuffer* term, char c);

extern Window win_term; 

#endif