#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include "common.h"

int LEFT(char** parsed, int n){      // for < and <<
    for (int i=0; i<n; i++){
        if ( parsed[i]== (char*)D_LEFT ){
            return D_LEFT;
        }
        // if ( parsed[i]== (char*)CD_LEFT ){
        //     return CD_LEFT;
        // }
    }
    return 0;
}


int RIGHT(char** parsed, int n){      // for > and >>
    for (int i=0; i<n; i++){
        if ( parsed[i] == (char*)D_RIGHT ){
            return D_RIGHT;
        }
        if ( parsed[i]== (char*)CD_RIGHT ){
            return CD_RIGHT;
        }
    }
    return 0;
}


void redirect_RIGHT(char** parsed, int n, char* filename, int mode){        // > and >>
    int fd = open(filename, O_WRONLY|O_CREAT|mode, 0666) ;
    if (fd == -1){
        return;
    }

    if (!fork()){
        dup2(fd, STDOUT_FILENO); //child
        close(fd);
        parsed[n-3] = NULL;
        int code = execvp(parsed[0], parsed);
        if (code != 0) {
            perror("Could not execute command");
        }
    }
}


void redirect_LEFT(char** parsed, int n, char* filename){        // <
    int fd = open(filename, O_RDONLY) ;
    if (fd == -1){
            return;
        }
    if (!fork()){
        dup2(fd, STDIN_FILENO);
        close(fd);
        parsed[n-3] = NULL;
        int code = execvp(parsed[0], parsed);
        if (code != 0) {
            perror("Could not execute command");
        }
    }
}


//redirection input/output "> <  >> "
void redirect(char** parsed, int n){

    int right = RIGHT(parsed, n) ;
    int left = LEFT(parsed, n) ;

    if (right) {

        char* filename = parsed[n-2] ;

        if (right == D_RIGHT) {         // >
            redirect_RIGHT(parsed, n, filename, O_TRUNC);
        }
        if (right == CD_RIGHT) {        // >>
            redirect_RIGHT(parsed, n, filename, O_APPEND);  
        }

    }

    else if (left) {

        char* filename = parsed[n-2] ;
        redirect_LEFT(parsed, n, filename);

        // if (left == D_LEFT) {         // <
        //     redirect_LEFT(parsed, n, filename);
        // }
        // if (left == CD_LEFT) {        // <<
        //     redirect_LEFT(parsed, n, filename);  
        // }

    }

}


//pipe 
    // pipe_parsed : list(command1, command2) and command1/2 = parsed
void exec_pipe(char** parsed1, char** parsed2){

    int fd[2];
    pipe(fd);
    if (fork() == 0) {          /* child */
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);
        int code = execvp(parsed1[0], parsed1);
        if (code != 0) {
            perror("Could not execute command");
        }
    } 
    else {                      /* parent */      
        wait(NULL);
        if (fork() == 0){
            close(fd[1]);
            dup2(fd[0], STDIN_FILENO);
            close(fd[0]);
            int code = execvp(parsed2[0], parsed2);
            if (code != 0) {
                perror("Could not execute command");
            }
        } 
        else {
            wait(NULL);
        }
    }
}
