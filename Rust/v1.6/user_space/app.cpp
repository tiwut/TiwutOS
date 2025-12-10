#include <stdint.h>

extern "C" {
    void os_print(const char* msg, uint32_t x, uint32_t y, uint8_t color);
    void os_cls(uint8_t color);
}
class Color {
public:
    static const uint8_t Black = 0;
    static const uint8_t Blue = 1;
    static const uint8_t Green = 2;
    static const uint8_t Red = 4;
    static const uint8_t White = 15;
    
    static uint8_t Mix(uint8_t fg, uint8_t bg) {
        return fg | (bg << 4);
    }
};
class Window {
    uint32_t x, y, w, h;
    uint8_t color;
public:
    Window(uint32_t _x, uint32_t _y, uint32_t _w, uint32_t _h) : x(_x), y(_y), w(_w), h(_h) {
        color = Color::Mix(Color::Black, Color::White);
    }

    void Draw() {
        for(uint32_t i = 0; i < h; i++) {
             os_print("|", x, y + i, color);
             os_print("|", x + w, y + i, color);
        }
        os_print("----------------", x, y, color);
        os_print("----------------", x, y + h, color);
    }

    void AddText(const char* text, uint32_t offsetX, uint32_t offsetY) {
        os_print(text, x + offsetX + 1, y + offsetY + 1, Color::Mix(Color::Black, Color::White));
    }
};
extern "C" void user_main() {
    os_cls(Color::Mix(Color::White, Color::Black));
    os_print("TIWUT OS - C++ USER SPACE", 25, 1, Color::Mix(Color::Green, Color::Black));
    Window win(10, 5, 40, 10);
    win.Draw();
    win.AddText("Hello from C++ Class!", 2, 2);
    win.AddText("Rust is the Kernel.", 2, 4);
    win.AddText("C++ is the App.", 2, 5);
    Window errorWin(40, 15, 30, 5);
    errorWin.Draw();
    errorWin.AddText("Status: Running...", 1, 1);
}
