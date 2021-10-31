#pragma once 

int LEFT(char** parsed, int n);
int RIGHT(char** parsed, int n);
void redirect_RIGHT(char** parsed, int n, char* filename, int mode);
void redirect_LEFT(char** parsed, int n, char* filename, int mode);
void redirect(char** parsed, int n);
void exec_pipe(char** parsed1, char** parsed2);
