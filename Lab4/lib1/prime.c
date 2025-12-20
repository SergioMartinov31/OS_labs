#include "../contracts.h"
#include <math.h>

int PrimeCount(int A, int B) {
    int count = 0;
    for (int n = A; n <= B; n++) {
        if (n < 2) continue;
        int isPrime = 1; // предполагаем, что число простое
        for (int d = 2; d * d <= n; d++) { //смотрим делители до sqrt(n)
            if (n % d == 0) { // нашли делитель
                isPrime = 0;
                break;
            }
        }
        if (isPrime) count++;
    }
    return count;
}