#ifndef BG_H
#define BG_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "parser.h"

void add_bg_proc(Shell* shell, pid_t pid);

void remove_bg_proc(Shell* shell, pid_t pid);

void check_bg_proc(Shell* shell);

int wait_bg_proc(Shell* shell, pid_t* pid);

#endif