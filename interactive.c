#include <stdio.h>
#include <unistd.h>

#include "parser.h"
#include "command.h"

#define MAXSIZE 1024



//non-interactive mode 
void non_interactive(char* filename) {
    if (!isatty(3)){
        char* droit = "r" ;

        FILE* file = fopen(filename, droit) ;
        if (file == NULL){
            printf("File cannot be read") ;
            return ; 
        } 
        
        char* buffer = calloc(80, sizeof(char)) ;
    
        while (!feof(file)){
            fgets(buffer, MAXSIZE, file) ;
            char** tokens;
            AbstractOp* ops = NULL;
            /* int n = */ parse(buffer, " ", &tokens, &ops) ; 
            exec_command_gen(tokens);
        } 

        free(buffer) ;
        fclose(file) ;
    }
    else {
        return ; 
    }
}
