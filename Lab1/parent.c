#include <stdio.h> // стандартный ввод/вывод(printf(),scanf()) + работа с файлами(fopen(),fclose()) 
#include <unistd.h> //доступ к pipe, fork и всем командам для ОС 
#include <stdlib.h> //EXIT_FAILURE
#include <string.h> //для работы со стороками 
#include <sys/types.h> //типы данных для системных вызовов типо size_t - беззнаковый размер данных; pid_t - идентификатор процесса 
#include <sys/wait.h> //работу с завершением процессов. #include <errno.h> //переменную errno для диагностики ошибок системных вызово

#define BUFSZ 256

int main() {
    int pipe1[2], pipe2[2];
    pid_t child_pid;

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    child_pid = fork();
    if (child_pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0) {
        // === Child process ===
        close(pipe1[1]);  // закрываем конец pipe1 для записи
        close(pipe2[0]);  // закрываем конец pipe2 для чтения

        dup2(pipe1[0], STDIN_FILENO);   // stdin = pipe1
        dup2(pipe2[1], STDOUT_FILENO);  // stdout = pipe2

        close(pipe1[0]); // по сути убираем дубликаты ссылок поскольку теперь stdin и stdout указывают туда же
        close(pipe2[1]);

        execl("./child", "./child", NULL);
        perror("execl"); //никогда не выполнится если execl успешен
        exit(EXIT_FAILURE);
    } else {
        // === Parent process ===
        close(pipe1[0]);  // закрываем конец pipe1 для чтения
        close(pipe2[1]);  // закрываем конец pipe2 для записи

        char buffer[BUFSZ];
        char errbuf[BUFSZ];
        ssize_t r;

        // Ввод имени файла
        printf("Введите имя файла: ");
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            fprintf(stderr, "Ошибка чтения\n");
            exit(EXIT_FAILURE);
        }
        buffer[strcspn(buffer, "\n")] = '\0'; //"Hello\n\0" -> "Hello\0"

        // Отправляем имя файла Child
        write(pipe1[1], buffer, strlen(buffer));
        write(pipe1[1], "\n", 1);  // обязательно \n тк fgets в Child читает до /n

        while (1) {
            printf("Введите строку (пустая строка/Ctrl+D для выхода): ");
            if (fgets(buffer, BUFSZ, stdin) == NULL) break; // не надо чистить буфер так как fgets сразу берёт и затирает его
            buffer[strcspn(buffer, "\n")] = '\0';

            if (strlen(buffer) == 0) break;

            // отправка строки Child
            write(pipe1[1], buffer, strlen(buffer));
            write(pipe1[1], "\n", 1);

        }



        close(pipe1[1]); //чтобы в Child fgets не ждал бесконечно новых данных из pipe 

        while ((r = read(pipe2[0], errbuf, sizeof(errbuf) - 1)) > 0){  //р читает из pipe и пишет в r - кол-во байтов прочитаных, 0 - конец файла(EOF), -1 -ошибка чтения
            errbuf[r] = '\0';
            printf("%s", errbuf);
        }  

        close(pipe2[0]);
        wait(NULL);
    }

    return 0;
}
