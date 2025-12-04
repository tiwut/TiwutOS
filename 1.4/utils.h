#ifndef UTILS_H
#define UTILS_H

typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

class Utils {
public:
    static int strlen(const char* s) { int i=0; while(s[i])i++; return i; }
    static int strcmp(const char* a, const char* b) { while(*a && *a==*b){a++;b++;} return *a-*b; }
    static void strcpy(char* d, const char* s) { while(*s) *d++ = *s++; *d=0; }
    static void memset(void* d, uint8_t v, int n) { uint8_t* p=(uint8_t*)d; while(n--)*p++=v; }
    static void int_to_str(int n, char* b) {
        if(n==0){b[0]='0';b[1]=0;return;}
        int i=0, k=n; while(k>0){k/=10;i++;} b[i]=0;
        while(n>0){b[--i]=(n%10)+'0'; n/=10;}
    }
};
#endif