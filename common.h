#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <stdlib.h>

enum BUILTIN_TOKENS
{
    NONE      = 0,

    CD        = 1,

    SEMICOLON = 1 << 1,
    AND       = 1 << 2,
    OR        = 1 << 3,
    PIPE      = 1 << 4,
    PARALLEL  = 1 << 5,

    D_RIGHT   = 1 << 9,
    CD_RIGHT  = 1 << 7,
    D_LEFT    = 1 << 8,
    CD_LEFT   = 1 << 9,

    CUSTOM    = 1 << 10,
    COMPOUND  = 1 << 11,
    
    AND_OR      = AND | OR,
    CTRL_FLOW   = AND_OR | SEMICOLON | PIPE | PARALLEL,
    REDIR_LEFT  = D_LEFT | CD_LEFT,
    REDIR_RIGHT = D_RIGHT | CD_RIGHT,
    REDIRS      = REDIR_LEFT | REDIR_RIGHT,
    NOT_CUSTOM  = CTRL_FLOW | REDIRS,
    ALL         = 0xffffffff,
};

extern const char* TOKENS[];
extern const int TOKENS_SIZE;

#define FD_READ 0
#define FD_WRITE 1

#endif