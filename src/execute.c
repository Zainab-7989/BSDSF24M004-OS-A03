
#include "shell.h"
#include <fcntl.h>

int execute(char* arglist[], int background) {
    char* commands[MAXARGS][MAXARGS + 1];
    char* input_files[MAXARGS];
    char* output_files[MAXARGS];

    int pipe_fds[MAXARGS][2];
    pid_t pids[MAXARGS];

    int command_count = 1;
    int arg_count = 0;

    for (int i = 0; i < MAXARGS; i++) {
        input_files[i] = NULL;
        output_files[i] = NULL;
        commands[i][0] = NULL;
    }

    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], "|") == 0) {
            if (arg_count == 0 || command_count >= MAXARGS) {
                fprintf(stderr, "Invalid pipe syntax.\n");
                return -1;
            }

            commands[command_count - 1][arg_count] = NULL;
            command_count++;
            arg_count = 0;
        }
        else if (strcmp(arglist[i], "<") == 0 ||
                 strcmp(arglist[i], ">") == 0) {
            int is_input = strcmp(arglist[i], "<") == 0;

            if (arglist[i + 1] == NULL ||
                strcmp(arglist[i + 1], "|") == 0 ||
                strcmp(arglist[i + 1], "<") == 0 ||
                strcmp(arglist[i + 1], ">") == 0) {
                fprintf(stderr, "Redirection filename missing.\n");
                return -1;
            }

            if (is_input) {
                if (input_files[command_count - 1] != NULL) {
                    fprintf(stderr, "Multiple input redirections.\n");
                    return -1;
                }
                input_files[command_count - 1] = arglist[i + 1];
            }
            else {
                if (output_files[command_count - 1] != NULL) {
                    fprintf(stderr, "Multiple output redirections.\n");
                    return -1;
                }
                output_files[command_count - 1] = arglist[i + 1];
            }

            i++;
        }
        else {
            if (arg_count >= MAXARGS) {
                fprintf(stderr, "Too many command arguments.\n");
                return -1;
            }

            commands[command_count - 1][arg_count++] = arglist[i];
        }
    }

    if (arg_count == 0) {
        fprintf(stderr, "Invalid command syntax.\n");
        return -1;
    }

    commands[command_count - 1][arg_count] = NULL;

    for (int i = 0; i < command_count - 1; i++) {
        if (pipe(pipe_fds[i]) == -1) {
            perror("pipe");

            for (int j = 0; j < i; j++) {
                close(pipe_fds[j][0]);
                close(pipe_fds[j][1]);
            }

            return -1;
        }
    }

    for (int i = 0; i < command_count; i++) {
        pids[i] = fork();

        if (pids[i] == -1) {
            perror("fork");

            for (int j = 0; j < command_count - 1; j++) {
                close(pipe_fds[j][0]);
                close(pipe_fds[j][1]);
            }

            for (int j = 0; j < i; j++) {
                waitpid(pids[j], NULL, 0);
            }

            return -1;
        }

        if (pids[i] == 0) {
            if (i > 0) {
                dup2(pipe_fds[i - 1][0], STDIN_FILENO);
            }

            if (i < command_count - 1) {
                dup2(pipe_fds[i][1], STDOUT_FILENO);
            }

            for (int j = 0; j < command_count - 1; j++) {
                close(pipe_fds[j][0]);
                close(pipe_fds[j][1]);
            }

            if (input_files[i] != NULL) {
                int fd = open(input_files[i], O_RDONLY);

                if (fd == -1) {
                    perror(input_files[i]);
                    exit(1);
                }

                if (dup2(fd, STDIN_FILENO) == -1) {
                    perror("dup2");
                    close(fd);
                    exit(1);
                }

                close(fd);
            }

            if (output_files[i] != NULL) {
                int fd = open(output_files[i],
                              O_WRONLY | O_CREAT | O_TRUNC,
                              0644);

                if (fd == -1) {
                    perror(output_files[i]);
                    exit(1);
                }

                if (dup2(fd, STDOUT_FILENO) == -1) {
                    perror("dup2");
                    close(fd);
                    exit(1);
                }

                close(fd);
            }

            execvp(commands[i][0], commands[i]);

            perror(commands[i][0]);
            exit(1);
        }
    }

    for (int i = 0; i < command_count - 1; i++) {
        close(pipe_fds[i][0]);
        close(pipe_fds[i][1]);
    }

    if (background) {
        for (int i = 0; i < command_count; i++) {
            add_job(pids[i]);
        }
    }
    else {
    int status = 0;
    int final_status = 0;

    for (int i = 0; i < command_count; i++) {
        if (waitpid(pids[i], &status, 0) == -1) {
            perror("waitpid");
            final_status = 1;
        }
        else if (i == command_count - 1) {
            if (WIFEXITED(status)) {
                final_status = WEXITSTATUS(status);
            }
            else {
                final_status = 1;
            }
        }
    }

    return final_status;
}

return 0;
}
