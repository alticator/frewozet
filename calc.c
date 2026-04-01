#include "calc.h"

static int is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n';
}

void run_calc(const char* expression) {
    const char* ptr = expression;
    int ok = 0;
    while (is_space(*ptr)) ptr++;
    uint32_t left = parse_uint(&ptr, &ok);
    if (!ok) {
        colorshell_write("Error: Expected a number at the beginning of the expression.\n", "error");
        return;
    }
    while (is_space(*ptr)) ptr++;
    char op = *ptr;
    if (op != '+' && op != '-' && op != '*' && op != '/') {
        colorshell_write("Error: Expected an operator (+, -, *, /).\n", "error");
        return;
    }
    ptr++;
    while (is_space(*ptr)) ptr++;
    uint32_t right = parse_uint(&ptr, &ok);
    if (!ok) {
        colorshell_write("Error: Expected a number after the operator.\n", "error");
        return;
    }
    if (!is_space(*ptr) && *ptr != '\0') {
        colorshell_write("Error: Unexpected characters after expression.\n", "error");
        colorshell_write("Only single operations permitted.\n", "error");
        return;
    }
    uint32_t result = 0;
    if (op == '+') {
        result = left + right;
    } else if (op == '-') {
        if (left < right) {
            colorshell_write("Result: -", "output");
            colorshell_write_uint(right - left, "output");
            terminal_write("\n");
            return;
        }
        result = left - right;
    } else if (op == '*') {
        result = left * right;
    } else if (op == '/') {
        if (right == 0) {
            colorshell_write("Error: Division by zero.\n", "error");
            return;
        }
        result = left / right;
    } else {
        colorshell_write("Error: Unknown operator.\n", "error");
        return;
    }
    colorshell_write("Result: ", "output");
    colorshell_write_uint(result, "output");
    terminal_write("\n");
}