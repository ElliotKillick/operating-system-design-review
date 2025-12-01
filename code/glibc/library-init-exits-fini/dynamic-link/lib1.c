#include <stdio.h>

__attribute__((constructor))
void hello1() {
    puts("Hello from Library 1!");
}

__attribute__((destructor))
void bye1() {
    puts("Goodbye from Library 1!");
}
