#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define MAX_ARGS 128

char error_message[30] = "An error has occurred\n";

void print_error(void)
{
    write(STDERR_FILENO, error_message, strlen(error_message));
}

char *shell_path[MAX_ARGS];
int shell_path_size = 0;

char *find_program(char *command)
{
    for (int i = 0; i < shell_path_size; i++) {
        // +2 це '/' і '\0'
        char *full = malloc(strlen(shell_path[i]) + strlen(command) + 2);

        strcpy(full, shell_path[i]);
        strcat(full, "/");
        strcat(full, command);

        if (access(full, X_OK) == 0) {
            return full;
        }

        free(full);
    }
    return NULL;
}

int run_builtin(char **args, int args_count)
{
    if (strcmp(args[0], "exit") == 0) {
        if (args_count != 1) {
            print_error();
            return 1;
        }
        exit(0);
    }

    if (strcmp(args[0], "cd") == 0) {
        if (args_count != 2) {
            print_error();
            return 1;
        }
        if (chdir(args[1]) != 0) {
            print_error();
        }
        return 1;
    }

    if (strcmp(args[0], "path") == 0) {
        for (int i = 0; i < shell_path_size; i++) {
            free(shell_path[i]);
        }
        shell_path_size = 0;

        for (int i = 1; i < args_count; i++) {
            shell_path[shell_path_size] = strdup(args[i]);
            shell_path_size++;
        }
        return 1;
    }

    return 0;   // це не вбудована команда
}

int run_program(char **args, char *out_file)
{
    char *program = find_program(args[0]);
    if (program == NULL) {
        print_error();
        return -1;
    }

    int pid = fork();

    if (pid < 0) {
        print_error();
        free(program);
        return -1;
    }

    if (pid == 0) {
        if (out_file != NULL) {
            int fd = open(out_file, O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (fd < 0) {
                print_error();
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }
        execv(program, args);
        // сюди потрапляємо тільки якщо execv не спрацював
        print_error();
        exit(1);
    }

    free(program);
    return pid;
}

int main(int argc, char *argv[])
{
    FILE *input = stdin;
    int interactive = 1;

    if (argc == 2) {
        input = fopen(argv[1], "r");
        if (input == NULL) {
            print_error();
            exit(1);
        }
        interactive = 0;
    } else if (argc > 2) {
        print_error();
        exit(1);
    }

    char *line = NULL;
    size_t line_size = 0;

    shell_path[0] = strdup("/bin");
    shell_path_size = 1;

    while (1) {
        if (interactive) {
            printf("wish> ");
            fflush(stdout);
        }

        if (getline(&line, &line_size, input) == -1) {
            break;
        }

        int pids[MAX_ARGS];
        int pids_count = 0;

        char *cmd_rest = line;
        char *command;

        while ((command = strsep(&cmd_rest, "&")) != NULL) {
            char *args[MAX_ARGS];
            int args_count = 0;
            char *out_file = NULL;
            int have_redirect = 0;
            int bad_syntax = 0;

            char *rest = command;
            char *token;

            while ((token = strsep(&rest, " \t\n")) != NULL) {
                if (strcmp(token, "") == 0) {
                    continue;   // strsep дає порожні токени
                }

                if (strcmp(token, ">") == 0) {
                    if (have_redirect) {
                        bad_syntax = 1;
                    }
                    have_redirect = 1;
                    continue;
                }

                if (have_redirect) {
                    if (out_file != NULL) {
                        bad_syntax = 1;
                    }
                    out_file = token;
                } else {
                    args[args_count] = token;
                    args_count++;
                }
            }
            args[args_count] = NULL;   // execv вимагає NULL в кінці

            if (have_redirect && (out_file == NULL || args_count == 0)) {
                bad_syntax = 1;
            }

            if (bad_syntax) {
                print_error();
                continue;
            }

            if (args_count == 0) {
                continue;
            }

            if (run_builtin(args, args_count)) {
                continue;
            }

            int pid = run_program(args, out_file);
            if (pid > 0) {
                pids[pids_count] = pid;
                pids_count++;
            }
        }

        // спочатку запустили всі команди, і тільки тепер чекаємо кожну
        for (int i = 0; i < pids_count; i++) {
            waitpid(pids[i], NULL, 0);
        }
    }

    free(line);
    if (!interactive) {
        fclose(input);
    }
    return 0;
}