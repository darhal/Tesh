#include "common.h"

const char* TOKENS[] = {
    "&",

    ";",
    "&&",
    "||",
    "|",

    "cd",
    "fg",

    ">",
    ">>",
    "<",
    "<<",
};

const int TOKENS_SIZE = sizeof(TOKENS) / sizeof(TOKENS[0]);