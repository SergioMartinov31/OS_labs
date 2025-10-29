#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main() {
    execlp("ls", "ls", NULL);
    perror("execlp");
    return EXIT_FAILURE;
}
