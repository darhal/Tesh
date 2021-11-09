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

        if (code != 0) {
            fprintf(stderr, "The given command '%s' is invalid\n", prog_cmd->token[0]);
            exit(code);
        }
    }

    return code;
}

int exec_command_gen(Shell* shell, AbstractOp* curr, AbstractOp* prev, AbstractOp* next, int pipes[2][2], int* cp, int* progress)
{
    pid_t child = -1;
    int* write_pipe = pipes[*cp];
    int* read_pipe  = pipes[(*cp + 1) % 2];

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
                close(read_pipe[FD_WRITE]);
                assert(dup2(read_pipe[FD_READ], STDIN_FILENO) != -1);
                close(read_pipe[FD_READ]);
            }

            if (next && next->op == PIPE) {
                // Write to pipe
                // printf("Writting to : %d\n", *cp);
                close(write_pipe[FD_READ]);
                assert(dup2(write_pipe[FD_WRITE], STDOUT_FILENO) != -1);
                close(write_pipe[FD_WRITE]);
            }

            return exec_compound_cmd(shell, curr);
        }
    }else if (curr->op == PIPE) {
        // We need to store this here to make sure we close it after we close the read pipeline 
        // otherwise the descriptors will overlap and it will cause a bug!
        int write_fd = write_pipe[FD_WRITE];
        close(read_pipe[FD_READ]);
        close(read_pipe[FD_WRITE]);
        *cp = (*cp + 1) % 2;
        pipe(pipes[*cp]);
        close(write_fd);
        //printf("Already existinga fd  (cp = %d) : R: %d W: %d\n", *cp, pipes[*cp][FD_READ], pipes[*cp][FD_WRITE]);
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
    int i = 0;

    for (; i < nb; i++) {
        AbstractOp* curr = &cmds[i];
        AbstractOp* prev = lla_prev(cmds, i, nb);
        AbstractOp* next = lla_next(cmds, i, nb);
        int is_succ = status == 0;
        int continue_exec = (!(curr->op & AND_OR) || (curr->op == AND && is_succ) || (curr->op == OR && !is_succ));

        if (continue_exec) {
            // printf("Executing : "); print_command_tokens(curr); printf("\n");
            status = exec_command_gen(shell, curr, prev, next, pipes, &current_pipe, &i);
        }else{
            i = fast_forward(cmds, i, nb, AND_OR | SEMICOLON);
        }

        // printf("[LOOP] fd (cp = %d) : R: %d W: %d\n", 0, pipes[0][FD_READ], pipes[0][FD_WRITE]);
        // printf("[LOOP] fd (cp = %d) : R: %d W: %d\n", 1, pipes[1][FD_READ], pipes[1][FD_WRITE]);
    }

    close(pipes[0][FD_WRITE]);
    close(pipes[0][FD_READ]);
    close(pipes[1][FD_WRITE]);
    close(pipes[1][FD_READ]);
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
            status = execute_commands(shell, curr->opsArr, curr->opsCount);
        }

        // print_command_tokens(curr); printf("\n");
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

void loop_interactive(Shell* shell)
{
    rl_bind_key('\t', rl_complete);
    char* prompt = NULL;
    int prompt_cap = 0;

    while (1) {
        // Check background procs
        check_bg_proc(shell);

        // Display prompt and read input
        get_prompt(&prompt, &prompt_cap);
        char* input = readline(prompt);

        // Check for EOF.
        if (!input)
            break;

        // Add input to readline history.
        add_history(input);

        // Process current input
        process_input(shell, input);

        // Free buffer that was allocated by readline
        free(input);
    }

    // Free allocated buffer 
    free(prompt);
}

void loop_interactive_without_readline(Shell* shell)
{
    char* prompt = NULL;
    int prompt_cap = 0;
    int init_cap = 1024;
    int cursor = 0;
    int c;
    char* input = realloc(NULL, init_cap * sizeof(char));

    while (1) {
        // Check background procs
        check_bg_proc(shell);

        // Display prompt and read input
        get_prompt(&prompt, &prompt_cap);
        printf("%s", prompt);
        fflush(stdout);

        // Get input
        while ((c = getchar()) != '\n' && c != EOF) { 
            if (cursor + 1 > init_cap) {
                init_cap *= 2;
                input = realloc(input, init_cap * sizeof(char));
            }

            input[cursor] = c;
            cursor += 1;
        }

        // Force terminate the string and reset cursor
        input[cursor] = '\0';
        cursor = 0;

        // Process current input
        process_input(shell, input);
    }

    // Free allocated buffer 
    free(input);
    free(prompt);
}

void loop_file(Shell* shell, const char* filename)
{
    int fd = open(filename, O_RDONLY);
    
    if (fd == -1) {
        return;
    }

    if (isatty(fd)) {
        close(fd);
        return;
    }

    int status = 0;
    int bread = 0;
    int lastBegin = 0;
    int init_cap = 1024;
    int cursor = 0;
    char* input = NULL;
    input = realloc(input, init_cap * sizeof(char));
    
    while ((bread = read(fd, input + cursor, init_cap - 1)) > 0) {
        if (cursor + bread > init_cap) {
            init_cap *= 2;
            input = realloc(input, init_cap * sizeof(char));
        }

        cursor += bread;
    }

    for (int i = 0; i < cursor; i++) {
        if ((shell->options & QUIT_ON_ERR) && status != 0) {
            // printf("Aborting ...\n");
            break;
        }

        if (input[i] == '\n' || i == cursor - 1) {
            input[i] = '\0'; // Force terminate the string
            // printf("Command (%d): %s\n", i, input + lastBegin);
            status = process_input(shell, input + lastBegin);
            lastBegin = i + 1;
        }
    }

    free(input);
    close(fd);
}

void shell_loop(Shell* shell)
{
    if (shell->scriptfile) {
        loop_file(shell, shell->scriptfile);
    }else if (shell->options & INTERACTIVE) {
        void* lib_readline = dlopen("libreadline.so", RTLD_LAZY);
        *(void**)(&rl_command_func) = dlsym(lib_readline, "rl_command_func");
        *(void**)(&rl_bind_key) = dlsym(lib_readline, "rl_bind_key");
        *(void**)(&rl_complete) = dlsym(lib_readline, "rl_complete");
        *(void**)(&readline) = dlsym(lib_readline, "readline");
        *(void**)(&add_history) = dlsym(lib_readline, "add_history");
        loop_interactive(shell);
        dlclose(lib_readline);
    }else{
        loop_interactive_without_readline(shell);
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
