#include "keyboard.h"
#include "ports.h"
#include "screen.h"

char kbd_map[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' '
};

char Keyboard::ReadKey() {
    if (Port::inb(0x64) & 1) {
        uint8_t scan = Port::inb(0x60);
        if (!(scan & 0x80)) { 
            return kbd_map[scan];
        }
    }
    return 0;
}

void Keyboard::ReadString(char* buffer, int max_len) {
    int idx = 0;
    int start_x = Screen::GetX();

    while (true) {
        char c = ReadKey();
        if (c) {
            if (c == '\n') {
                Screen::PrintChar('\n');
                buffer[idx] = 0;
                return;
            } 
            else if (c == '\b') {
                if (idx > 0) {
                    idx--;
                    if (Screen::GetX() > start_x) {
                        Screen::Backspace();
                    }
                }
            } 
            else if (idx < max_len - 1) {
                buffer[idx++] = c;
                Screen::PrintChar(c);
            }
        }
        for(int i=0; i<10000; i++); 
    }
}