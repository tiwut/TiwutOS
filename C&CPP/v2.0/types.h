#ifndef TYPES_H
#define TYPES_H

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

class IO {
public:
    static inline void outb(uint16_t port, uint8_t val) {
        __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
    }
    static inline uint8_t inb(uint16_t port) {
        uint8_t ret;
        __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
        return ret;
    }
};

static inline void fast_memcpy(void* dst, void* src, int count) {
    int dwords = count / 4;
    __asm__ volatile("cld; rep movsl" : : "S"(src), "D"(dst), "c"(dwords));
}

#endif
