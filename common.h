#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

enum BUILTIN_TOKENS
{
    NONE      = 0,

    PARALLEL  = 1,

    SEMICOLON = 1 << 1,
    AND       = 1 << 2,
    OR        = 1 << 3,
    PIPE      = 1 << 4,

    CD        = 1 << 5,
    FG        = 1 << 6,

    D_RIGHT   = 1 << 7,
    CD_RIGHT  = 1 << 8,
    D_LEFT    = 1 << 9,
    CD_LEFT   = 1 << 10,

    TEXT     = 1 << 11,
    COMPOUND = 1 << 12,
    COMMAND  = 1 << 13,
    
    HCTRL_FLOW  = PARALLEL,
    AND_OR      = AND | OR,
    BUILTIN     = CD | FG,
    CTRL_FLOW   = AND_OR | SEMICOLON | PIPE | BUILTIN,
    REDIR_LEFT  = D_LEFT | CD_LEFT,
    REDIR_RIGHT = D_RIGHT | CD_RIGHT,
    REDIRS      = REDIR_LEFT | REDIR_RIGHT,
    NOT_CUSTOM  = CTRL_FLOW | REDIRS | HCTRL_FLOW,
    ALL         = 0xffffffff,
};

extern const char* TOKENS[];
extern const int TOKENS_SIZE;

#define FD_READ 0
#define FD_WRITE 1

enum OPTIONS
{
    QUIT_ON_ERR = 1,
    INTERACTIVE = 1 << 1,
};

typedef struct _Shell 
{
    const char* scriptfile;
    pid_t* background;
    int count;
    int capacity;
    int options;
} Shell;

#endif