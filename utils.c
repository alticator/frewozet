#include "utils.h"
#include "string.h"

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

void format_bytes(uint64_t bytes, char* out) {
    size_t pos = 0;
    const uint64_t KB = 1024;
    const uint64_t MB = KB * 1024;
    const uint64_t GB = MB * 1024;
    if (bytes < KB) {
        str_append_uint64(out, &pos, bytes);
        str_append(out, &pos, " B");
    } else if (bytes < MB) {
        uint64_t whole = bytes / KB;
        uint64_t rem = bytes % KB;
        str_append_uint64(out, &pos, whole);
        if (rem >= KB / 10) {
            str_append(out, &pos, ".");
            str_append_uint64(out, &pos, (rem * 10) / KB);
        }
        str_append(out, &pos, " KB");
    } else if (bytes < GB) {
        uint64_t whole = bytes / MB;
        uint64_t rem = bytes % MB;
        str_append_uint64(out, &pos, whole);
        if (rem >= MB / 10) {
            str_append(out, &pos, ".");
            str_append_uint64(out, &pos, (rem * 10) / MB);
        }
        str_append(out, &pos, " MB");
    } else {
        uint64_t whole = bytes / GB;
        uint64_t rem = bytes % GB;
        str_append_uint64(out, &pos, whole);
        if (rem >= GB / 10) {
            str_append(out, &pos, ".");
            str_append_uint64(out, &pos, (rem * 10) / GB);
        }
        str_append(out, &pos, " GB");
    }
    out[pos] = '\0';
}