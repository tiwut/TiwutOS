#ifndef KEYBOARD_H
#define KEYBOARD_H

class Keyboard {
public:
    static char ReadKey();
    static void ReadString(char* buffer, int max_len);
};

#endif
