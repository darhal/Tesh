#include "readline.h"

int (*rl_command_func)(int, int) = NULL;
int (*rl_bind_key)(int, rl_command_func_t*) = NULL;
int (*rl_complete)(int, int) = NULL;
char* (*readline)(const char*) = NULL;
void (*add_history)(const char *) = NULL;
void (*rl_clear_history)() = NULL;