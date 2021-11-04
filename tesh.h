#ifndef TESH_H
#define TESH_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "parser.h"

void get_prompt(char* buff);

int exec_command_gen(AbstractOp* curr, AbstractOp* prev, AbstractOp* next, int pipes[2][2], int* cp);

int exec_compound_cmd(AbstractOp* cmd);

void execute_commands(AbstractOp* cmds, int nb);

void process_input(char* input);


#endif