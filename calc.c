#include "calc.h"

static int is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n';
}

static uint32_t parse_uint(const char** str, int* ok) {
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

static void terminal_write_uint(uint32_t value) {
    char buffer[32];
    int length = 0;
    if (value == 0) {
        buffer[length++] = '0';
    } else {
        uint32_t temp = value;
        while (temp > 0) {
            buffer[length++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = 0; i < length / 2; i++) {
            char tmp = buffer[i];
            buffer[i] = buffer[length - 1 - i];
            buffer[length - 1 - i] = tmp;
        }
    }
    buffer[length] = '\0';
    terminal_write(buffer);
}

void run_calc(const char* expression) {
    const char* ptr = expression;
    int ok = 0;
    while (is_space(*ptr)) ptr++;
    uint32_t left = parse_uint(&ptr, &ok);
    if (!ok) {
        terminal_write("Error: Expected a number at the beginning of the expression.\n");
        return;
    }
    while (is_space(*ptr)) ptr++;
    char op = *ptr;
    if (op != '+' && op != '-' && op != '*' && op != '/') {
        terminal_write("Error: Expected an operator (+, -, *, /).\n");
        return;
    }
    ptr++;
    while (is_space(*ptr)) ptr++;
    uint32_t right = parse_uint(&ptr, &ok);
    if (!ok) {
        terminal_write("Error: Expected a number after the operator.\n");
        return;
    }
    if (!is_space(*ptr) && *ptr != '\0') {
        terminal_write("Error: Unexpected characters after expression.\n");
        terminal_write("Only single operations permitted.\n");
        return;
    }
    uint32_t result = 0;
    if (op == '+') {
        result = left + right;
    } else if (op == '-') {
        result = left - right;
    } else if (op == '*') {
        result = left * right;
    } else if (op == '/') {
        if (right == 0) {
            terminal_write("Error: Division by zero.\n");
            return;
        }
        result = left / right;
    } else {
        terminal_write("Error: Unknown operator.\n");
        return;
    }
    terminal_write("Result: ");
    terminal_write_uint(result);
    terminal_write("\n");
}