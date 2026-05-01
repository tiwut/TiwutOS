#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>

class Screen {
private:
    static uint16_t* video_memory;
    static int cursor_x;
    static int cursor_y;
    static uint8_t color;

public:
    static void SetColor(uint8_t fg, uint8_t bg);
    static void MoveCursor();
    static void Scroll();
    static void Clear(uint8_t color_code);
    
    static void Print(const char* str);
    static void PrintChar(char c);
    static void PrintInt(int n);
    static void Backspace();

    static void DrawRect(int x, int y, int w, int h, uint8_t c);
    static void DrawBox(int x, int y, int w, int h, const char* title);
    
    static int GetX() { return cursor_x; }
    static int GetY() { return cursor_y; }
    static void SetCursorPos(int x, int y);
};

#endif