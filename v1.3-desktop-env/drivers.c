#include "kernel.h"

uint32_t* video_mem = 0;
int screen_w = 0;
int screen_h = 0;
int pitch = 0;

uint8_t inb(uint16_t port) { uint8_t r; __asm__("in %%dx, %%al":"=a"(r):"d"(port)); return r; }
void outb(uint16_t port, uint8_t data) { __asm__("out %%al, %%dx"::"a"(data),"d"(port)); }

void init_graphics(struct multiboot_info* mb) {
    video_mem = (uint32_t*)(uint32_t)mb->framebuffer_addr;
    screen_w = mb->framebuffer_width;
    screen_h = mb->framebuffer_height;
    pitch = mb->framebuffer_pitch;
}

void put_pixel(int x, int y, uint32_t color) {
    if(x>=0 && x<screen_w && y>=0 && y<screen_h) {
        // Pitch is in bytes, so we divide by 4 for 32-bit pixels
        video_mem[y * (pitch/4) + x] = color;
    }
}

void draw_rect(int x, int y, int w, int h, uint32_t color) {
    for(int j=y; j<y+h; j++)
        for(int i=x; i<x+w; i++)
            put_pixel(i, j, color);
}

// -- MOUSE (PS/2) --
void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if(type==0) { while(timeout--) { if((inb(0x64) & 1)==1) return; } }
    else { while(timeout--) { if((inb(0x64) & 2)==0) return; } }
}

void mouse_write(uint8_t write) {
    mouse_wait(1); outb(0x64, 0xD4);
    mouse_wait(1); outb(0x60, write);
}

uint8_t mouse_read() {
    mouse_wait(0); return inb(0x60);
}

void init_mouse() {
    uint8_t status;
    mouse_wait(1); outb(0x64, 0xA8);
    mouse_wait(1); outb(0x64, 0x20);
    mouse_wait(0); status = (inb(0x60) | 2);
    mouse_wait(1); outb(0x64, 0x60);
    mouse_wait(1); outb(0x60, status);
    mouse_write(0xF6); mouse_read();
    mouse_write(0xF4); mouse_read();
}
