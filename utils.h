#ifndef UTILS_H
#define UTILS_H
#include <stdint.h>
#include <stddef.h>

uint32_t parse_uint(const char** str, int* ok);
int strings_equal(const char* a, const char* b);
int string_starts_with(const char* str, const char* prefix);

#endif