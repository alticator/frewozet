#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>

void* memcpy(void* dest, const void* src, size_t n);
void* memset(void* dest, int value, size_t n);
size_t strlen(const char* str);
int strcmp(const char* a, const char* b);
int strncmp(const char* a, const char* b, size_t n);
int strequal(const char* a, const char* b);
int strnequal(const char* a, const char* b, size_t n);
char* strcpy(char* destination, const char* source);
void str_append(char* dest, size_t *pos, const char* src);
void str_appendchar(char* dest, size_t *pos, char c);
void str_append_uint64(char* dest, size_t *pos, uint64_t value);

#endif