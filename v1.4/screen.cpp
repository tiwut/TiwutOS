#include "screen.h"
#include "ports.h"
#include "utils.h"

uint16_t* Screen::video_memory = (uint16_t*)0xB8000;
int Screen::cursor_x = 0;
int Screen::cursor_y = 0;
uint8_t Screen::color = 0x07;

void Screen::SetColor(uint8_t fg, uint8_t bg) {
    color = (bg << 4) | fg;
}

void Screen::MoveCursor() {
    uint16_t pos = cursor_y * 80 + cursor_x;
    Port::outb(0x3D4, 0x0F); Port::outb(0x3D5, (uint8_t)(pos & 0xFF));
    Port::outb(0x3D4, 0x0E); Port::outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void Screen::SetCursorPos(int x, int y) {
    cursor_x = x;
    cursor_y = y;
    MoveCursor();
}

void Screen::Scroll() {
    if (cursor_y >= 23) {
        for (int y = 5; y < 23; y++) {
            for (int x = 2; x < 78; x++) {
                video_memory[(y-1) * 80 + x] = video_memory[y * 80 + x];
            }
        }
        for (int x = 2; x < 78; x++) {
            video_memory[22 * 80 + x] = (color << 8) | ' ';
        }
        cursor_y = 22;
    }
}

void Screen::Clear(uint8_t c) {
    uint16_t val = (c << 8) | ' ';
    for (int i = 0; i < 80 * 25; i++) video_memory[i] = val;
    cursor_x = 0; cursor_y = 0;
    MoveCursor();
}

void Screen::PrintChar(char c) {
    if (c == '\n') {
        cursor_x = 2;
        cursor_y++;
    } else {
        video_memory[cursor_y * 80 + cursor_x] = (color << 8) | c;
        cursor_x++;
    }
    
    if (cursor_x >= 78) { 
        cursor_x = 2; 
        cursor_y++; 
    }
    Scroll();
    MoveCursor();
}

void Screen::Print(const char* str) {
    for (int i = 0; str[i] != 0; i++) PrintChar(str[i]);
}

void Screen::Backspace() {
    if (cursor_x > 2) {
        cursor_x--;
        video_memory[cursor_y * 80 + cursor_x] = (color << 8) | ' ';
        MoveCursor();
    }
}

void Screen::PrintInt(int n) {
    char buf[16];
    Utils::int_to_string(n, buf);
    Print(buf);
}


void Screen::DrawRect(int x, int y, int w, int h, uint8_t c) {
    uint16_t val = (c << 8) | ' ';
    for (int j = y; j < y + h; j++) {
        for (int i = x; i < x + w; i++) {
            video_memory[j * 80 + i] = val;
        }
    }
}

void Screen::DrawBox(int x, int y, int w, int h, const char* title) {
    uint8_t border_col = 0x1F;
    uint16_t corner_tl = (border_col << 8) | 201; // ╔
    uint16_t corner_tr = (border_col << 8) | 187; // ╗
    uint16_t corner_bl = (border_col << 8) | 200; // ╚
    uint16_t corner_br = (border_col << 8) | 188; // ╝
    uint16_t line_h    = (border_col << 8) | 205; // ═
    uint16_t line_v    = (border_col << 8) | 186; // ║

    video_memory[y * 80 + x] = corner_tl;
    video_memory[y * 80 + (x + w)] = corner_tr;
    video_memory[(y + h) * 80 + x] = corner_bl;
    video_memory[(y + h) * 80 + (x + w)] = corner_br;

    for (int i = x + 1; i < x + w; i++) {
        video_memory[y * 80 + i] = line_h;
        video_memory[(y + h) * 80 + i] = line_h;
    }
    for (int i = y + 1; i < y + h; i++) {
        video_memory[i * 80 + x] = line_v;
        video_memory[i * 80 + (x + w)] = line_v;
    }

    uint16_t fill = (0x0F << 8) | ' ';
    for (int j = y + 1; j < y + h; j++) {
        for (int i = x + 1; i < x + w; i++) {
            video_memory[j * 80 + i] = fill;
        }
    }

    int t_len = Utils::strlen(title);
    int t_x = x + (w / 2) - (t_len / 2);
    for (int i = 0; i < t_len; i++) {
        video_memory[y * 80 + t_x + i] = (0x1E << 8) | title[i];
    }
}