#include "contracts.h"
#include <stdlib.h>

char* translation(long x) {
    char* buf = malloc(70);
    int i = 0;
    if (x == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return buf;
    }
    while (x > 0) {
        buf[i++] = (x % 2) + '0';
        x /= 2;
    }
    buf[i] = '\0';

    // reverse
    for (int j = 0; j < i / 2; j++) {
        char tmp = buf[j];
        buf[j] = buf[i - 1 - j];
        buf[i - 1 - j] = tmp;
    }
    return buf;
}
