#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <dlfcn.h>
#include "tesh.h"
#include "readline.h"

// #define NEW_APPROACH

void get_prompt(char** prompt, int* cap)
{
    char hostname[1024];
    gethostname(hostname, 1024);
    char* cwd = getcwd(NULL, 0); // as of POSIX.1-2001
    const char* username = getenv("USER");
    int total_size = strlen(username) + strlen(cwd) + 1024 + 8;

    if (total_size >= *cap) {
        *prompt = realloc(*prompt, total_size * sizeof(char));
        *cap = total_size;
    }
    
    sprintf(*prompt, "%s@%s:%s$ ", username, hostname, cwd);
    free(cwd);
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
                flags = O_CREAT | O_RDONLY; // | (prev->op == D_LEFT) ? O_TRUNC : O_APPEND;
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

        if (code != 0) {
            fprintf(stderr, "The given command '%s' is invalid\n", prog_cmd->token[0]);
            exit(code);
        }
    }else{
        exit(-1);
    }

    return code;
}

int exec_command_gen(Shell* shell, AbstractOp* curr, AbstractOp* prev, AbstractOp* next, 
                    int* pipes, pid_t* pids, int* pcount, int* progress, int sfork)
{
    pid_t child = -1;
    int prevIsPipe  = prev && prev->op == PIPE;
    int nextIsPipe  = next && next->op == PIPE;
#if defined(NEW_APPROACH)
    int cpi = *pcount;
#else
    int cpi = 1;
#endif
    int* write_pipe = &pipes[cpi * 2];
    int* read_pipe  = (cpi - 1) >= 0 ? &pipes[(cpi - 1) * 2] : NULL;

    if (curr->op & BUILTIN) {
        AbstractOp* arg = NULL;

        if (next && next->op == COMMAND && next->opsArr && next->opsCount) {
            arg = &next->opsArr[0];
            (*progress)++;
        }

        return exec_builtin(shell, curr, arg);
    } else if (curr->op == COMMAND) {
        if (nextIsPipe) {
            pipe(write_pipe);
        }
        
        child = sfork ? fork() : 0;
        
        if (child == 0) {
            if (prevIsPipe && read_pipe) {
                // Read from pipe
                close(read_pipe[FD_WRITE]);
                assert(dup2(read_pipe[FD_READ], STDIN_FILENO) != -1);
                close(read_pipe[FD_READ]);
            }

            if (nextIsPipe) {
                // Write to pipe
                close(write_pipe[FD_READ]);
                assert(dup2(write_pipe[FD_WRITE], STDOUT_FILENO) != -1);
                close(write_pipe[FD_WRITE]);
            }

            return exec_compound_cmd(shell, curr);
        }else if (child > 0 && nextIsPipe) {
            pids[*pcount] = child;
            // usleep(1000);
#if defined(NEW_APPROACH)
            // close(pipes[*pcount * 2 + FD_WRITE]);
            // (*pcount)++;
#endif
        }
    }else if (curr->op == PIPE) {
#if defined(NEW_APPROACH)
        close(write_pipe[FD_WRITE]);
        (*pcount)++;
#else 
        if (read_pipe[FD_READ]) {
            close(read_pipe[FD_READ]);
        }
        read_pipe[FD_READ] = write_pipe[FD_READ];
        close(write_pipe[FD_WRITE]);
        write_pipe[FD_WRITE] = 0;
#endif
    }

    if (child != -1) {
        int status = 0;
#if defined(NEW_APPROACH)
        if (!nextIsPipe) {
            if (*pcount != 0) {
                for (int i = 0; i < *pcount; i++) {
                    close(pipes[i * 2 + FD_READ]);
                    while (waitpid(pids[i], &status, 0) != pids[i]) { }
                    status = status || WEXITSTATUS(status);
                }

                *pcount = 0;
            }else{
                while (waitpid(child, &status, 0) != child) { }
                status =  WEXITSTATUS(status);
            }
        }
#else
        if (!nextIsPipe || prevIsPipe) {
            while (waitpid(child, &status, 0) != child) { }
            status =  WEXITSTATUS(status);
        }
#endif
        // printf("Child %d exited with code %d\n", child, status);
        return status;
    }

    return 0;
}

int execute_commands(Shell* shell, AbstractOp* cmds, int nb, int sfork)
{
    int status = 0;
    int pcount = 0;
    int* pipes = calloc(nb * 2, 2 * sizeof(int));
    pid_t* pids = calloc(nb * 2, sizeof(pid_t));
    int i = 0;

    for (; i < nb; i++) {
        AbstractOp* curr = &cmds[i];
        AbstractOp* prev = lla_prev(cmds, i, nb);
        AbstractOp* next = lla_next(cmds, i, nb);
        int is_succ = status == 0;
        int continue_exec = !(curr->op & AND_OR) || (curr->op == AND && is_succ) || (curr->op == OR && !is_succ);

        if (continue_exec) {
            // printf("Executing : "); print_command_tokens(curr); printf("\n");
            status = exec_command_gen(shell, curr, prev, next, pipes, pids, &pcount, &i, 1);
        }else{
            i = fast_forward(cmds, i, nb, AND_OR | SEMICOLON);
        }
    }

#if !defined(NEW_APPROACH)
    if (pipes[0 + FD_WRITE])
        close(pipes[0 + FD_WRITE]);
    if (pipes[0 + FD_READ])
        close(pipes[0 + FD_READ]);
    if (pipes[1 * 2 + FD_WRITE])
        close(pipes[1 * 2 + FD_WRITE]);
    if (pipes[1 * 2 + FD_READ])
        close(pipes[1 * 2 + FD_READ]);
#endif

    free(pipes);
    free(pids);
    return status;
}

int pp_commands(Shell* shell, AbstractOp* cmds, int nb)
{
    int status = 0;

    for (int i = 0; i < nb; i++) {
        AbstractOp* curr = &cmds[i];
        AbstractOp* next = lla_next(cmds, i, nb);
        int prun = next && next->op == PARALLEL;

        if ((shell->options & QUIT_ON_ERR) && status != 0) {
            return status;
        }
        
        if (curr->op & HCTRL_FLOW){
            continue;
        }else if (prun) {
            pid_t child = fork();

            if (child > 0) {
                printf("[%d]\n", child);
                fflush(stdout);
                add_bg_proc(shell, child);
            }else if (child == 0) {
                // printf("Executing parallel : "); print_command_tokens(curr); printf("\n");
                usleep(5000);
                int status = execute_commands(shell, curr->opsArr, curr->opsCount, curr->opsCount > 1);
                exit(status);
            }
        }else{
            // printf("Executing serial : "); print_command_tokens(curr); printf("\n");
            status = execute_commands(shell, curr->opsArr, curr->opsCount, 1);
        }
    }

    return status;
}

int process_input(Shell* shell, char* input)
{
    char** tokens;
    AbstractOp* ops = NULL;
    int nb_ops = 0;
    /*int count = */parse(input, " ", &tokens, &ops, &nb_ops);
    int status = pp_commands(shell, ops, nb_ops);
    free_abstract_op(ops, nb_ops);
    free(tokens);
    return status;
}

void destroy_shell(Shell* shell)
{
    if (shell->background) {
        free(shell->background);
    }
}

void main_loop(Shell* shell, int fd)
{
    if (rl_bind_key && rl_complete)
        rl_bind_key('\t', rl_complete);
    
    char* prompt = NULL;
    int prompt_cap = 0;
    int status = 0;
    int atty = isatty(fd);
    readline_fd = fd;

    while (1) {
        // Check background procs
        // check_bg_proc(shell);

        // Display prompt and read input
        if (atty) {
            get_prompt(&prompt, &prompt_cap);
        }
        
        // Get input
        char* input = readline(prompt);

        // Check for EOF or ERROR
        if (!input)
            break;

         // Add input to readline history
        if (add_history) {
            add_history(input);
        }

        // Process current input
        status = process_input(shell, input);
        // Free input
        free(input);

        // Check the command status and leave when QUIT_ON_ERR is specified
        if ((shell->options & QUIT_ON_ERR) && status != 0) {
            break;
        }
    }

    if (add_history)
        rl_clear_history();

    // Free allocated buffer
    if (prompt)
        free(prompt);
}

int loop_interactive(Shell* shell)
{
    void* lib_readline = NULL;

    if (shell->options & INTERACTIVE) {
        lib_readline = dlopen("libreadline.so", RTLD_LAZY);
        *(void**)(&rl_command_func) = dlsym(lib_readline, "rl_command_func");
        *(void**)(&rl_bind_key) = dlsym(lib_readline, "rl_bind_key");
        *(void**)(&rl_complete) = dlsym(lib_readline, "rl_complete");
        *(void**)(&readline) = dlsym(lib_readline, "readline");
        *(void**)(&add_history) = dlsym(lib_readline, "add_history");
        *(void**)(&rl_clear_history) = dlsym(lib_readline, "rl_clear_history");
    }

    main_loop(shell, STDIN_FILENO);

    if (lib_readline)
        dlclose(lib_readline);
    return 1;
}

int loop_file(Shell* shell, const char* filename)
{
    int fd = open(filename, O_RDONLY);
    
    if (fd == -1)
        return 0;

    main_loop(shell, fd);
    close(fd);
    return 1;
}

void shell_loop(Shell* shell)
{
    // By default we use our readline
    readline = my_readline;

    if (shell->scriptfile) {
        loop_file(shell, shell->scriptfile);
    }else{
        loop_interactive(shell);
    }
}

void parse_args(Shell* shell, int argc, char** argv)
{
    int opt;

    while ((opt = getopt(argc, argv, "er")) != -1) { 
        switch(opt) {
        case 'r':
            shell->options |= INTERACTIVE;
            break;
        case 'e':
            shell->options |= QUIT_ON_ERR;
            break;
        }
    }

    // Additional arguments
    for(; optind < argc; optind++) {
        shell->scriptfile = argv[optind];
        break;
    }
}
