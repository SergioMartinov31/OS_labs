#include "../contracts.h"
#include <stdlib.h>

char* translation(long x) {
    char* buf = (char*)malloc(70 * sizeof(char)); // выделяем память под строку 70 потому что long обычно не больше 64 бит + запас//'\0'
    int i = 0; // щётчик цифп
    if (x == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return buf;
    }
    while (x > 0) {
        buf[i++] = (x % 2) + '0'; //'0' для перевода цифры в символ потому что в ASCII '0' = 48 а '1' = 49
        x /= 2; // делим на основание системы счисления
    }
    buf[i] = '\0';

    for (int j = 0; j < i / 2; j++) { // разворачиваем строку
        char tmp = buf[j]; // берём цифру с начала
        buf[j] = buf[i - 1 - j]; // берём цифру с конца
        buf[i - 1 - j] = tmp; // меняем их местами
    }
    return buf;
}