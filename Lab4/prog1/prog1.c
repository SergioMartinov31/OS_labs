#include <stdio.h>
#include <stdlib.h>
#include "contracts.h"

int main() {
    int cmd;
    printf("PROGRAM 1 (static linking with impl1)\n");

    while (scanf("%d", &cmd) == 1) { //чтение команды
        if (cmd == 1) {
            int A, B;
            scanf("%d %d", &A, &B);  //чтение границ
            printf("Prime count = %d\n", PrimeCount(A, B));
        }
        else if (cmd == 2) {
            long x;
            scanf("%ld", &x);
            char* r = translation(x); //вызов функции перевода - получаем строку
            printf("%s\n", r);
            free(r); //освобождение памяти для строки
        }
    }

    return 0;
}
