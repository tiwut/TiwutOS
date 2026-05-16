#pragma once

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

struct multiboot_module { uint32_t mod_start, mod_end, string, reserved; };

enum AppType {
  APP_TERM,
  APP_EDITOR,
  APP_SETTINGS,
  APP_BROWSER,
  APP_TASKMGR,
  APP_FILEMGR,
  APP_DISKUTIL
};

struct Window {
  bool active;
  bool minimized;
  int x, y, w, h;
  char title[32];
  AppType type;
  char buffer[2048];
  int cursor;
  float anim;
};

extern "C" void cpp_main(multiboot_info* mb);
