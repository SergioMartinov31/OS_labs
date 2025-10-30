#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/mman.h> //Для mmap, munmap
#include <sys/stat.h> //Для флагов доступа к файлам: 0666 и типа файла
#include <fcntl.h> //Для shm_open

#define BUF_SIZE 1024

struct shmseg {
    char buf[BUF_SIZE];
    int ready;
};

int main() {
    const char *shm_name_data = "/shm_data"; //имена сегментов, которые мы откроем для чтения/записи.
    const char *shm_name_err  = "/shm_err";

    int fd_data = shm_open(shm_name_data, O_RDWR, 0666); //открыть уже сущ. сегменты(чтение/запись)
    int fd_err  = shm_open(shm_name_err,  O_RDWR, 0666);
    if (fd_data == -1 || fd_err == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    //отображаем сегменты в память дочернего процесса
    struct shmseg *shm_data = mmap(NULL, sizeof(struct shmseg), PROT_READ | PROT_WRITE, MAP_SHARED, fd_data, 0);
    char *shm_err = mmap(NULL, BUF_SIZE * 10, PROT_READ | PROT_WRITE, MAP_SHARED, fd_err, 0);
    if (shm_data == MAP_FAILED || shm_err == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Ждём, пока родитель не положит данные (ready=1).
    while (shm_data->ready == 0){
        usleep(1000);
    }
    // парент заночил передачу данных
    if (shm_data->ready == -1){
        exit(0);
    } 

    char filename[BUF_SIZE];
    strncpy(filename, shm_data->buf, BUF_SIZE); //копируем имя файла из разделяемой памяти
    shm_data->ready = 0; //готово к новым данным

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        snprintf(shm_err + strlen(shm_err), BUF_SIZE * 10 - strlen(shm_err),
                 "open: ошибка при открытии файла '%s'\n", filename);
        exit(EXIT_FAILURE);
    }

    // перенаправляем stdout в файл
    dup2(fd, STDOUT_FILENO);
    close(fd);

    while (1) {
        // Ждём, пока родитель не положит данные (ready=1).
        while (shm_data->ready == 0){
             usleep(1000);
        }
        // Проверяем на окончание передачи данных
        if (shm_data->ready == -1) break;

        char buf[BUF_SIZE];
        strncpy(buf, shm_data->buf, BUF_SIZE); //копируем строку из разделяемой памяти
        shm_data->ready = 0;

        if (isupper((unsigned char)buf[0])) {
            printf("%s\n", buf);
            fflush(stdout);
        } else {
            snprintf(shm_err + strlen(shm_err),
                     BUF_SIZE * 10 - strlen(shm_err),
                     "Error: строка должна начинаться с заглавной буквы - '%s'\n",
                     buf);
        }
    }

    munmap(shm_data, sizeof(struct shmseg)); //отсоединяем сегменты разделяемой памяти от дочернего процесса
    munmap(shm_err, BUF_SIZE * 10); 
    return 0;
}
