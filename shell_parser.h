#ifndef SHELL_PARSER_H
#define SHELL_PARSER_H

#define SHELL_MAX_ARGS 16

int shell_tokenize(char* input, char** argv, int max_args);

#endif