#include "../contracts.h"
#include <math.h>

int PrimeCount(int A, int B) {
    int count = 0;
    for (int n = A; n <= B; n++) {
        if (n < 2) continue;
        int isPrime = 1;
        for (int d = 2; d * d <= n; d++) {
            if (n % d == 0) {
                isPrime = 0;
                break;
            }
        }
        if (isPrime) count++;
    }
    return count;
}