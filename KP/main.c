// macOS помечает sem_init как deprecated, но POSIX-семафоры используются корректно.
// На Linux-системах предупреждения нет.

//каждый джоб выполняется в отдельном потоке, внутри потока запускается внешний процесс через system()
//Зависимости реализованы ожиданием завершения других джоб
//семафор нужен чтобы ограничить число одновременных джоб, использующих общий ресурc, например доступ к базе данных
//мьютексы используются для защиты счётчика запущенных джоб

#include <stdio.h>
#include <stdlib.h> //maloc, free, system(это функция для запуска каких-то внешних команд)
#include <string.h>
#include <unistd.h> //usleep - функция для приостановки выполнения на заданное количество микросекунд
#include <semaphore.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "cJSON.h"

#define MAX_JOBS 100

typedef struct Job {
    char name[64]; // имя джобы
    char cmd[256]; // команда для выполнения
    int dep_count; // кол-во зависимостей 
    struct Job* deps[MAX_JOBS]; //Зависимости реализованы ожиданием завершения других джоб
    sem_t* semaphore; // указатель на семафор
    int completed; // флаг завершения
    bool running; // для отслеживания выполнения
} Job;

typedef struct Semaphore { //именный семафор чтобы можно было ссылаться по имени
    char name[64]; // имя семафора
    sem_t sem; // сам семафор
} Semaphore;

Job jobs[MAX_JOBS]; //массив джобов
Semaphore semaphores[MAX_JOBS]; //массив семафоров
int job_count = 0; //счётчик джоб
int sem_count = 0; //счётчик семафоров
int max_parallel_jobs = 2; //максимальное число параллельных джоб(дефолтное значение)

pthread_mutex_t running_count_mutex = PTHREAD_MUTEX_INITIALIZER; //мьютекс для защиты счётчика запущенных джоб
int running_count = 0; //счётчик запущенных джоб
bool dag_error = false; //флаг ошибки в DAG

// Поиск семафора по имени
sem_t* find_semaphore(const char* name) { //возвращает указатель на семафор по имени чтобы все джобы могли ссылаться на один семафор
    for (int i = 0; i < sem_count; i++) {
        if (strcmp(semaphores[i].name, name) == 0) //сравниваем строки
            return &semaphores[i].sem;
    }
    return NULL;
}




// Проверка циклов в DAG (DFS)
bool has_cycle_util(Job* job, bool visited[], bool rec_stack[]) { // visitied - массив посещённых джоб, rec_stack - стек рекурсии
    int idx = job - jobs; //вычисляем индекс джобы в массиве
    if (!visited[idx]) {
        visited[idx] = true;
        rec_stack[idx] = true;

        for (int i = 0; i < job->dep_count; i++) {
            int dep_idx = job->deps[i] - jobs; //вычисляем индекс зависимости
            if (!visited[dep_idx] && has_cycle_util(job->deps[i], visited, rec_stack)) //если не посещена, рекурсивно вызываем
                return true;
            else if (rec_stack[dep_idx]) //если не посещена, но в стеке рекурсии, значит цикл
                return true;
        }
    }
    rec_stack[idx] = false;
    return false;
}
// обёртка для проверки циклов
bool has_cycle() {
    bool visited[MAX_JOBS] = {0}; //все джобы не посещены
    bool rec_stack[MAX_JOBS] = {0}; //нет джоб в стеке рекурсии
    for (int i = 0; i < job_count; i++)
        if (has_cycle_util(&jobs[i], visited, rec_stack))
            return true;
    return false;
}

// Выполнение джобы
void* run_job(void* arg) { //функция потока
    Job* job = (Job*)arg; //приводим аргумент к типу Job* так как до этого мы передали как void*(этого требует pthread_create)

    // Ждём завершения всех зависимостей
    for (int i = 0; i < job->dep_count; i++) { //джоб не может стартовать пока не завершатся все его зависимости
        while (!job->deps[i]->completed && !dag_error) usleep(1000); //зависимость не завершена,ошибок нет, ждем 1 мс(чтобы джоб не стартовал раньше зависимостей)
    }

    if (dag_error) return NULL;

    // Ограничение максимального числа параллельных джоб
    while (1) {
        pthread_mutex_lock(&running_count_mutex); // running_count - общий для всех потоков
        if (running_count < max_parallel_jobs) {
            running_count++;
            job->running = true;
            pthread_mutex_unlock(&running_count_mutex);
            break;
        }
        pthread_mutex_unlock(&running_count_mutex); // если слота нет то отдаём мьютекс и ждём(пусть пока другие джобы попытаются стартовать)
    }

    if (job->semaphore) sem_wait(job->semaphore); //если джоба использует семафор, ждём его

    printf("Запуск джобы: %s\n", job->name);
    int ret = system(job->cmd); // запускаем процесс, передаём команду для джобы(под капотом создаётся новый процесс через fork+exec); блокирует поток до завершения команды; процесс запущееный через system() завершается сам,  а system() ждёт его завершения и возвращает код выхода
    
    if (job->semaphore) sem_post(job->semaphore); // закончили работу и отдаём семафор

    job->completed = 1;
    job->running = false;

    pthread_mutex_lock(&running_count_mutex);
    running_count--;
    pthread_mutex_unlock(&running_count_mutex);

    if (ret != 0) {
        printf("Ошибка в джобе %s, прерывание DAG\n", job->name);
        dag_error = true;
    }

    return NULL;
}

// Чтение JSON и инициализация
void load_json(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) { perror("fopen"); exit(1); }

    fseek(f, 0, SEEK_END); // перемещаем указатель в конец файла
    long size = ftell(f); // получаем размер файла
    fseek(f, 0, SEEK_SET); // перемещаем указатель обратно в начало файла

    char* data = malloc(size + 1); // выделяем память для содержимого файла; +1 для нуль-терминатора '\0'
    fread(data, 1, size, f); // читаем файл в память
    fclose(f); // закрываем файл
    data[size] = '\0'; // добавляем нуль-терминатор в конец строки

    cJSON* json = cJSON_Parse(data);  // строит дерево из JSON-строки
    free(data); // освобождаем память после парсинга
    if (!json) { printf("Ошибка парсинга JSON\n"); exit(1); }

    cJSON* max_p = cJSON_GetObjectItem(json, "max_parallel_jobs"); // получаем максимальное число параллельных джоб
    if (max_p) max_parallel_jobs = max_p->valueint; // обновляем глобальную переменную

    // Семафоры
    cJSON* json_sems = cJSON_GetObjectItem(json, "semaphores"); //получаем массив семафоров из JSON
    cJSON* s; //временная переменная для итерации
    cJSON_ArrayForEach(s, json_sems) {
        strcpy(semaphores[sem_count].name, cJSON_GetObjectItem(s, "name")->valuestring); //копируем имя семафора
        int value = cJSON_GetObjectItem(s, "value")->valueint;
        sem_init(&semaphores[sem_count].sem, 0, value); //инициализируем семафор с заданным значением
        sem_count++;
    }

    // Джобы
    cJSON* json_jobs = cJSON_GetObjectItem(json, "jobs"); //получаем массив джоб из JSON
    cJSON* j; //временная переменная для итерации
    cJSON_ArrayForEach(j, json_jobs) { //итерация по всем джобам для инициализации структур
        strcpy(jobs[job_count].name, cJSON_GetObjectItem(j, "name")->valuestring);
        strcpy(jobs[job_count].cmd, cJSON_GetObjectItem(j, "cmd")->valuestring);
        char* sem_name = NULL;
        cJSON* js = cJSON_GetObjectItem(j, "semaphore"); //получаем имя семафора
        if (js && js->valuestring) sem_name = js->valuestring; // если семафор указан, сохраняем его имя
        jobs[job_count].semaphore = sem_name ? find_semaphore(sem_name) : NULL; //находим семафор по имени и сохраняем указатель
        jobs[job_count].dep_count = 0;
        jobs[job_count].completed = 0;
        jobs[job_count].running = false;
        job_count++;
    }

    // Зависимости
    int i = 0;
    cJSON_ArrayForEach(j, json_jobs) {
        cJSON* deps = cJSON_GetObjectItem(j, "depends_on"); //получаем массив зависимостей
        if (deps) {
            cJSON* d;
            cJSON_ArrayForEach(d, deps) {
                char* dep_name = d->valuestring;
                for (int k = 0; k < job_count; k++) {
                    if (strcmp(jobs[k].name, dep_name) == 0) {
                        jobs[i].deps[jobs[i].dep_count++] = &jobs[k];
                        break;
                    }
                }
            }
        }
        i++;
    }

    cJSON_Delete(json); // освобождаем память, занятую деревом JSON
}

int main() {
    load_json("dag.json");

    if (has_cycle()) {
        printf("Ошибка: DAG содержит цикл\n");
        return 1;
    }

    pthread_t threads[MAX_JOBS]; //массив потоков для джоб
    for (int i = 0; i < job_count; i++) { // для каждой джобы создаём поток
        pthread_create(&threads[i], NULL, run_job, &jobs[i]); //создаём поток для каждой джобы; передаём указатель на джобу как аргумент 
    }

    for (int i = 0; i < job_count; i++) { // ожидаем завершения всех потоков
        pthread_join(threads[i], NULL);
    }

    if (dag_error) {
        printf("DAG прерван из-за ошибки\n");
        return 1;
    }

    printf("DAG выполнен успешно\n");
    return 0;
}
