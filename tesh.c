#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

void get_prompt(char* buff)
{
    char hostname[128];
    const char* username = getlogin();
    gethostname(hostname, 128);
    sprintf(buff, "%s@%s: ", username, hostname);
}

void process_input(char* input)
{

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

        // Free buffer that was allocated by readline
        free(input);
    }

    return 0;
}
