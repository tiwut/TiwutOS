#ifndef IO_H
#define IO_H
#include "utils.h"

class Port {
public:
    static uint8_t inb(uint16_t p);
    static void outb(uint16_t p, uint8_t d);
};

class Mouse {
public:
    static int x, y;
    static bool left, right;
    static void Init();
    static void Update();
};

class Keyboard {
public:
    static char Read();
    static uint8_t Scan();
};
#endif
