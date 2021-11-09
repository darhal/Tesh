#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

typedef struct _AbstractOp
{
    char** token;
    int    count;
    int    op;
    
    struct _AbstractOp* opsArr;
    int opsCount;
    int opsCap;

    // struct _AbstractOp* right;
} AbstractOp;

typedef struct _LinkedListArr
{
    AbstractOp* arr;
    int pos; 
    int count;
} LinkedListArr;

int count_chars(const char* string, char sep);

int contains(const char* string, const char** tokens, int nb_tok);

AbstractOp* get_next_op(AbstractOp** ops, int* ops_cap, int* nb_ops);

void free_abstract_op(AbstractOp* ops, int count);

int parse(char* string, const char* sep, char*** tokens, AbstractOp** ops, int* nb_ops);

AbstractOp* lla_next(AbstractOp* arr, int pos, int cap);

AbstractOp* lla_prev(AbstractOp* arr, int pos, int cap);

int fast_forward(AbstractOp* arr, int pos, int cap, int flags);

int get_next_builtin(AbstractOp* pcmd);

void print_command_tokens(AbstractOp* curr);

#endif