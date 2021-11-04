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

    return CUSTOM;
}

void free_abstract_op(AbstractOp* ops, int count)
{
    for (int i = 0; i < count; i++) {
        if (ops[i].opsArr) {
            free(ops[i].opsArr);
        }
    }

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
    AbstractOp* curr_op = NULL; // get_next_op(ops, &ops_cap, nb_ops);
    AbstractOp* compound = NULL;
    token = strtok(string, sep);
    
    while (token) {
        (*tokens)[i] = token;
        int tok_code = contains(token, TOKENS, TOKENS_SIZE);
        token = strtok(NULL, sep);
        // printf("cap  %d size %d ptr %p\n", ops_cap, *nb_ops, curr_op);
        // printf("tok_code = %d\n", tok_code);

        if (tok_code & CTRL_FLOW) {
            curr_op = get_next_op(ops, &ops_cap, nb_ops);
            curr_op->token = *tokens + i;
            curr_op->count = 1;
            curr_op->op = tok_code;
            AbstractOp* cnext = (compound && compound->opsCount) ? &compound->opsArr[compound->opsCount - 1] : NULL;
            
            if (cnext && cnext->op == CUSTOM) {
                (*tokens)[i++] = NULL;
                cnext->count++;
            }

            compound = NULL;
        }else{
            // Begin of compound 
            if (!compound) {
                compound = get_next_op(ops, &ops_cap, nb_ops);
                compound->op = COMPOUND;
            }

            AbstractOp* cnext = compound->opsCount ? &compound->opsArr[compound->opsCount - 1] : NULL;

            if (!cnext || (tok_code & REDIRS) || (cnext && cnext->op & REDIRS)) {
                if ((cnext && cnext->op == CUSTOM) && (tok_code & REDIRS)) {
                    (*tokens)[i++] = NULL;
                    cnext->count++;
                }

                cnext = get_next_op(&compound->opsArr, &compound->opsCap, &compound->opsCount);
                cnext->token = *tokens + i;
                cnext->op    = tok_code;
            }

            cnext->count += 1;
        }

        i++;
    }

    AbstractOp* cnext = (compound && compound->opsCount) ? &compound->opsArr[compound->opsCount - 1] : NULL;
    if (cnext && cnext->op == CUSTOM) {
        (*tokens)[i++] = NULL;
        cnext->count++;
    }

    return i;
}

void print_command_tokens(AbstractOp* curr)
{
    if (curr->op == COMPOUND) {
        printf("(");
        for (int j = 0; j < curr->opsCount; j++) {
            print_command_tokens(&curr->opsArr[j]);
        }
        printf(")");
    }else if (curr->op & CUSTOM) {
        for (int k = 0; k < curr->count; k++) {
            printf("%s ", curr->token[k]);
        }
    }else{
        printf(" %d ", curr->op);
    }

    // printf("\n");
}

AbstractOp* lla_next(AbstractOp* arr, int pos, int cap)
{
    return pos + 1 > 0 && pos + 1 < cap ? &arr[pos+1] : NULL;
}

AbstractOp* lla_prev(AbstractOp* arr, int pos, int cap)
{
    return pos - 1 > 0 && pos - 1 < cap ? &arr[pos-1] : NULL;
}