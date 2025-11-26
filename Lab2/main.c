#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h> 
#include <time.h>

typedef struct {
    double x;
    double y;
    double z;
} point;

typedef struct {
    point* points; // массив точек
    int n;         // количество точек
} data;

typedef struct {
    data* input_data;  // указатель на сткпутуру с точками
    int start; // диапазон индексов для обработки
    int end;
} thread_arg;

double max_area = 0.0;
pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER; // мьютекс для защиты доступа к max_area

// Генерация точек для теста
data* generate_points() {
    int n = 1000;
    point* points = malloc(sizeof(point) * n);
    if (!points) { fprintf(stderr, "Ошибка выделения памяти!\n"); exit(1); }

    for (int i = 0; i < n; i++) {
        points[i].x = (double)i;
        points[i].y = (double)(i % 20);
        points[i].z = (double)(i % 10);
    }

    data* result = malloc(sizeof(data));
    if (!result) { free(points); fprintf(stderr, "Ошибка выделения памяти!\n"); exit(1); }

    result->points = points;
    result->n = n;
    return result;
}

// Вычисление площади треугольника через векторное произведение
double triangle_area(point a, point b, point c) {
    double abx = b.x - a.x, aby = b.y - a.y, abz = b.z - a.z;
    double acx = c.x - a.x, acy = c.y - a.y, acz = c.z - a.z;

    double cx = aby * acz - abz * acy;
    double cy = abz * acx - abx * acz;
    double cz = abx * acy - aby * acx;

    return 0.5 * sqrt(cx*cx + cy*cy + cz*cz);
}

// Функция потока
void* calculation_area(void* arg) {
    thread_arg* t = (thread_arg*)arg; // приведение типа
    point* points = t->input_data->points;
    int n = t->input_data->n;
    double local_max = 0.0; // максимум площади в этом потоке

    for (int i = t->start; i < t->end; i++) {
        for (int j = i + 1; j < n; j++) {
            for (int k = j + 1; k < n; k++) {
                double area = triangle_area(points[i], points[j], points[k]);
                if (area > local_max) local_max = area;
            }
        }
    }

    pthread_mutex_lock(&result_mutex);
    if (local_max > max_area) max_area = local_max;
    pthread_mutex_unlock(&result_mutex);

    return NULL;
}

int main(int argc, char* argv[]) { // argc - кол-во аргументов, argv - массив аргументов( argv[0] - имя программы)
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <кол-во потоков>\n", argv[0]);
        return 1;
    }

    int num_threads = atoi(argv[1]); // преобразование строки в целое число
    if (num_threads <= 0) {
        fprintf(stderr, "Ошибка! Количество потоков должно быть положительным!\n");
        return 1;
    }

    data* input_data = generate_points();
    int n = input_data->n;

    pthread_t* threads = malloc(sizeof(pthread_t) * num_threads);
    thread_arg* args = malloc(sizeof(thread_arg) * num_threads);

    int step = n / num_threads; // количество точек на поток

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start); // сохраняем время начала работы программы

    for (int i = 0; i < num_threads; i++) {
        args[i].input_data = input_data;
        args[i].start = i * step;
        args[i].end = (i == num_threads - 1) ? n : (i + 1) * step;

        if (pthread_create(&threads[i], NULL, calculation_area, &args[i]) != 0) {
            fprintf(stderr, "Ошибка создания потока %d!\n", i);
            free(input_data->points);
            free(input_data);
            free(threads);
            free(args);
            return 1;
        }
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double t = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Максимальная площадь: %lf\n", max_area);
    printf("Время выполнения: %lf секунд\n", t);
    printf("Количество потоков: %d\n", num_threads);

    free(input_data->points);
    free(input_data);
    free(threads);
    free(args);

    return 0;
}
