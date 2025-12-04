#include "types.h"

const char kbd_map[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' '
};

class Input {
public:
    static char GetKey() {
        if(IO::inb(0x64)&1) {
            if(!(IO::inb(0x64)&0x20)) {
                uint8_t s = IO::inb(0x60);
                if(!(s&0x80) && s<128) return kbd_map[s];
            }
        }
        return 0;
    }
};

class Mouse {
public:
    static int x, y;
    static bool left;
    static uint8_t cycle;
    static int8_t pkt[3];

    static void Init() {
        IO::outb(0x64, 0xA8); IO::outb(0x64, 0x20);
        uint8_t s = (IO::inb(0x60)|2);
        IO::outb(0x64, 0x60); IO::outb(0x60, s);
        IO::outb(0x64, 0xD4); IO::outb(0x60, 0xF4); IO::inb(0x60);
        x=512; y=384;
    }

    static void Update() {
        if((IO::inb(0x64)&1) && (IO::inb(0x64)&0x20)) {
            uint8_t b = IO::inb(0x60);
            if(cycle==0 && (b&8)) { pkt[0]=b; cycle++; }
            else if(cycle==1) { pkt[1]=b; cycle++; }
            else if(cycle==2) {
                pkt[2]=b; cycle=0;
                left = (pkt[0] & 1);
                x += pkt[1]; y -= pkt[2];
                if(x<0)x=0; if(x>1020)x=1020;
                if(y<0)y=0; if(y>760)y=760;
            }
        }
    }
};
int Mouse::x=0; int Mouse::y=0; bool Mouse::left=false;
uint8_t Mouse::cycle=0; int8_t Mouse::pkt[3];