#ifndef READLINE_H
#define READLINE_H

#include <stdlib.h>

// readline functions
typedef int rl_command_func_t(int, int);
extern int (*rl_command_func)(int, int);
extern int (*rl_bind_key)(int, rl_command_func_t*);
extern int (*rl_complete)(int, int);
extern char* (*readline)(const char*);
extern void (*add_history)(const char*);

#endif