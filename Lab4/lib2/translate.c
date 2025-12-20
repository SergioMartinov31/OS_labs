#include "../contracts.h"
#include <stdlib.h>

char* translation(long x) {
    char* buf = (char*)malloc(70 * sizeof(char));
    int i = 0;
    if (x == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return buf;
    }
    while (x > 0) {
        buf[i++] = (x % 3) + '0';
        x /= 3;
    }
    buf[i] = '\0';

    for (int j = 0; j < i / 2; j++) {
        char tmp = buf[j];
        buf[j] = buf[i - 1 - j];
        buf[i - 1 - j] = tmp;
    }
    return buf;
}