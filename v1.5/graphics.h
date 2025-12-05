#ifndef GRAPHICS_H
#define GRAPHICS_H
#include "utils.h"

struct multiboot_info {
    uint32_t flags, mem_l, mem_u, boot_dev, cmdline, mods_c, mods_a;
    uint32_t syms[4], mmap_l, mmap_a, drives_l, drives_a, config;
    uint32_t loader, apm, vbe_ctrl, vbe_mode_inf;
    uint16_t vbe_mode, vbe_seg, vbe_off, vbe_len;
    uint64_t fb_addr; uint32_t fb_pitch, fb_width, fb_height;
    uint8_t fb_bpp, fb_type;
} __attribute__((packed));

class Graphics {
public:
    static void Init(multiboot_info* mb);
    static void Put(int x, int y, uint32_t color);
    static void Rect(int x, int y, int w, int h, uint32_t color);
    static void Box(int x, int y, int w, int h, uint32_t bg, uint32_t border);
    static void Char(int x, int y, char c, uint32_t color);
    static void Text(int x, int y, const char* s, uint32_t color);
};
#endif
