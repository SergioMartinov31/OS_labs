#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/mman.h> // Для mmap, munmap
#include <sys/stat.h> // Для флагов доступа к файлам: 0666 и типа файла
#include <fcntl.h>    // Для shm_open
#include <semaphore.h> // Для семафоров

#define BUF_SIZE 1024
#define LOG_SIZE 4096

struct shmseg {
    char buf[BUF_SIZE];
    sem_t sem_empty;
    sem_t sem_full;
};

struct shmerr {
    char log[LOG_SIZE];
    sem_t sem_err;
};

int main() {
    const char *shm_name_data = "/shm_data"; // имена сегментов, которые мы откроем для чтения/записи.
    const char *shm_name_err  = "/shm_err";

    int fd_data = shm_open(shm_name_data, O_RDWR, 0666); // открыть уже существующие сегменты
    int fd_err  = shm_open(shm_name_err,  O_RDWR, 0666);
    if (fd_data == -1 || fd_err == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // Отображаем сегменты в память дочернего процесса
    struct shmseg *shm_data = mmap(NULL, sizeof(struct shmseg),
                                   PROT_READ | PROT_WRITE, MAP_SHARED, fd_data, 0);
    struct shmerr *shm_err = mmap(NULL, sizeof(struct shmerr),
                                  PROT_READ | PROT_WRITE, MAP_SHARED, fd_err, 0);
    if (shm_data == MAP_FAILED || shm_err == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // === Чтение имени файла ===
    sem_wait(&shm_data->sem_full);              // ждём, пока Parent положит имя файла
    char filename[BUF_SIZE];
    strncpy(filename, shm_data->buf, BUF_SIZE); // копируем имя файла
    sem_post(&shm_data->sem_empty);             // Parent может писать новые данные

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644); // открываем/создаем файл
    if (fd == -1) {
        sem_wait(&shm_err->sem_err);
        snprintf(shm_err->log + strlen(shm_err->log),
                 LOG_SIZE - strlen(shm_err->log),
                 "open: ошибка при открытии файла '%s'\n", filename);
        sem_post(&shm_err->sem_err);
        exit(EXIT_FAILURE);
    }

    dup2(fd, STDOUT_FILENO); // перенаправляем stdout в файл
    close(fd);

    // === Чтение строк из Parent ===
    while (1) {
        sem_wait(&shm_data->sem_full); // ждём данные от Parent
        if (shm_data->buf[0] == '\0') { // конец передачи
            sem_post(&shm_data->sem_empty);
            break;
        }

        char buf[BUF_SIZE];
        strncpy(buf, shm_data->buf, BUF_SIZE);
        sem_post(&shm_data->sem_empty); // Parent может писать новые данные

        if (isupper((unsigned char)buf[0])) {
            printf("%s\n", buf);
            fflush(stdout);
        } else {
            sem_wait(&shm_err->sem_err);
            snprintf(shm_err->log + strlen(shm_err->log),
                     LOG_SIZE - strlen(shm_err->log),
                     "Error: строка должна начинаться с заглавной буквы - '%s'\n",
                     buf);
            sem_post(&shm_err->sem_err);
        }
    }

    munmap(shm_data, sizeof(struct shmseg)); // отсоединяем сегменты
    munmap(shm_err, sizeof(struct shmerr)); 
    return 0;
}
