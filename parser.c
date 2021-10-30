#include "parser.h"

int count_chars(const char* string, char sep)
{
    int count = 0;

    while (*string) {
        if (*(string++) == sep)
            count++;
    }

    return count;
}


int contains(const char* string, const char** tokens, int nb_tok)
{
    if (!string) 
        return 0;
    
    for (int i = 0; i < nb_tok; i++) {
        if (tokens[i] && strcmp(string, tokens[i]) == 0) {
            return i;
        }
    }

    return 0;
}

int parse(char* string, const char* sep, char*** tokens, AbstractOp** ops)
{
    char* token;
    int i = 0;
    int nb_spcs = count_chars(string, sep[0]) * 2;
    *tokens = calloc(nb_spcs, sizeof(char*));
    *ops = calloc(1, sizeof(AbstractOp));
    AbstractOp* curr_op = *ops;
    token = strtok(string, sep);
    
    while (token) {
        (*tokens)[i] = token;
        int tok_code = contains(token, TOKENS, END);
        token = strtok(NULL, sep);

        if (tok_code > NONE && tok_code < END) {
            if (curr_op->op) {
                (*tokens)[i++] = NULL;
                curr_op->end++;
                curr_op->next = calloc(1, sizeof(AbstractOp));
                curr_op = curr_op->next;
            }
            
            curr_op->start = i;
            curr_op->end = i;
            curr_op->op = tok_code;
        }else if (curr_op->op != CUSTOM){
            if (curr_op->op) {
                curr_op->next = calloc(1, sizeof(AbstractOp));
                curr_op = curr_op->next;
            }

            curr_op->start = i;
            curr_op->end = i;
            curr_op->op = CUSTOM;
        }else{
            curr_op->end++;
        }

        i++;
    }

    if (curr_op->op == CUSTOM) {
        (*tokens)[i++] = NULL;
        curr_op->end++;
    }

    return i;
}