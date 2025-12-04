#include "kernel.h"

int strcmp(char* s1, char* s2) {
    while(*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(char* s1, char* s2, int n) {
    while(n > 0 && *s1 && (*s1 == *s2)) { s1++; s2++; n--; }
    if(n == 0) return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

void strcpy(char* dest, char* src) {
    while(*src) { *dest++ = *src++; }
    *dest = 0;
}

int strlen(char* s) {
    int i = 0; while(s[i]) i++; return i;
}

void *memset(void *s, int c, int n) {
    unsigned char *p = s;
    while(n--) *p++ = (unsigned char)c;
    return s;
}
