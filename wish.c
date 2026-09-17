#include <stdio.h>
#include <stdlib.h>

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

        printf("ти ввів: %s", line);

        free(line);
        return 0;
    }

    printf("hello\n");
    return 0;
}