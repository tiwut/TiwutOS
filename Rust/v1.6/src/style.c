#include <stdint.h>

// --- Deine ursprüngliche Style-Funktion ---
uint8_t calculate_style(uint8_t foreground, uint8_t background) {
    return foreground | (background << 4);
}

// --- Speicher-Funktionen für den Kernel (damit der Linker zufrieden ist) ---

// Kopiert Speicher von src nach dest
void *memcpy(void *dest, const void *src, unsigned long n) {
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;
    for (unsigned long i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }
    return dest;
}

// Setzt Speicher auf einen bestimmten Wert
void *memset(void *s, int c, unsigned long n) {
    uint8_t *p = (uint8_t *)s;
    for (unsigned long i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }
    return s;
}

// Vergleicht zwei Speicherbereiche
int memcmp(const void *s1, const void *s2, unsigned long n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;
    for (unsigned long i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    return 0;
}