#include "utils.h"

uint32_t parse_uint(const char** str, int* ok) {
    uint32_t value = 0;
    int found_digit = 0;
    while (**str >= '0' && **str <= '9') {
        found_digit = 1;
        value = value * 10 + (uint32_t)(**str - '0');
        (*str)++;
    }
    *ok = found_digit;
    return value;
}

int strings_equal(const char* a, const char* b) {
    size_t i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return 0;
        }
        i++;
    }
    return a[i] == '\0' && b[i] == '\0';
}

int string_starts_with(const char* str, const char* prefix) {
    size_t i = 0;
    while (prefix[i] != '\0' && str[i] != '\0') {
        if (str[i] != prefix[i]) {
            return 0;
        }
        i++;
    }
    return prefix[i] == '\0';
}