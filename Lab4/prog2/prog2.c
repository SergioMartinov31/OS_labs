#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

typedef int (*prime_f)(int, int); // тип указателя на функцию PrimeCount
typedef char* (*translate_f)(long); // тип указателя на функцию translation

void* lib; // указатель на загруженную библиотеку
prime_f PrimeCount; // переменая-фиункция для PrimeCount
translate_f translation;

int load_lib(const char* path) { // функция загрузки библиотеки по пути
    if (lib) dlclose(lib);
    lib = dlopen(path, RTLD_LAZY); // загрузка библиотеки
    if (!lib) {
        printf("dlopen error: %s\n", dlerror());
        return 0;
    }

    PrimeCount = (prime_f)dlsym(lib, "PrimeCount"); //ищем PrimeCount в библиотеке
    translation = (translate_f)dlsym(lib, "translation");

    if (!PrimeCount || !translation) {
        printf("dlsym error\n");
        return 0;
    }

    printf("Loaded: %s\n", path);
    return 1;
}

int main() {
    printf("PROGRAM 2 (dynamic loading)\n");

    load_lib("../libs/impl1/libimpl1.so");

    int cmd;
    while (scanf("%d", &cmd) == 1) {
        if (cmd == 0) {
            static int cur = 1;
            cur = 3 - cur;
            if (cur == 1)
                load_lib("../libs/impl1/libimpl1.so");
            else
                load_lib("../libs/impl2/libimpl2.so");
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
            printf("%s\n", r);
            free(r);
        }
    }
}
