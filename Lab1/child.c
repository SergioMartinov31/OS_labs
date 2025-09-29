#include <stdio.h> // стандартный ввод/вывод(printf(),scanf()) + работа с файлами(fopen(),fclose()) 
#include <unistd.h> //доступ к pipe, fork и всем командам для ОС 
#include <stdlib.h> //EXIT_FAILURE
#include <string.h> //для работы со стороками 
#include <ctype.h> //для работы с чарами, isupper


#define BUFSZ 256

int main() {
    char buffer[BUFSZ];

    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        const char *msg = "Не удалось прочитать имя файла\n";
        write(STDERR_FILENO, msg, strlen(msg));

        return 1;
    }
    buffer[strcspn(buffer, "\n")] = '\0';

    FILE *fp = fopen(buffer, "w");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';

        if (isupper((unsigned char)buffer[0])) {
            fprintf(fp, "%s\n", buffer);
            fflush(fp);
        } else {
            char err[BUFSZ];
            sprintf(err, "Error: строка должна начинаться с заглавной буквы - '%s'\n", buffer);

            write(STDOUT_FILENO, err, strlen(err));

        }
    }

    fclose(fp);
    return 0;
}
