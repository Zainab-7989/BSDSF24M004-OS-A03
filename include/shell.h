
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
#define MAXARGS 64
#define ARGLEN 256
#define HISTORY_SIZE 20
#define PROMPT "FCIT> "

char** tokenize(char* cmdline);
int execute(char** arglist, int background);
void add_job(pid_t pid);
void reap_jobs(void);
void print_jobs(void);
int handle_builtin(char** arglist);
void store_command_history(const char* command);
void print_history(void);
char* get_history_command(int number);
char** shell_completion(const char* text, int start, int end);

int handle_variable_assignment(char** arglist);
void expand_variables(char** arglist);
void print_shell_variables(void);

#endif
