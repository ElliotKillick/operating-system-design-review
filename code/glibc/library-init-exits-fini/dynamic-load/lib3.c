#include <stdio.h>

__attribute__((constructor))
void hello3() {
    puts("Hello from Library 3!");
}

__attribute__((destructor))
void bye3() {
    puts("Goodbye from Library 3!");
}
