#ifndef UTILS_H
#define UTILS_H

extern int readline_fd;

int read_input(int fd, char** input, int* cap);

char* my_readline(const char* prompt);

#endif