#pragma once
#include "../api/types.h"

class Utils {
public:
  static int strlen(const char *s) {
    int i = 0;
    while (s[i])
      i++;
    return i;
  }
  static void strcpy(char *d, const char *s) {
    while (*s)
      *d++ = *s++;
    *d = 0;
  }
  static int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
      s1++;
      s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
  }
  static int strncmp(const char *s1, const char *s2, int n) {
    while (n && *s1 && (*s1 == *s2)) {
      s1++;
      s2++;
      n--;
    }
    if (n == 0)
      return 0;
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
  }
  static void fill32(void *dest, uint32_t val, int count) {
    uint32_t *d = (uint32_t *)dest;
    while (count--)
      *d++ = val;
  }
  static void itoa(int n, char *buf) {
    if (n == 0) {
      buf[0] = '0';
      buf[1] = 0;
      return;
    }
    char t[16];
    int i = 0;
    while (n > 0) {
      t[i++] = (n % 10) + '0';
      n /= 10;
    }
    int j = 0;
    while (i > 0)
      buf[j++] = t[--i];
    buf[j] = 0;
  }
};

class Port {
public:
  static inline void outb(uint16_t p, uint8_t d) {
    __asm__ volatile("outb %0, %1" ::"a"(d), "Nd"(p));
  }
  static inline uint8_t inb(uint16_t p) {
    uint8_t r;
    __asm__ volatile("inb %1, %0" : "=a"(r) : "Nd"(p));
    return r;
  }
  static inline void out16(uint16_t p, uint16_t d) {
    __asm__ volatile("outw %0, %1" ::"a"(d), "Nd"(p));
  }
  static inline uint16_t in16(uint16_t p) {
    uint16_t r;
    __asm__ volatile("inw %1, %0" : "=a"(r) : "Nd"(p));
    return r;
  }
  static inline void out32(uint16_t p, uint32_t d) {
    __asm__ volatile("outl %0, %1" ::"a"(d), "Nd"(p));
  }
  static inline uint32_t in32(uint16_t p) {
    uint32_t r;
    __asm__ volatile("inl %1, %0" : "=a"(r) : "Nd"(p));
    return r;
  }
  static inline void WaitWrite() {
    int t = 10000;
    while ((inb(0x64) & 2) && t--)
      ;
  }
  static inline void WaitRead() {
    int t = 10000;
    while (!(inb(0x64) & 1) && t--)
      ;
  }
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
  static char Read() {
    while (HasReceived() == 0) {
    }
    return Port::inb(0x3F8);
  }
  static int IsTransmitEmpty() { return Port::inb(0x3F8 + 5) & 0x20; }
  static void WriteChar(char a) {
    while (IsTransmitEmpty() == 0) {
    }
    Port::outb(0x3F8, a);
  }
  static void Write(const char *str) {
    while (*str)
      WriteChar(*str++);
  }
};

struct File {
  char name[32];
  char content[512];
  int size;
  bool used;
};
File fs_files[16];

void FS_Init() {
  for (int i = 0; i < 16; i++) {
    fs_files[i].used = false;
  }
  Utils::strcpy(fs_files[0].name, "readme.txt");
  Utils::strcpy(fs_files[0].content,
                "Welcome to TiwutOS!\nThis is a virtual FS in RAM.");
  fs_files[0].size = Utils::strlen(fs_files[0].content);
  fs_files[0].used = true;
  Utils::strcpy(fs_files[1].name, "todo.list");
  Utils::strcpy(fs_files[1].content, "- Buy milk\n- Write OS kernel\n- Sleep");
  fs_files[1].size = Utils::strlen(fs_files[1].content);
  fs_files[1].used = true;

  Utils::strcpy(fs_files[2].name, "/OS/system.cfg");
  Utils::strcpy(fs_files[2].content, "ENCRYPTED_SYSTEM_DATA");
  fs_files[2].size = 21;
  fs_files[2].used = true;
  Utils::strcpy(fs_files[3].name, "/App/Settings/main.nx");
  Utils::strcpy(fs_files[3].content, "sys.auth()");
  fs_files[3].size = 10;
  fs_files[3].used = true;
}

int FS_Find(const char *name) {
  for (int i = 0; i < 16; i++) {
    if (fs_files[i].used && Utils::strcmp(fs_files[i].name, name) == 0) {
      if (Utils::strncmp(fs_files[i].name, "/OS", 3) == 0 ||
          Utils::strncmp(fs_files[i].name, "/App", 4) == 0)
        return -1;
      return i;
    }
  }
  return -1;
}

void FS_Remove(const char *name) {
  int idx = FS_Find(name);
  if (idx != -1)
    fs_files[idx].used = false;
}

void FS_Create(const char *name) {
  if (FS_Find(name) != -1)
    return;
  for (int i = 0; i < 16; i++) {
    if (!fs_files[i].used) {
      Utils::strcpy(fs_files[i].name, name);
      fs_files[i].content[0] = 0;
      fs_files[i].size = 0;
      fs_files[i].used = true;
      return;
    }
  }
}
