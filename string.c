#include "string.h"

void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

void* memset(void* dest, int value, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    uint8_t v = (uint8_t)value;
    for (size_t i = 0; i < n; i++) {
        d[i] = v;
    }
    return dest;
}

size_t strlen(const char* str) {
    size_t length = 0;
    while (str[length] != '\0') {
        length++;
    }
    return length;
}

int strcmp(const char* a, const char* b) {
    size_t i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return (int)((unsigned char)a[i]) - (int)((unsigned char)b[i]);
        }
        i++;
    }
    return (int)((unsigned char)a[i]) - (int)((unsigned char)b[i]);
}

int strncmp(const char* a, const char* b, size_t n) {
    size_t i = 0;
    while (i < n && a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return (int)((unsigned char)a[i]) - (int)((unsigned char)b[i]);
        }
        i++;
    }
    if (i == n) {
        return 0;
    }
    return (int)((unsigned char)a[i]) - (int)((unsigned char)b[i]);
}
