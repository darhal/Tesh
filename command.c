#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>


//Execute a parsed command 
void exec_command(char** parsed){
    if (fork() == 0) {
            int code = execvp(parsed[0], parsed);
            if (code != 0) {
                perror("Could not execute command");
            }
        }
    else{
        wait(NULL); 
        return; 
    }
}


//Execute builtin command (cd)
void exec_builtin(char** parsed){
    if ((strcmp(parsed[1], "~") == 0) || (parsed[1] == NULL)) {
        char* dir = getenv("HOME");
        chdir(dir);
        return;
    }
    chdir(parsed[1]) ;
    return;
}


//Execute command (general)
void exec_command_gen(char** parsed){
    if (strcmp(parsed[0],"cd") == 0){
        exec_builtin(parsed);
    }
    else {
        exec_command(parsed) ;
    }
}