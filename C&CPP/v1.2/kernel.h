#ifndef KERNEL_H
#define KERNEL_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
enum Colors { BLACK=0, BLUE=1, GREEN=2, CYAN=3, RED=4, MAGENTA=5, BROWN=6, LGREY=7, DGREY=8, LBLUE=9, LGREEN=10, LCYAN=11, LRED=12, LMAGENTA=13, YELLOW=14, WHITE=15 };
uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t data);
void insw(uint16_t port, void* addr, uint32_t cnt);
void outsw(uint16_t port, void* addr, uint32_t cnt);
int strcmp(char* s1, char* s2);
int strncmp(char* s1, char* s2, int n);
void strcpy(char* dest, char* src);
int strlen(char* s);
void *memset(void *s, int c, int n);
void clear_screen();
void print(char* str);
void print_c(char* str, uint8_t fg, uint8_t bg);
void set_cursor(int x, int y);
void ata_read_sector(uint32_t lba, uint8_t* buffer);
void ata_write_sector(uint32_t lba, uint8_t* buffer);
void get_rtc_time();
extern int cursor_x;
extern int cursor_y;

#endif
