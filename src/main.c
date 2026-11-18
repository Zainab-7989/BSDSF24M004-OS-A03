
#include "shell.h"

static int has_io_operator(char** arglist) {
    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], "<") == 0 ||
            strcmp(arglist[i], ">") == 0 ||
            strcmp(arglist[i], "|") == 0) {
            return 1;
        }
    }
    return 0;
}

static void free_arguments(char** arglist) {
    if (arglist == NULL) return;

    for (int i = 0; arglist[i] != NULL; i++) {
        free(arglist[i]);
    }

    free(arglist);
}

static void process_command(char** tokens, int background) {
    if (tokens == NULL || tokens[0] == NULL) return;

    if (!background && !has_io_operator(tokens) &&
        handle_builtin(tokens)) {
        return;
    }

    if (background && !has_io_operator(tokens) &&
        (strcmp(tokens[0], "cd") == 0 ||
         strcmp(tokens[0], "exit") == 0 ||
         strcmp(tokens[0], "jobs") == 0 ||
         strcmp(tokens[0], "history") == 0 ||
         strcmp(tokens[0], "help") == 0)) {
        fprintf(stderr, "This built-in cannot run in the background.\n");
        return;
    }

    execute(tokens, background);
}

int main(void) {
    char* cmdline;
    char** arglist;

    using_history();
    rl_attempted_completion_function = shell_completion;

    while (1) {
        reap_jobs();

        cmdline = readline(PROMPT);

        if (cmdline == NULL) break;

        if (cmdline[0] != '\0') {
            add_history(cmdline);
        }

        if (cmdline[0] == '!') {
            char* endptr;
            errno = 0;
            long number = strtol(cmdline + 1, &endptr, 10);

            if (errno != 0 || endptr == cmdline + 1 ||
                *endptr != '\0' || number < 1 || number > INT_MAX) {
                fprintf(stderr, "Invalid history command.\n");
                free(cmdline);
                continue;
            }

            char* previous = get_history_command((int)number);

            if (previous == NULL) {
                fprintf(stderr, "History entry not found.\n");
                free(cmdline);
                continue;
            }

            free(cmdline);
            cmdline = previous;
            printf("%s\n", cmdline);
        }

        if (cmdline[0] != '\0') {
            store_command_history(cmdline);
        }

        arglist = tokenize(cmdline);

        if (arglist != NULL) {
            char* segment[MAXARGS + 1];
            int count = 0;
            int background = 0;

            for (int i = 0; ; i++) {
                if (arglist[i] == NULL ||
                    strcmp(arglist[i], ";") == 0 ||
                    strcmp(arglist[i], "&") == 0) {

                    if (arglist[i] != NULL &&
                        strcmp(arglist[i], "&") == 0) {
                        background = 1;
                    }

                    segment[count] = NULL;

                    if (count > 0) {
                        process_command(segment, background);
                    }

                    count = 0;
                    background = 0;

                    if (arglist[i] == NULL) break;
                }
                else {
                    if (count < MAXARGS) {
                        segment[count++] = arglist[i];
                    }
                }
            }

            free_arguments(arglist);
        }

        free(cmdline);
    }

    reap_jobs();
    printf("\nShell exited.\n");
    return 0;
}
