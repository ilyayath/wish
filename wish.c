#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARGS 128

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

int main(void)
{
    char *line = NULL;
    size_t line_size = 0;

    shell_path[0] = strdup("/bin");
    shell_path_size = 1;

    while (1) {
        printf("wish> ");
        fflush(stdout);

        if (getline(&line, &line_size, stdin) == -1) {
            break;
        }

        char *args[MAX_ARGS];
        int args_count = 0;

        char *rest = line;
        char *token;

        while ((token = strsep(&rest, " \t\n")) != NULL) {
            if (strcmp(token, "") == 0) {
                continue;   // strsep дає порожні токени
            }
            args[args_count] = token;
            args_count++;
        }
        args[args_count] = NULL;   // execv вимагає NULL в кінці

        if (args_count == 0) {
            continue;
        }

        if (strcmp(args[0], "exit") == 0) {
            if (args_count != 1) {
                printf("exit не приймає аргументів\n");
                continue;
            }
            exit(0);
        }

        if (strcmp(args[0], "cd") == 0) {
            if (args_count != 2) {
                printf("cd приймає рівно один аргумент\n");
                continue;
            }
            if (chdir(args[1]) != 0) {
                printf("немає такого каталогу\n");
            }
            continue;
        }

        char *program = find_program(args[0]);
        if (program == NULL) {
            printf("команду не знайдено\n");
            continue;
        }

        int pid = fork();

        if (pid < 0) {
            printf("fork не вдався\n");
        } else if (pid == 0) {
            execv(program, args);
            // сюди потрапляємо тільки якщо execv не спрацював
            printf("execv не вдався\n");
            exit(1);
        } else {
            waitpid(pid, NULL, 0);
            free(program);
        }
    }

    free(line);
    return 0;
}