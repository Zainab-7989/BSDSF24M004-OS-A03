#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30
#define HISTORY_SIZE 20
#define PROMPT "FCIT> "

char** tokenize(char* cmdline);
int execute(char** arglist);
int handle_builtin(char** arglist);
void store_command_history(const char* command);
void print_history(void);
char* get_history_command(int number);
char** shell_completion(const char* text, int start, int end);

#endif
