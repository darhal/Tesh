#include "tesh.h"

int main(int argc, char** argv) 
{
    Shell shell = { NULL, NULL, 0, 0 };

    parse_args(&shell, argc, argv);
    shell_loop(&shell);
    destroy_shell(&shell);

    return 0;
}
