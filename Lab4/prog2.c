#define _GNU_SOURCE // отключаем расширенные возможности GNU для dlfcn.h
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h> // dlopen, dlsym, dlclose

typedef int (*prime_f)(int, int); // тип указателя на функцию подсчета простых чисел
typedef char* (*translate_f)(long);

void* lib = NULL; // указатель на загруженную библиотеку
prime_f PrimeCount = NULL; //указатель на функцию подсчета простых чисел
translate_f translation = NULL;

int load_lib(const char* path) {
    if (lib) dlclose(lib); // выгружаем предыдущую библиотеку
    
    lib = dlopen(path, RTLD_LAZY); // загружаем новую библиотеку RTLD_LAZY - отложенная загрузка символов
    if (!lib) {
        printf("Error loading library: %s\n", dlerror()); //dlerror - получение сообщения об ошибке
        return 0;
    }

    PrimeCount = (prime_f)dlsym(lib, "PrimeCount"); //dlsym - получение адреса функции из библиотеки
    translation = (translate_f)dlsym(lib, "translation");

    if (!PrimeCount || !translation) {
        printf("Error finding functions: %s\n", dlerror());
        return 0;
    }

    printf("Loaded library: %s\n", path);
    return 1;
}

int main() {
    printf("PROGRAM 2 (dynamic loading)\n");
    printf("Commands:\n");
    printf("  0       - switch library\n");
    printf("  1 A B   - count primes in [A, B]\n");
    printf("  2 X     - convert X\n");
    printf("  Ctrl+D to exit\n");

    // Начинаем с первой библиотеки
    if (!load_lib("./lib1/lib1.so")) {
        return 1;
    }

    int current_lib = 1; // текущая библиотека
    int cmd; // команда пользователя
    
    while (scanf("%d", &cmd) == 1) {
        if (cmd == 0) { 
            // Переключаем библиотеку
            current_lib = (current_lib == 1) ? 2 : 1;
            char lib_path[50];
            snprintf(lib_path, sizeof(lib_path), "./lib%d/lib%d.so", current_lib, current_lib);
            
            if (load_lib(lib_path)) {
                printf("Switched to lib%d\n", current_lib);
            }
        }
        else if (cmd == 1) {
            int A, B;
            scanf("%d %d", &A, &B);
            printf("Prime count = %d\n", PrimeCount(A, B));
        }
        else if (cmd == 2) {
            long x;
            scanf("%ld", &x);
            char* r = translation(x);
            printf("Result: %s\n", r);
            free(r);
        }
    }

    if (lib) dlclose(lib); // выгружаем библиотеку перед выходом
    return 0;
}