#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

int main() {
    char *buffer = NULL;
    size_t bufsize = 0;
    ssize_t len;

    // === читаем имя файла ===
    if ((len = getline(&buffer, &bufsize, stdin)) == -1) {
        const char *msg = "Не удалось прочитать имя файла\n";
        write(STDERR_FILENO, msg, strlen(msg));
        free(buffer);
        return EXIT_FAILURE;
    }
    buffer[strcspn(buffer, "\n")] = '\0';

    int fd = open(buffer, O_WRONLY | O_CREAT | O_TRUNC, 0644); // 0644 = rw-r--r--
    if (fd == -1) {
        perror("open");
        free(buffer);
        return EXIT_FAILURE;
    }

    // перенаправляем stdout в файл
    dup2(fd, STDOUT_FILENO);
    close(fd);

    // === читаем строки из stdin ===
    while ((len = getline(&buffer, &bufsize, stdin)) != -1) {
        buffer[strcspn(buffer, "\n")] = '\0';

        if (isupper((unsigned char)buffer[0])) {
            printf("%s\n", buffer);
            fflush(stdout);
        } else {
            dprintf(STDERR_FILENO,
                    "Error: строка должна начинаться с заглавной буквы - '%s'\n",
                    buffer);
        }
    }

    free(buffer);
    return EXIT_SUCCESS;
}
