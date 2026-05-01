#include "os.h"

extern int mx, my;
extern bool m_left;

Window win_term;
bool start_open = false;
bool dragging = false;
int drag_off_x, drag_off_y;

void gui_init() {
    win_term.active = true;
    win_term.x = 100; win_term.y = 100; win_term.w = 400; win_term.h = 300;
    strcpy(win_term.title, "Terminal");
    strcpy(win_term.buffer.lines[0], "Welcome to TiwutOS");
    win_term.buffer.row = 1; win_term.buffer.col = 0;
}

void gui_update() {
    if(m_left && my > 740 && mx < 50) start_open = !start_open;

    if(m_left) {
        if(!dragging && mx >= win_term.x && mx <= win_term.x+win_term.w && my >= win_term.y && my <= win_term.y+30) {
            dragging = true;
            drag_off_x = mx - win_term.x;
            drag_off_y = my - win_term.y;
        }
        if(dragging) {
            win_term.x = mx - drag_off_x;
            win_term.y = my - drag_off_y;
        }
    } else {
        dragging = false;
    }
}

void gui_draw() {
    draw_rect(0, 0, 1024, 768, 0x003A6EA5); 

    draw_rect(0, 738, 1024, 30, 0xC0C0C0);
    draw_rect(2, 740, 46, 26, 0x008000); 
    draw_text(6, 748, "START", 0xFFFFFF);

    if(start_open) {
        draw_rect(2, 600, 150, 130, 0xFFFFFF);
        draw_rect(2, 600, 150, 130, 0x808080);
        draw_text(10, 610, "1. Terminal", 0x000000);
        draw_text(10, 630, "2. Calculator", 0x000000);
        draw_text(10, 650, "3. Shutdown", 0x000000);
    }

    if(win_term.active) {
        draw_rect(win_term.x, win_term.y, win_term.w, win_term.h, 0xFFFFFF);
        draw_rect(win_term.x, win_term.y, win_term.w, 30, 0x000080);
        draw_text(win_term.x+10, win_term.y+10, win_term.title, 0xFFFFFF);
        draw_rect(win_term.x+win_term.w-30, win_term.y+5, 20, 20, 0xFF0000);

        for(int i=0; i<10; i++) {
            draw_text(win_term.x+10, win_term.y+40+(i*12), win_term.buffer.lines[i], 0x000000);
        }
    }

    draw_rect(mx, my, 10, 10, 0xFFFFFF);
    draw_rect(mx+2, my+2, 6, 6, 0x000000);
}