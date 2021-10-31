#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "parser.h"
#include "redirections.h"


void get_prompt(char* buff)
{
    char hostname[128];
    char cwd[1024];
    const char* username = getlogin();
    gethostname(hostname, 128);
    getcwd(cwd, 1024);
    sprintf(buff, "%s@%s:%s ", username, hostname, cwd);
}

void process_input(char* input)
{
    char** tokens;
    AbstractOp* ops = NULL;
    /*int count = */parse(input, " ", &tokens, &ops);

    while (ops && ops->op != NONE) {
        if (ops->op == CUSTOM) {
            for (int i = ops->start; i <= ops->end; i++) {
                printf(" %s ", tokens[i]);
            }
        }else{
            printf(" %d ", ops->op);
        }
        
        ops = ops->next;
    }

    printf("\n");
    /*for (int i = 0; i < count; i++) {
        if (tokens[i]) {
            printf("%s\n", tokens[i]);
        }
    }*/
}


int main(int argc, const char** argv) 
{
    rl_bind_key('\t', rl_complete);
    char prompt_buff[1024];

    while (1) {
        // Display prompt and read input
        get_prompt(prompt_buff);
        char* input = readline(prompt_buff);

        // Check for EOF.
        if (!input)
            break;

        // Add input to readline history.
        add_history(input);

        // Process current input
        process_input(input);

        // //execute > command
        // char* parsed3[] = {"echo", "test_left_cd", NULL, (char*)CD_RIGHT , "test.txt", NULL};
        // char* parsed3[] = {"sort", NULL, (char*)D_LEFT , "test.txt", NULL};
        // redirect(parsed3, 5);


        // Free buffer that was allocated by readline
        free(input);
    }

    return 0;
}
