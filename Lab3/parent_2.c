#include <stdio.h> // Для printf, getline, fprintf — стандартный ввод/вывод
#include <unistd.h> // Для fork, execl и других системных вызовов Unix
#include <stdlib.h> // Для exit, free, NULL
#include <string.h> // Для strncpy, strlen, strcspn
#include <sys/mman.h> // Для mmap, munmap
#include <sys/stat.h> // Для флагов доступа к файлам: 0666 и типа файла
#include <fcntl.h>  // Для shm_open, ftruncate
#include <sys/wait.h> // Для wait
#include <ctype.h> // Для isupper
#include <semaphore.h> // Для семафоров

#define BUF_SIZE 1024
#define LOG_SIZE 4096

// Для mmap нужно заранее знать точный размер сегмента памяти, который мы хотим отобразить в процесс.
// Shared memory не может хранить обычные указатели на динамические массивы, потому что адреса в разных процессах разные.
// Поэтому для shm_data и shm_err мы используем фиксированный размер буфера.
// Семафоры добавляют синхронизацию без busy-wait.
struct shmseg {
    char buf[BUF_SIZE];
    sem_t sem_empty; // Parent может писать в buf, когда семафор >0
    sem_t sem_full;  // Child может читать из buf, когда семафор >0
};

// структура для логов ошибок
struct shmerr {
    char log[LOG_SIZE];
    sem_t sem_err; // семафор для доступа к логам
};

int main() {
    const char *shm_name_data = "/shm_data"; // имя сегмента разделяемой памяти для данных
    const char *shm_name_err  = "/shm_err";  // имя сегмента разделяемой памяти для ошибок

    // создаём сегменты памяти
    int fd_data = shm_open(shm_name_data, O_CREAT | O_RDWR, 0666); // создать и открыть (чтение/запись)
    int fd_err  = shm_open(shm_name_err,  O_CREAT | O_RDWR, 0666);
    if (fd_data == -1 || fd_err == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    ftruncate(fd_data, sizeof(struct shmseg)); // задаём размер сегмента для данных
    ftruncate(fd_err, sizeof(struct shmerr));  // задаём размер сегмента для лога ошибок

    // MAP_SHARED - изменения видны другим процессам
    struct shmseg *shm_data = mmap(NULL, sizeof(struct shmseg),
                                   PROT_READ | PROT_WRITE, MAP_SHARED, fd_data, 0); // отображаем сегмент в память
    struct shmerr *shm_err = mmap(NULL, sizeof(struct shmerr),
                                  PROT_READ | PROT_WRITE, MAP_SHARED, fd_err, 0);
    if (shm_data == MAP_FAILED || shm_err == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Инициализация семафоров (_, 1, _ ) - 1 - общий семафор между процессами
    if (sem_init(&shm_data->sem_empty, 1, 1) == -1){
        perror("sem_init");
        exit(EXIT_FAILURE);
    }; // буфер пуст, Parent может писать
    if (sem_init(&shm_data->sem_full, 1, 0) == -1){
        perror("sem_init");
        exit(EXIT_FAILURE);
    }  // Child ждёт данных
    if (sem_init(&shm_err->sem_err, 1, 1) == -1){
        perror("sem_init");
        exit(EXIT_FAILURE);
    }    // лог свободен


    shm_err->log[0] = '\0'; // инициализируем лог ошибок пустой строкой

    pid_t pid = fork(); // создаём дочерний процесс
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // === Child process ===
        execl("./child_2", "./child_2", NULL);
        perror("execl"); // execl вернётся только при ошибке
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
        buffer[strcspn(buffer, "\n")] = '\0'; // убираем \n
        if (strlen(buffer) == 0) {
            fprintf(stderr, "Ошибка: имя файла не может быть пустым\n");
            free(buffer);
            exit(EXIT_FAILURE);
        }

        // Отправка имени файла Child с синхронизацией через семафор
        if (sem_wait(&shm_data->sem_empty) == -1) { // ждём, пока буфер пуст, уменьшаем sem_empty
            perror("sem_wait (sem_empty)");
            exit(EXIT_FAILURE);
        }
        if (strncpy(shm_data->buf, buffer, BUF_SIZE) == NULL) { // передаём имя файла в буфер (возвращает  указатель на shm_data->buf)
            fprintf(stderr, "strncpy failed\n");
            exit(EXIT_FAILURE);
        }
        if (sem_post(&shm_data->sem_full) == -1) { // даём сигнал Child, что данные готовы
            perror("sem_post (sem_full)");
            exit(EXIT_FAILURE);
}

        while (1) {
            printf("Введите строку (пустая строка/Ctrl+D для выхода): ");
            if ((len = getline(&buffer, &bufsize, stdin)) == -1) break;
            buffer[strcspn(buffer, "\n")] = '\0'; // убираем \n
            if (strlen(buffer) == 0) break;       // пустая строка — сигнал окончания ввода

            sem_wait(&shm_data->sem_empty);      // ждём, пока буфер пуст, Parent может писать
            strncpy(shm_data->buf, buffer, BUF_SIZE); // копируем строку в разделяемую память
            sem_post(&shm_data->sem_full);       // Child может читать
        }

        // Сигнал окончания данных
        sem_wait(&shm_data->sem_empty);
        shm_data->buf[0] = '\0'; // пустая строка как маркер конца
        sem_post(&shm_data->sem_full); //Child может читать

        wait(NULL); // ждем завершения дочернего процесса

        // Вывод логов ошибок
        if (strlen(shm_err->log) > 0)
            printf("\n=== Лог ошибок ===\n%s", shm_err->log);
        else
            printf("\nОшибок не обнаружено ✅\n");

        // Отсоединяем сегменты разделяемой памяти
        munmap(shm_data, sizeof(struct shmseg));
        munmap(shm_err, sizeof(struct shmerr));
        shm_unlink(shm_name_data); // удаляем сегменты из системы
        shm_unlink(shm_name_err);
        free(buffer); // освобождаем память буфера
    }
    return 0;
}
