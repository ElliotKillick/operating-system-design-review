#include <stdio.h>
#include <iostream>

extern "C"
__attribute__((constructor))
void hello1() {
    puts("Hello from Library 1!");

    // C++ constructor fails (simulate the "new" keyword failing to allocate memory)
    throw std::bad_alloc();
}

extern "C"
__attribute__((destructor))
void bye1() {
    puts("Goodbye from Library 1!");
}
