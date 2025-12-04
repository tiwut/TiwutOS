#include "io.h"

// --- PORTS ---
uint8_t Port::inb(uint16_t p) { uint8_t r; __asm__ volatile("inb %1, %0" : "=a"(r) : "Nd"(p)); return r; }
void Port::outb(uint16_t p, uint8_t d) { __asm__ volatile("outb %0, %1" : : "a"(d), "Nd"(p)); }

// --- MOUSE ---
int Mouse::x = 400; int Mouse::y = 300;
bool Mouse::left = false; bool Mouse::right = false;
uint8_t m_cycle=0; int8_t m_byte[3];

void Mouse::Init() {
    uint32_t t=10000;
    Port::outb(0x64, 0xA8); Port::outb(0x64, 0x20);
    while(t--) if((Port::inb(0x64)&1)==1) break;
    uint8_t s = (Port::inb(0x60)|2);
    Port::outb(0x64, 0x60); Port::outb(0x60, s);
    Port::outb(0x64, 0xD4); Port::outb(0x60, 0xF4); Port::inb(0x60);
}

void Mouse::Update() {
    if(Port::inb(0x64)&1) {
        uint8_t b = Port::inb(0x60);
        if(m_cycle==0 && (b&8)) { m_byte[0]=b; m_cycle++; }
        else if(m_cycle==1) { m_byte[1]=b; m_cycle++; }
        else if(m_cycle==2) {
            m_byte[2]=b; m_cycle=0;
            left = m_byte[0]&1; right = m_byte[0]&2;
            x += m_byte[1]; y -= m_byte[2];
            if(x<0)x=0; if(x>630)x=630; if(y<0)y=0; if(y>470)y=470;
        }
    }
}

// --- KEYBOARD ---
char Keyboard::Read() {
    if(Port::inb(0x64)&1) {
        uint8_t s = Port::inb(0x60);
        if(!(s&0x80)) {
            const char* m = "..1234567890-=..qwertyuiop[]\n.asdfghjkl;'`.\\zxcvbnm,./";
            if(s<58) return m[s]; if(s==57) return ' '; if(s==14) return '\b';
        }
    }
    return 0;
}
uint8_t Keyboard::Scan() { if(Port::inb(0x64)&1) return Port::inb(0x60); return 0; }
