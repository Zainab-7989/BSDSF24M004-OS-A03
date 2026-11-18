#include "shell.h"

static char* command_history[HISTORY_SIZE];
static int history_count = 0;

char** tokenize(char* cmdline) {
    if (cmdline == NULL || cmdline[0] == '\0') {
        return NULL;
    }

    char** arglist = calloc(MAXARGS + 1, sizeof(char*));

    if (arglist == NULL) {
        perror("calloc");
        return NULL;
    }

    for (int i = 0; i < MAXARGS + 1; i++) {
        arglist[i] = calloc(ARGLEN, sizeof(char));

        if (arglist[i] == NULL) {
            perror("calloc");

            for (int j = 0; j < i; j++) {
                free(arglist[j]);
            }

            free(arglist);
            return NULL;
        }
    }

    char* cp = cmdline;
    int argnum = 0;

    while (*cp != '\0') {
        while (*cp == ' ' || *cp == '\t') {
            cp++;
        }

        if (*cp == '\0') {
            break;
        }

        if (argnum >= MAXARGS) {
            fprintf(stderr, "Too many arguments.\n");
            break;
        }

        if (*cp == '<' || *cp == '>' || *cp == '|' ||
    *cp == ';' || *cp == '&')  {
            arglist[argnum][0] = *cp;
            arglist[argnum][1] = '\0';
            argnum++;
            cp++;
            continue;
        }

        int len = 0;
        char quote = '\0';

        while (*cp != '\0') {
            if (quote == '\0' &&
    (*cp == ' ' || *cp == '\t' ||
     *cp == '<' || *cp == '>' || *cp == '|' ||
     *cp == ';' || *cp == '&')) {
                break;
            }

            if ((*cp == '"' || *cp == '\'') && quote == '\0') {
                quote = *cp;
                cp++;
                continue;
            }

            if (*cp == quote && quote != '\0') {
                quote = '\0';
                cp++;
                continue;
            }

            if (len < ARGLEN - 1) {
                arglist[argnum][len++] = *cp;
            }

            cp++;
        }

        if (quote != '\0') {
            fprintf(stderr, "Unclosed quotation mark.\n");

            for (int i = 0; i < MAXARGS + 1; i++) {
                free(arglist[i]);
            }

            free(arglist);
            return NULL;
        }

        if (len > 0) {
            arglist[argnum][len] = '\0';
            argnum++;
        }
    }

    if (argnum == 0) {
        for (int i = 0; i < MAXARGS + 1; i++) {
            free(arglist[i]);
        }

        free(arglist);
        return NULL;
    }

    arglist[argnum] = NULL;
    return arglist;
}

void store_command_history(const char* command) {
    int index = history_count % HISTORY_SIZE;

    if (history_count >= HISTORY_SIZE) {
        free(command_history[index]);
    }

    command_history[index] = strdup(command);
    history_count++;
}

void print_history(void) {
    int first = history_count > HISTORY_SIZE
                ? history_count - HISTORY_SIZE : 0;

    for (int i = first; i < history_count; i++) {
        printf("%d %s\n", i + 1, command_history[i % HISTORY_SIZE]);
    }
}

char* get_history_command(int number) {
    int first = history_count > HISTORY_SIZE
                ? history_count - HISTORY_SIZE : 0;

    if (number < first + 1 || number > history_count) {
        return NULL;
    }

    return strdup(command_history[(number - 1) % HISTORY_SIZE]);
}

int handle_builtin(char** arglist) {
    if (strcmp(arglist[0], "exit") == 0) {
        printf("Shell exited.\n");
        exit(0);
    }

    if (strcmp(arglist[0], "cd") == 0) {
        if (arglist[1] == NULL) {
            fprintf(stderr, "cd: directory required\n");
        }
        else if (chdir(arglist[1]) != 0) {
            perror("cd");
        }
        return 1;
    }

    if (strcmp(arglist[0], "help") == 0) {
        printf("Built-in commands:\n");
        printf("  exit              Exit the shell\n");
        printf("  cd <directory>    Change current directory\n");
        printf("  help              Display this help message\n");
        printf("  jobs              Display job control status\n");
        printf("  history           Display command history\n");
        printf("  !n                Execute command number n\n");
        return 1;
    }

    if (strcmp(arglist[0], "jobs") == 0) {
    print_jobs();
    return 1;
}

    if (strcmp(arglist[0], "history") == 0) {
        print_history();
        return 1;
    }

    return 0;
}


static const char* builtin_commands[] = {
    "cd", "exit", "help", "jobs", "history", NULL
};

static char* command_generator(const char* text, int state) {
    static int builtin_index;
    static char* path_list;
    static char* path_save;
    static DIR* directory;
    static char* current_dir;

    if (state == 0) {
        builtin_index = 0;

        if (directory != NULL) {
            closedir(directory);
            directory = NULL;
        }

        free(path_list);
        path_list = getenv("PATH") ? strdup(getenv("PATH")) : strdup("");
        path_save = NULL;
        current_dir = NULL;
    }

    while (builtin_commands[builtin_index] != NULL) {
        const char* name = builtin_commands[builtin_index++];

        if (strncmp(name, text, strlen(text)) == 0) {
            return strdup(name);
        }
    }

    while (1) {
        if (directory == NULL) {
            char* path_entry = strtok_r(
                path_save == NULL ? path_list : NULL,
                ":",
                &path_save
            );

            if (path_entry == NULL) {
                free(path_list);
                path_list = NULL;
                return NULL;
            }

            current_dir = path_entry;
            directory = opendir(current_dir[0] == '\0' ? "." : current_dir);

            if (directory == NULL) {
                continue;
            }
        }

        struct dirent* entry;

        while ((entry = readdir(directory)) != NULL) {
            if (strncmp(entry->d_name, text, strlen(text)) != 0) {
                continue;
            }

            char full_path[PATH_MAX];

            snprintf(full_path, sizeof(full_path), "%s/%s",
                     current_dir, entry->d_name);

            if (access(full_path, X_OK) == 0) {
                return strdup(entry->d_name);
            }
        }

        closedir(directory);
        directory = NULL;
    }
}

char** shell_completion(const char* text, int start, int end) {
    (void)end;

    if (start == 0) {
        return rl_completion_matches(text, command_generator);
    }

    return rl_completion_matches(text, rl_filename_completion_function);
}

#define MAX_JOBS 100

static pid_t background_jobs[MAX_JOBS];
static int job_count = 0;

void add_job(pid_t pid) {
    if (job_count < MAX_JOBS) {
        background_jobs[job_count++] = pid;
        printf("[Background] PID: %d\n", pid);
    } else {
        fprintf(stderr, "Job list is full.\n");
    }
}

void reap_jobs(void) {
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < job_count; i++) {
            if (background_jobs[i] == pid) {
                printf("[Completed] PID: %d\n", pid);

                for (int j = i; j < job_count - 1; j++) {
                    background_jobs[j] = background_jobs[j + 1];
                }

                job_count--;
                break;
            }
        }
    }
}

void print_jobs(void) {
    reap_jobs();

    if (job_count == 0) {
        printf("No background jobs.\n");
        return;
    }

    printf("Background Jobs:\n");

    for (int i = 0; i < job_count; i++) {
        printf("PID: %d\n", background_jobs[i]);
    }
}
