#include "bg.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>

void add_bg_proc(Shell* shell, pid_t pid)
{
    if (!shell->count || shell->count > shell->capacity) {
        shell->capacity = (shell->capacity + 1) * 2;
        shell->background = realloc(shell->background, shell->capacity * sizeof(pid_t));
    }

    shell->background[shell->count++] = pid;
}

void remove_bg_proc(Shell* shell, pid_t pid)
{
    int i = 0;

    for (; i < shell->count; i++) {
        if (shell->background[i] == pid) {
            shell->background[i] = 0;
            break;
        }
    }

    for (int k = i; k < shell->count-1; k++) {
        // printf("bg[k] = bg[k+1] (k = %d, bg[k] = %d, bg[k+1] = %d)\n", k, shell->background[k], shell->background[k+1]);
        shell->background[k] = shell->background[k+1];
    }

    shell->count--;
}

int wait_bg_proc(Shell* shell, pid_t* pid)
{
    for (int i = shell->count - 1; i >= 0; i--) {
        // printf("%d(pid = %d)(bg pid = %d)\n", i, *pid, shell->background[i]);
        if (*pid == 0 || shell->background[i] == *pid) {
            *pid = shell->background[i];
            int status = 0;
            int res = waitpid(shell->background[i], &status, 0);

            if (res == shell->background[i]) {
                status =  WEXITSTATUS(status);
                remove_bg_proc(shell, res);
                return status;
            }
        }
    }

    *pid = 0;
    return 0;
}

void check_bg_proc(Shell* shell)
{
    int rcount = 0;
    pid_t* to_remove = calloc(shell->count, sizeof(pid_t));

    for (int i = 0; i < shell->count; i++) {
        if (shell->background[i]) {
            int status;
            int res = waitpid(shell->background[i], &status, WNOHANG);
            status =  WEXITSTATUS(status);

            if (res != 0) {
                to_remove[rcount++] = shell->background[i];
            }
        }
    }

    for (int i = 0; i < rcount; i++) {
        int pid = to_remove[i];
        // printf("Removing pid : %d\n", pid);
        remove_bg_proc(shell, pid);
    }

    free(to_remove);
}
