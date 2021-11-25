#ifndef TESH_H
#define TESH_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "utils.h"
#include "parser.h"
#include "bg.h"

void get_prompt(char** prompt, int* cap);

int exec_builtin(Shell* shell, AbstractOp* cmd, AbstractOp* next);

int exec_command_gen(Shell* shell, AbstractOp* curr, AbstractOp* prev, AbstractOp* next, int pipes[2][2], int* cp, int* progress);

int exec_compound_cmd(Shell* shell, AbstractOp* cmd);

int execute_commands(Shell* shell, AbstractOp* cmds, int nb);

int pp_commands(Shell* shell, AbstractOp* cmds, int nb);

int process_input(Shell* shell, char* input);

void destroy_shell(Shell* shell);

void main_loop(Shell* shell, int fd);

int loop_interactive(Shell* shell);

int loop_file(Shell* shell, const char* filename);

void shell_loop(Shell* shell);

void parse_args(Shell* shell, int argc, char** argv);

#endif