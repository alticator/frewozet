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

int strequal(const char* a, const char* b) {
    return strcmp(a, b) == 0;
}

int strnequal(const char* a, const char* b, size_t n) {
    return strncmp(a, b, n) == 0;
}

char* strcpy(char* dst, const char* src) {
    size_t i = 0;
    while (src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
    return dst;
}

void str_append(char* dest, size_t *pos, const char* src) {
    while (*src != '\0') {
        dest[(*pos)++] = *src++;
    }
    dest[*pos] = '\0';
}

void str_appendchar(char* dest, size_t *pos, char c) {
    dest[(*pos)++] = c;
    dest[*pos] = '\0';
}

void str_append_uint64(char* dest, size_t *pos, uint64_t value) {
    char buffer[21]; // Max length for 64-bit integer + null terminator
    size_t i = 0;
    if (value == 0) {
        buffer[i++] = '0';
    } else {
        while (value > 0) {
            buffer[i++] = '0' + (value % 10);
            value /= 10;
        }
        // Reverse the digits
        for (size_t j = 0; j < i / 2; j++) {
            char temp = buffer[j];
            buffer[j] = buffer[i - 1 - j];
            buffer[i - 1 - j] = temp;
        }
    }
    buffer[i] = '\0';
    str_append(dest, pos, buffer);
}