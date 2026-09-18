#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ARGS 128

int main(void)
{
    char *line = NULL;
    size_t line_size = 0;

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
                continue;   // між двома пробілами strsep дає порожній токен
            }
            args[args_count] = token;
            args_count++;
        }
        args[args_count] = NULL;   // execv вимагає NULL в кінці

        for (int i = 0; i < args_count; i++) {
            printf("[%d] = %s\n", i, args[i]);
        }
    }

    free(line);
    return 0;
}