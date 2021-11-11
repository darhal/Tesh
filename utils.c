#include "utils.h"
#include <stdlib.h>
#include <unistd.h>

int read_input(int fd, char** input, int* cap)
{
    int cursor = 0;
    int bytes_read = 0;

    do {
        if (cursor + 1 >= *cap) {
            *cap *= 2;
            *input = realloc(input, *cap * sizeof(char));
        }

        bytes_read = read(fd, *input + cursor, 1);
        cursor += 1;

        if ((*input)[cursor-1] == '\n')
            break;
    } while(bytes_read > 0);

    // Force terminate the string and reset cursor
    (*input)[cursor-1] = '\0';
    return bytes_read > 0;
}