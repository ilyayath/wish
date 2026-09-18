#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARGS 128

// каталоги, у яких шукаємо програми
char *shell_path[MAX_ARGS];
int shell_path_size = 0;

// шукаємо команду в усіх каталогах path
// повертає повний шлях (його треба звільнити) або NULL, якщо не знайшли
char *find_program(char *command)
{
    for (int i = 0; i < shell_path_size; i++) {
        // +2 це символ '/' і '\0'
        char *full = malloc(strlen(shell_path[i]) + strlen(command) + 2);

        strcpy(full, shell_path[i]);
        strcat(full, "/");
        strcat(full, command);

        if (access(full, X_OK) == 0) {
            return full;   // знайшли, і файл можна виконати
        }

        free(full);
    }
    return NULL;
}

int main(void)
{
    char *line = NULL;
    size_t line_size = 0;

    // на початку шукаємо програми тільки в /bin
    shell_path[0] = strdup("/bin");
    shell_path_size = 1;

    while (1) {
        printf("wish> ");
        fflush(stdout);

        if (getline(&line, &line_size, stdin) == -1) {
            break;   // кінець вводу (Ctrl+D)
        }

        // розбиваємо рядок на окремі слова
        char *args[MAX_ARGS];
        int args_count = 0;

        char *rest = line;
        char *token;

        while ((token = strsep(&rest, " \t\n")) != NULL) {
            if (strcmp(token, "") == 0) {
                continue;   // між двома пробілами strsep дає порожній токен
            }
            args[args_count] = token;
            args_count++;
        }
        args[args_count] = NULL;   // execv вимагає NULL в кінці

        if (args_count == 0) {
            continue;   // порожній рядок, просто Enter
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
            // ми в дочірньому процесі
            execv(program, args);
            // сюди потрапляємо тільки якщо execv не спрацював
            printf("execv не вдався\n");
            exit(1);
        } else {
            // ми в батьківському процесі
            waitpid(pid, NULL, 0);
            free(program);
        }
    }

    free(line);
    return 0;
}