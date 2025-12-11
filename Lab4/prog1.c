#include <stdio.h>
#include <stdlib.h>
#include "contracts.h"

int main() {
    int cmd;
    printf("PROGRAM 1 (static linking with lib1)\n");
    printf("Commands:\n");
    printf("  1 A B  - count primes in [A, B]\n");
    printf("  2 X    - convert X to binary\n");
    printf("  Ctrl+D to exit\n");
    // scanf возраащет количество успешно прочитанных элементов
    while (scanf("%d", &cmd) == 1) {
        if (cmd == 1) {
            int A, B;
            scanf("%d %d", &A, &B);
            printf("Prime count = %d\n", PrimeCount(A, B));
        }
        else if (cmd == 2) {
            long x;
            scanf("%ld", &x);
            char* r = translation(x);
            printf("Binary: %s\n", r);
            free(r);
        }
    }

    return 0;
}