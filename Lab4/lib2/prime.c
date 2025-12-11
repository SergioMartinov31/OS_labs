#include "../contracts.h"
#include <stdlib.h>

int PrimeCount(int A, int B) {
    int n = B + 1;
    char* sieve = calloc(n, 1);

    sieve[0] = sieve[1] = 1;

    for (int i = 2; i * i <= B; i++) {
        if (!sieve[i]) {
            for (int j = i * i; j <= B; j += i)
                sieve[j] = 1;
        }
    }

    int count = 0;
    for (int x = A; x <= B; x++)
        if (!sieve[x]) count++;

    free(sieve);
    return count;
}