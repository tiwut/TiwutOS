#include "kernel.h"

#define VGA_ADDR 0xB8000
#define SCREEN_W 80
#define ATA_PRIMARY 0x1F0

int cursor_x = 0;
int cursor_y = 0;
unsigned short* term_buf = (unsigned short*)VGA_ADDR;

uint8_t inb(uint16_t port) { uint8_t r; __asm__("in %%dx, %%al":"=a"(r):"d"(port)); return r; }
void outb(uint16_t port, uint8_t data) { __asm__("out %%al, %%dx"::"a"(data),"d"(port)); }
void insw(uint16_t p, void* a, uint32_t c) { __asm__ volatile("rep insw":"+D"(a),"+c"(c):"d"(p):"memory"); }
void outsw(uint16_t p, void* a, uint32_t c) { __asm__ volatile("rep outsw":"+S"(a),"+c"(c):"d"(p):"memory"); }

void set_cursor(int x, int y) {
    uint16_t pos = y * SCREEN_W + x;
    outb(0x3D4, 0x0F); outb(0x3D5, (uint8_t)(pos&0xFF));
    outb(0x3D4, 0x0E); outb(0x3D5, (uint8_t)((pos>>8)&0xFF));
}

void clear_screen() {
    for(int i=0; i<80*25; i++) term_buf[i] = 0x0F20;
    cursor_x=0; cursor_y=0; set_cursor(0,0);
}

void print_c(char* str, uint8_t fg, uint8_t bg) {
    int i = 0;
    while(str[i]) {
        if(str[i] == '\n') { cursor_x = 0; cursor_y++; }
        else {
            term_buf[cursor_y * SCREEN_W + cursor_x] = str[i] | ((bg << 12) | (fg << 8));
            cursor_x++;
        }
        if(cursor_x >= 80) { cursor_x = 0; cursor_y++; }
        if(cursor_y >= 25) {
             for(int y=1; y<25; y++) 
                 for(int x=0; x<80; x++) 
                     term_buf[(y-1)*80+x] = term_buf[y*80+x];
             for(int x=0; x<80; x++) term_buf[24*80+x] = 0x0F20;
             cursor_y=24;
        }
        i++;
    }
    set_cursor(cursor_x, cursor_y);
}

void print(char* str) { print_c(str, WHITE, BLACK); }

void ata_wait() { while(inb(ATA_PRIMARY+7)&0x80); while(!(inb(ATA_PRIMARY+7)&0x08)); }

void ata_read_sector(uint32_t lba, uint8_t* buffer) {
    outb(ATA_PRIMARY+6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY+2, 1); outb(ATA_PRIMARY+3, (uint8_t)lba);
    outb(ATA_PRIMARY+4, (uint8_t)(lba>>8)); outb(ATA_PRIMARY+5, (uint8_t)(lba>>16));
    outb(ATA_PRIMARY+7, 0x20); ata_wait(); insw(ATA_PRIMARY, buffer, 256);
}

void ata_write_sector(uint32_t lba, uint8_t* buffer) {
    outb(ATA_PRIMARY+6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY+2, 1); outb(ATA_PRIMARY+3, (uint8_t)lba);
    outb(ATA_PRIMARY+4, (uint8_t)(lba>>8)); outb(ATA_PRIMARY+5, (uint8_t)(lba>>16));
    outb(ATA_PRIMARY+7, 0x30); ata_wait(); outsw(ATA_PRIMARY, buffer, 256);
}

void print_dec(int n) {
    if(n==0){print("0");return;}
    char b[10]; int i=0; while(n>0){b[i++]=(n%10)+'0';n/=10;}
    for(int j=0;j<i/2;j++){char t=b[j];b[j]=b[i-1-j];b[i-1-j]=t;} b[i]=0; print(b);
}

void get_rtc_time() {
    outb(0x70, 0x09); uint8_t y = inb(0x71);
    outb(0x70, 0x08); uint8_t m = inb(0x71);
    outb(0x70, 0x07); uint8_t d = inb(0x71);
    outb(0x70, 0x04); uint8_t h = inb(0x71);
    outb(0x70, 0x02); uint8_t mm = inb(0x71);
    print("20"); print_dec((y/16)*10+(y%16)); print("-"); print_dec((m/16)*10+(m%16)); print("-"); print_dec((d/16)*10+(d%16));
    print(" "); print_dec((h/16)*10+(h%16)); print(":"); print_dec((mm/16)*10+(mm%16)); print("\n");
}
