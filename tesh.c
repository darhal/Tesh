#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "tesh.h"

void get_prompt(char* buff)
{
    char hostname[128];
    char cwd[1024];
    const char* username = getlogin();
    gethostname(hostname, 128);
    getcwd(cwd, 1024);
    sprintf(buff, "%s@%s:%s$ ", username, hostname, cwd);
}

int exec_builtin(Shell* shell, AbstractOp* cmd, AbstractOp* next)
{
    char* arg = (next && next->op == TEXT && next->token  && next->count > 0) ? next->token[0] : NULL;

    if (cmd->op == CD) {
        if (!arg || strcmp(arg, "~") == 0) {
            char* dir = getenv("HOME");
            chdir(dir);
        }else{
            chdir(arg);
        }
    }else if (cmd->op == FG) {
        pid_t pid = arg ? atoi(arg) : 0;
        int status = wait_bg_proc(shell, &pid);

        if (pid) {
            printf("[%d->%d]\n", pid, status);
        }else{
            printf("Couldn't find the specified process or no process running in background !\n");
        }
    }

    return 0;
}

int exec_compound_cmd(Shell* shell, AbstractOp* cmd)
{
    int code = -1;
    AbstractOp* prog_cmd = NULL;

    if (cmd->op != COMMAND) 
        return code;

    for (int i = 0; i < cmd->opsCount; i++) {
        AbstractOp* curr = &cmd->opsArr[i];
        AbstractOp* prev = lla_prev(cmd->opsArr, i, cmd->opsCount);
        // AbstractOp* next = lla_next(cmd->opsArr, i, cmd->opsCount);
        int flags = -1;
        int direction = -1;

        if (curr->op == TEXT) {
            if (i == 0) {
                prog_cmd = curr;
            }else if (prev && (prev->op & REDIR_RIGHT)) {
                flags = O_CREAT | O_WRONLY | ((prev->op == D_RIGHT) ? O_TRUNC : O_APPEND);
                direction = STDOUT_FILENO;
            }else if (prev && (prev->op & REDIR_LEFT)) {
                flags = O_CREAT| O_RDONLY; // | (prev->op == D_LEFT) ? O_TRUNC : O_APPEND;
                direction = STDIN_FILENO;
            }
        }

        if (flags != -1 && direction != -1) {
            if (curr->count) {
                int fd = open(curr->token[0], flags, 0666);
                dup2(fd, direction); 
                close(fd);
            }
        }
    }

    if (prog_cmd) {
        // fprintf(stderr, "Executing : %s\n", prog_cmd->token[0]);
        code = execvp(prog_cmd->token[0], prog_cmd->token);
        if (code != 0)
            fprintf(stderr, "Invalid command\n");
    }

    return code;
}

int exec_command_gen(Shell* shell, AbstractOp* curr, AbstractOp* prev, AbstractOp* next, int pipes[2][2], int* cp, int* progress)
{
    pid_t child = -1;
    int* write_fd = pipes[*cp];
    int* read_fd  = pipes[(*cp + 1) % 2];

    if (curr->op & BUILTIN) {
        AbstractOp* arg = NULL;

        if (next && next->op == COMMAND && next->opsArr && next->opsCount) {
            arg = &next->opsArr[0];
            (*progress)++;
        }

        return exec_builtin(shell, curr, arg);
    } else if (curr->op == COMMAND) {
        child = fork();

        if (child == 0) {
            // printf("(cp = %d) Next is : %d | prev is : %d\n", *cp, next ? next->op : -1, prev ? prev->op : -1);
            if (prev && prev->op == PIPE) {
                // Read from pipe
                // printf("Reading from : %d\n", (*cp + 1) % 2);
                close(read_fd[FD_WRITE]);
                assert(dup2(read_fd[FD_READ], STDIN_FILENO) != -1);
                close(read_fd[FD_READ]);
            }

            if (next && next->op == PIPE) {
                // Write to pipe
                // printf("Writting to : %d\n", *cp);
                close(write_fd[FD_READ]);
                assert(dup2(write_fd[FD_WRITE], STDOUT_FILENO) != -1);
                close(write_fd[FD_WRITE]);
            }

            return exec_compound_cmd(shell, curr);
        }
    }else if (curr->op == PIPE) {
        close(write_fd[FD_WRITE]);
        *cp = (*cp + 1) % 2;
        pipe(pipes[*cp]);
        // printf("[SWAPPING] (cp = %d) Next is : %d | prev is : %d\n", *cp, next ? next->op : -1, prev ? prev->op : -1);
    }

    if (child != -1) {
        int status = 0;
        while (waitpid(child, &status, 0) != child) { }
        status =  WEXITSTATUS(status);
        // printf("Child %d exited with code %d\n", child, status);
        return status;
    }

    return 0;
}

int execute_commands(Shell* shell, AbstractOp* cmds, int nb)
{
    int status = 0;
    int current_pipe = 0;
    int pipes[2][2] = {{0, 0}, {0, 0}};
    assert(pipe(pipes[0]) != -1);
    assert(pipe(pipes[1]) != -1);

    for (int i = 0; i < nb; i++) {
        AbstractOp* curr = &cmds[i];
        AbstractOp* prev = lla_prev(cmds, i, nb);
        AbstractOp* next = lla_next(cmds, i, nb);
        int is_succ = status == 0;
        int continue_exec = !(curr->op & AND_OR) || (curr->op == AND && is_succ) || (curr->op == OR && !is_succ);
        
        if (continue_exec) {
            //printf("Executing : ");
            //print_command_tokens(curr); printf("\n");
            status = exec_command_gen(shell, curr, prev, next, pipes, &current_pipe, &i);
        }else{
            return status;
        }
    }

    return status;
}

void pp_commands(Shell* shell, AbstractOp* cmds, int nb)
{
    for (int i = 0; i < nb; i++) {
        AbstractOp* curr = &cmds[i];
        AbstractOp* next = lla_next(cmds, i, nb);
        int prun = next && next->op == PARALLEL;
        
        if (prun) {
            pid_t child = fork();

            if (child == 0) {
                int status = execute_commands(shell, curr->opsArr, curr->opsCount);
                exit(status);
            }else{
                add_bg_proc(shell, child);
                printf("[%d]\n", child);
            }
        }else{
            // print_command_tokens(curr); printf("\n");
            execute_commands(shell, curr->opsArr, curr->opsCount);
        }
    }
}

void process_input(Shell* shell, char* input)
{
    char** tokens;
    AbstractOp* ops = NULL;
    int nb_ops = 0;
    /*int count = */parse(input, " ", &tokens, &ops, &nb_ops);
    pp_commands(shell, ops, nb_ops);
    free_abstract_op(ops, nb_ops);
    free(tokens);
}

void destroy_shell(Shell* shell)
{
    if (shell->background) {
        free(shell->background);
    }
}