#include "types.h"

class Utils {
public:
    static void memcpy(void* dest, const void* src, int count) {
        uint32_t* d = (uint32_t*)dest;
        const uint32_t* s = (const uint32_t*)src;
        count /= 4;
        while(count--) *d++ = *s++;
    }

    static void memset(void* dest, uint32_t val, int count) {
        uint32_t* d = (uint32_t*)dest;
        while(count--) *d++ = val;
    }

    static int strlen(const char* s) { int i=0; while(s[i])i++; return i; }
    
    static void strcpy(char* d, const char* s) {
        while(*s) *d++ = *s++;
        *d = 0;
    }
};