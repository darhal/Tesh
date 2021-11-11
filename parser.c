#include "parser.h"
#include <stdio.h>

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
            return (1 << i);
        }
    }

    return TEXT;
}

void free_abstract_op(AbstractOp* ops, int count)
{
    for (int i = 0; i < count; i++) {
        free_abstract_op(ops[i].opsArr, ops[i].opsCount);
    }

    if (ops)
        free(ops);
}

AbstractOp* get_next_op(AbstractOp** ops, int* ops_cap, int* nb_ops)
{
    if (*nb_ops >= *ops_cap) {
        *ops_cap = (*ops_cap + 1) * 2;
        *ops = realloc(*ops, *ops_cap * sizeof(AbstractOp));
        memset(*ops + *nb_ops, 0, (*ops_cap - *nb_ops) * sizeof(AbstractOp));
        // printf("> cap  %d size %d\n", *ops_cap, *nb_ops);
    }

    AbstractOp* res = *ops + *nb_ops;
    *nb_ops += 1;
    return res;
}

int parse(char* string, const char* sep, char*** tokens, AbstractOp** ops, int* nb_ops)
{
    char* token;
    *ops = NULL;
    *nb_ops = 0;
    int i = 0;
    int nb_spcs = (count_chars(string, sep[0]) + 1) * 2;
    int ops_cap = 0;
    *tokens = calloc(nb_spcs, sizeof(char*));
    AbstractOp* hdeep[] = { NULL, NULL, NULL };
    token = strtok(string, sep);
    
    while (token) {
        // printf("Token id : %d\n", i);
        (*tokens)[i] = token;
        int tok_code = contains(token, TOKENS, TOKENS_SIZE);
        token = strtok(NULL, sep);
        int end_comp = token == NULL;
        // printf("cap  %d size %d ptr %p\n", ops_cap, *nb_ops, curr_op);
        
        if (tok_code & HCTRL_FLOW) {
            hdeep[0] = get_next_op(ops, &ops_cap, nb_ops);
            hdeep[0]->token = *tokens + i;
            hdeep[0]->count = 1;
            hdeep[0]->op = tok_code;
            hdeep[0] = NULL;
            end_comp |= 1;
            //printf("Adding null 1\n");
        }else if (!hdeep[0]) {
            hdeep[0] = get_next_op(ops, &ops_cap, nb_ops);
            hdeep[0]->op = COMPOUND;
        }

        if (hdeep[0]) {
            if (tok_code & CTRL_FLOW) {
                hdeep[1] = get_next_op(&hdeep[0]->opsArr, &hdeep[0]->opsCap, &hdeep[0]->opsCount);
                hdeep[1]->token = *tokens + i;
                hdeep[1]->count = 1;
                hdeep[1]->op = tok_code;
                end_comp |= 1;
                //printf("Adding null 2 (%s)\n", hdeep[1]->token[0]);
            }else{
                // Begin of command 
                if (!hdeep[2]) {
                    hdeep[2] = get_next_op(&hdeep[0]->opsArr, &hdeep[0]->opsCap, &hdeep[0]->opsCount);
                    hdeep[2]->op = COMMAND;
                }

                AbstractOp* cnext = hdeep[2]->opsCount ? &hdeep[2]->opsArr[hdeep[2]->opsCount - 1] : NULL;

                if (!cnext || (tok_code & REDIRS) || (cnext && cnext->op & REDIRS)) {
                    if ((cnext && cnext->op == TEXT) && (tok_code & REDIRS)) {
                        cnext->token[cnext->count++] = NULL;
                        //printf("Adding null 3 (%d)\n", tok_code);
                    }

                    cnext = get_next_op(&hdeep[2]->opsArr, &hdeep[2]->opsCap, &hdeep[2]->opsCount);
                    cnext->token = *tokens + i;
                    cnext->op    = tok_code;
                }

                cnext->count += 1;
            }
        }

        if (end_comp && hdeep[2] && hdeep[2]->opsCount) {
            AbstractOp* cnext = &hdeep[2]->opsArr[hdeep[2]->opsCount - 1];

            if (cnext->op == TEXT) {
                cnext->token[cnext->count++] = NULL;
                //printf("Adding null 4 \n");
            }

            hdeep[2] = NULL;
        }

        i++;
    }

    return i;
}

void print_command_tokens(AbstractOp* curr)
{
    if (curr->op == COMPOUND) {
        printf("[");
        for (int j = 0; j < curr->opsCount; j++) {
            print_command_tokens(&curr->opsArr[j]);
        }
        printf("]");
    }else if (curr->op == COMMAND) {
        printf("(");
        for (int j = 0; j < curr->opsCount; j++) {
            print_command_tokens(&curr->opsArr[j]);
        }
        printf(")");
    }else if (curr->op & TEXT) {
        for (int k = 0; k < curr->count; k++) {
            char* tk = curr->token[k] ? curr->token[k] : "NULL";
            printf("%s ", tk);
        }
    }else{
        printf(" %d ", curr->op);
    }
}

AbstractOp* lla_next(AbstractOp* arr, int pos, int cap)
{
    return pos + 1 > 0 && pos + 1 < cap ? &arr[pos+1] : NULL;
}

AbstractOp* lla_prev(AbstractOp* arr, int pos, int cap)
{
    return pos - 1 > 0 && pos - 1 < cap ? &arr[pos-1] : NULL;
}

int fast_forward(AbstractOp* arr, int pos, int cap, int flags)
{
    int i = pos + 1;

    for (i = pos + 1; i < cap; i++) {
        if (arr[i].op & flags) {
            return i;
        }
    }

    return cap; 
}

int get_next_builtin(AbstractOp* cmds)
{
    for (int i = 0; i < cmds->opsCount; i++) {
        if (cmds->opsArr[i].count & BUILTIN) {
            return i;
        }
    }

    return -1;
}