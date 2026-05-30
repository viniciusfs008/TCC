#include "string.h"

void* memset(void *dst, int c, int len) {
    char *d = (char *) dst;
    for(int i = 0; i < len; i++) {
        d[i] = c;
    }
    return dst;
}

void* memcpy(void *dst, const void *src, int len) {
    char *d = (char *) dst;
    const char *s = (const char *) src;
    for(int i = 0; i < len; i++) {
        d[i] = s[i];
    }
    return dst;
}