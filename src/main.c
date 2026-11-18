
#include "shell.h"


static int has_io_operator(char** arglist);
static char* trim_line(char* line) {
    while (*line == ' ' || *line == '\t') {
        line++;
    }

    size_t len = strlen(line);

    while (len > 0 &&
           (line[len - 1] == ' ' ||
            line[len - 1] == '\t' ||
            line[len - 1] == '\n' ||
            line[len - 1] == '\r')) {
        line[--len] = '\0';
    }

    return line;
}

static int run_if_command(char* line) {
    char** tokens = tokenize(line);

    if (tokens == NULL) {
        return 0;
    }

    int status;

    if (!has_io_operator(tokens) && handle_builtin(tokens)) {
        status = 0;
    }
    else {
        status = execute(tokens, 0);
    }

    for (int i = 0; tokens[i] != NULL; i++) {
        free(tokens[i]);
    }

    free(tokens);

    return status;
}

static void free_block(char** lines, int count) {
    for (int i = 0; i < count; i++) {
        free(lines[i]);
    }
}

static void handle_if_block(char* first_line) {
    char* condition = trim_line(first_line + 2);

    char* then_lines[MAXARGS];
    char* else_lines[MAXARGS];

    int then_count = 0;
    int else_count = 0;
    int in_else = 0;
    int valid = 0;

    char* line;

    while (1) {
        line = readline("if> ");

        if (line == NULL) {
            break;
        }

        char* trimmed = trim_line(line);

        if (strcmp(trimmed, "then") == 0) {
            free(line);
            break;
        }

        fprintf(stderr, "Expected 'then'.\n");
        free(line);
        free_block(then_lines, then_count);
        free_block(else_lines, else_count);
        return;
    }

    while (1) {
        line = readline("if> ");

        if (line == NULL) {
            break;
        }

        char* trimmed = trim_line(line);

        if (strcmp(trimmed, "else") == 0) {
            in_else = 1;
            free(line);
            continue;
        }

        if (strcmp(trimmed, "fi") == 0) {
            valid = 1;
            free(line);
            break;
        }

        if (trimmed[0] != '\0') {
            if (!in_else && then_count < MAXARGS) {
                then_lines[then_count++] = strdup(trimmed);
            }
            else if (in_else && else_count < MAXARGS) {
                else_lines[else_count++] = strdup(trimmed);
            }
            else {
                fprintf(stderr, "Too many commands in if block.\n");
                free(line);
                break;
            }
        }

        free(line);
    }

    if (!valid) {
        fprintf(stderr, "Missing 'fi'. If block cancelled.\n");
        free_block(then_lines, then_count);
        free_block(else_lines, else_count);
        return;
    }

    int status = run_if_command(condition);

    if (status == 0) {
        for (int i = 0; i < then_count; i++) {
            run_if_command(then_lines[i]);
        }
    }
    else {
        for (int i = 0; i < else_count; i++) {
            run_if_command(else_lines[i]);
        }
    }

    free_block(then_lines, then_count);
    free_block(else_lines, else_count);
}


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

        char* trimmed_cmd = trim_line(cmdline);

if (strncmp(trimmed_cmd, "if ", 3) == 0) {
    handle_if_block(trimmed_cmd);
    free(cmdline);
    continue;
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
