#include "os.h"

void strcpy(char* d, const char* s) { while(*s) *d++ = *s++; *d=0; }
int strlen(const char* s) { int i=0; while(s[i])i++; return i; }
void itoa(int n, char* b) {
    if(n==0){b[0]='0';b[1]=0;return;}
    int i=0,k=n; while(k>0){k/=10;i++;} b[i]=0;
    while(n>0){b[--i]=(n%10)+'0';n/=10;}
}

void app_terminal_input(AppBuffer* term, char c) {
    if(c == '\n') {
        term->row++; term->col=0;
        strcpy(term->lines[term->row], "root@tiwut: ");
        term->col = 12;
    } else if(c == '\b') {
        if(term->col > 12) { term->col--; term->lines[term->row][term->col] = 0; }
    } else {
        if(term->col < 38) {
            term->lines[term->row][term->col] = c;
            term->lines[term->row][term->col+1] = 0;
            term->col++;
        }
    }
    if(term->row >= 9) term->row = 0;
}