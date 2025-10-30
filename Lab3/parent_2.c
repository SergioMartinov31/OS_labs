#include <stdio.h> //Для printf, getline, fprintf — стандартный ввод/вывод
#include <unistd.h> //Для fork, usleep, execl и других системных вызовов Unix
#include <stdlib.h> //Для exit, free, NULL
#include <string.h> //Для strncpy, strlen, strcspn
#include <sys/mman.h> //Для mmap, munmap
#include <sys/stat.h> //Для флагов доступа к файлам: 0666 и типа файла
#include <fcntl.h>  //Для shm_open, ftruncate
#include <sys/wait.h> //Для wait
#include <ctype.h> // Для isupper

#define BUF_SIZE 1024

//Для mmap нужно заранее знать точный размер сегмента памяти, который мы хотим отобразить в процесс. Shared memory не может хранить обычные указатели на динамические массивы, потому что адреса в разных процессах разные. Поэтому для shm_data и shm_err мы используем фиксированный размер буфера. Это не идеальная динамика, но гарантирует корректную работу обмена данными между процессами.

struct shmseg {
    char buf[BUF_SIZE];
    int ready; // 0 - пусто, 1 - готово, -1 - конец
};

int main() {
    const char *shm_name_data = "/shm_data"; // имя сегмента разделяемой памяти для данных
    const char *shm_name_err  = "/shm_err"; // имя сегмента разделяемой памяти для ошибок

    // создаём сегменты памяти
    int fd_data = shm_open(shm_name_data, O_CREAT | O_RDWR, 0666); //создать и открыть(чтение/запись)
    int fd_err  = shm_open(shm_name_err,  O_CREAT | O_RDWR, 0666);
    if (fd_data == -1 || fd_err == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    ftruncate(fd_data, sizeof(struct shmseg)); //задаём размер сегмента для данных
    ftruncate(fd_err, BUF_SIZE * 10); //задаём размер сегмента для лога ошибок


    //MAP_SHARED - изменения видны другим процессам
    struct shmseg *shm_data = mmap(NULL, sizeof(struct shmseg), PROT_READ | PROT_WRITE, MAP_SHARED, fd_data, 0); //отображаем сегмент в память
    char *shm_err = mmap(NULL, BUF_SIZE * 10, PROT_READ | PROT_WRITE, MAP_SHARED, fd_err, 0);
    if (shm_data == MAP_FAILED || shm_err == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    shm_data->ready = 0;
    shm_err[0] = '\0'; //инициализируем лог ошибок пустой строкой

    pid_t pid = fork(); //создаём дочерний процесс
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // === Child process ===
        execl("./child_2", "./child_2", NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    } else {
        // === Parent process ===
        char *buffer = NULL;
        size_t bufsize = 0;
        ssize_t len;

        printf("Введите имя файла: ");
        if ((len = getline(&buffer, &bufsize, stdin)) == -1) {
            fprintf(stderr, "Ошибка чтения\n");
            free(buffer);
            exit(EXIT_FAILURE);
        }
        buffer[strcspn(buffer, "\n")] = '\0'; ////"Hello\n\0" -> "Hello\0"
        if (strlen(buffer) == 0) {
            fprintf(stderr, "Ошибка: имя файла не может быть пустым\n");
            free(buffer);
            exit(EXIT_FAILURE);
        }

        while (shm_data->ready != 0){ //ждём пока дочерний процесс прочитает предыдущие данные
             usleep(1000);
        }
        strncpy(shm_data->buf, buffer, BUF_SIZE); //добавляем имя файла в разделяемую память
        shm_data->ready = 1;

        while (1) {
            printf("Введите строку (пустая строка/Ctrl+D для выхода): ");
            if ((len = getline(&buffer, &bufsize, stdin)) == -1) break;
            buffer[strcspn(buffer, "\n")] = '\0';

            if (strlen(buffer) == 0) break;

            // отправка строки Child
            while (shm_data->ready != 0){ //ждём пока дочерний процесс прочитает предыдущие данные
                 usleep(1000);
            }
            strncpy(shm_data->buf, buffer, BUF_SIZE); //добавляем строку в разделяемую память
            shm_data->ready = 1;
        }

        while (shm_data->ready != 0){ 
             usleep(1000);
        }
        shm_data->ready = -1; //сообщаем Child что данных больше не будет

        wait(NULL); //удалить запись в таблице процессов, дочернего процеса

        if (strlen(shm_err) > 0)
            printf("\n=== Лог ошибок ===\n%s", shm_err);
        else
            printf("\nОшибок не обнаружено ✅\n");

        munmap(shm_data, sizeof(struct shmseg)); //отсоединяем сегменты разделяемой памяти от процесса
        munmap(shm_err, BUF_SIZE * 10);
        shm_unlink(shm_name_data); // удаляем сегменты разделяемой памяти из системы
        shm_unlink(shm_name_err);
        free(buffer); // освобождаем память буфера
    }
    return 0;
}
