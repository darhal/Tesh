#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int readline_fd = STDIN_FILENO;

int read_input(int fd, char** input, int* cap)
{
    int cursor = 0;
    int bytes_read = 0;

    do {
        if (cursor + 1 >= *cap) {
            *cap *= 2;
            *input = realloc(*input, *cap * sizeof(char));
        }

        bytes_read = read(fd, *input + cursor, 1);
        cursor += 1;

        if ((bytes_read > 0) && ((*input)[cursor-1] == '\n'))
            break;
    } while(bytes_read > 0);

    // Force terminate the string
    (*input)[cursor-1] = '\0';
    return bytes_read > 0;
}

char* my_readline(const char* prompt)
{
    if (prompt) {
        printf("%s", prompt);
        fflush(stdout);
    }

    int input_cap = 512;
    char* input = realloc(NULL, input_cap * sizeof(char));
    int ok = read_input(readline_fd, &input, &input_cap);

    if (ok)
        return input;
    
    free(input);
    return NULL;
}