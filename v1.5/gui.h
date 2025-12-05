#ifndef GUI_H
#define GUI_H
#include "utils.h"

struct Window {
    bool active;
    char title[32];
    int x, y, w, h;
    char buffer[15][40];
    int widget_count;
    char widgets[10][32];
};

class GUI {
public:
    static Window wins[5];
    static int focus;
    static void Init();
    static void Update();
    static void Render();
    static void PythonParse(int win_id, char* cmd);
};
#endif
