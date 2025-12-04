#include "kernel.h"

int str_contains(char* str, char* sub) {
    int len = strlen(sub);
    for(int i=0; str[i]; i++) {
        if(strncmp(str+i, sub, len) == 0) return 1;
    }
    return 0;
}

void extract_string(char* line, char* out) {
    int i=0, j=0;
    while(line[i] && line[i]!='"') i++;
    if(line[i]=='"') {
        i++;
        while(line[i] && line[i]!='"') out[j++] = line[i++];
    }
    out[j] = 0;
}

void extract_args(char* line, char* out) {
    int i=0, j=0;
    while(line[i] && line[i]!='(') i++;
    if(line[i]=='(') {
        i++;
        while(line[i] && line[i]!=')') out[j++] = line[i++];
    }
    out[j] = 0;
}

void exec_python(char* code) {
    print_c(" [Python Interpreter v3.11 (Simulated)]\n", YELLOW, BLACK);
    
    char line[64];
    int ptr = 0;
    int line_idx = 0;
    
    int gui_mode = 0;

    while(code[ptr]) {
        line_idx = 0;
        while(code[ptr] && code[ptr]!='\n' && line_idx < 63) {
            line[line_idx++] = code[ptr++];
        }
        line[line_idx] = 0;
        if(code[ptr]=='\n') ptr++;

        if(str_contains(line, "import tkinter")) {
            print("Importing GUI Library...\n");
            gui_mode = 1;
        }
        else if(str_contains(line, "tk.Window")) {
            if(gui_mode) {
                for(int y=5; y<20; y++) {
                    for(int x=10; x<70; x++) {
                        print_c(" ", WHITE, DGREY);
                        set_cursor(x, y);
                    }
                }
                set_cursor(10, 5);
                print_c(" tk ", BLACK, LGREY);
            }
        }
        else if(str_contains(line, "tk.Label")) {
            if(gui_mode) {
                char text[32]; extract_string(line, text);
                set_cursor(12, 7);
                print_c(text, WHITE, DGREY);
            }
        }
        else if(str_contains(line, "tk.Button")) {
            if(gui_mode) {
                char text[32]; extract_string(line, text);
                set_cursor(12, 10);
                print_c("[ ", BLACK, LGREY);
                print_c(text, BLACK, LGREY);
                print_c(" ]", BLACK, LGREY);
            }
        }
        else if(str_contains(line, "print")) {
            char text[32]; extract_args(line, text);
            if(text[0] == '"') {
                char clean[32]; extract_string(line, clean);
                print(">> "); print(clean); print("\n");
            } else {
                print(">> "); print(text); print("\n");
            }
        }
        else if(str_contains(line, "=")) {
        }
    }
    
    if(gui_mode) {
        set_cursor(0, 22);
        print("GUI Application Finished. Press Key.");
        while(!(inb(0x64)&1)); inb(0x60);
        clear_screen();
    }
}

void exec_c(char* code) {
    print_c(" [TiwutCC - C Compiler]\n", CYAN, BLACK);
    
    char line[64];
    int ptr = 0;
    int line_idx = 0;
    int in_main = 0;
    
    int var_val = 0;
    char var_name = 0;

    while(code[ptr]) {
        line_idx = 0;
        while(code[ptr] && code[ptr]!='\n' && line_idx < 63) {
            line[line_idx++] = code[ptr++];
        }
        line[line_idx] = 0;
        if(code[ptr]=='\n') ptr++;

        if(str_contains(line, "int main")) {
            print("Entry point found.\n");
            in_main = 1;
        }
        else if(in_main) {
            if(str_contains(line, "printf")) {
                char text[32]; extract_string(line, text);
                print(text); print("\n");
            }
            else if(str_contains(line, "int ")) {
                int i=0; while(line[i] && line[i]!='t') i++; i++; // skip int
                while(line[i]==' ') i++;
                var_name = line[i]; // 'a'
                while(line[i] && line[i]!='=') i++; 
                if(line[i]=='=') {
                    i++; while(line[i]==' ') i++;
                    var_val = line[i] - '0';
                    print("Allocated int "); 
                    char t[2] = {var_name,0}; print(t);
                    print(" = "); 
                    char v[2] = {line[i],0}; print(v);
                    print("\n");
                }
            }
            else if(str_contains(line, "return")) {
                print("Process exited with code 0.\n");
                return;
            }
        }
    }
}
