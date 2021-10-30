#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <stdlib.h>

enum BUILTIN_TOKENS
{
    NONE,

    CD,
    LS,
    END_UNARY,

    SEMICOLON,
    PIPE,
    AND,
    OR,
    D_RIGHT,
    CD_RIGHT,
    D_LEFT,
    CD_LEFT,
    END,

    CUSTOM,
};

extern const char* TOKENS[];

#endif