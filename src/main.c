#include "shell.h"

int main(void) {
    char* cmdline;
    char** arglist;

    using_history();
    rl_attempted_completion_function = shell_completion;

    while ((cmdline = readline(PROMPT)) != NULL) {

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

        if ((arglist = tokenize(cmdline)) != NULL) {
            if (!handle_builtin(arglist)) {
                execute(arglist);
            }

            for (int i = 0; arglist[i] != NULL; i++) {
                free(arglist[i]);
            }

            free(arglist);
        }

        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
