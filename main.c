#include <readline/readline.h>
#include <readline/history.h>
#include "tesh.h"

int main(int argc, const char** argv) 
{
    Shell shell = {NULL, 0, 0};
    rl_bind_key('\t', rl_complete);
    char prompt_buff[1024];

    while (1) {
        // Check background procs
        check_bg_proc(&shell);

        // Display prompt and read input
        get_prompt(prompt_buff);
        char* input = readline(prompt_buff);

        // Check for EOF.
        if (!input)
            break;

        // Add input to readline history.
        add_history(input);

        // Process current input
        process_input(&shell, input);

        // Free buffer that was allocated by readline
        free(input);
    }

    destroy_shell(&shell);
    return 0;
}
