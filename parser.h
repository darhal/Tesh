#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

typedef struct _AbstractOp
{
    int start;
    int end;
    int op;
    struct _AbstractOp* next;
} AbstractOp;

int count_chars(const char* string, char sep);

int contains(const char* string, const char** tokens, int nb_tok);

int parse(char* string, const char* sep, char*** tokens, AbstractOp** ops);

#endif