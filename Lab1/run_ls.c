#include <unistd.h>


int main() {
    execlp("ls", "ls", NULL);
    return 0;
}
