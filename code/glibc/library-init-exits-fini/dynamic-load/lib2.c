#include <stdio.h>
#include <stdlib.h>

__attribute__((constructor))
void hello2_1() {
    puts("Hello from Library 2!");

    // Simulate an initialization failure hap
    // Oh no! Initialization failed because a malloc failed to allocate memory or something.
    // Unix constructors are not failable but let's try at least gracefully exiting instead of an abrupt abort.
    exit(EXIT_FAILURE);

    puts("Hello from Library 2 again!");
}

__attribute__((constructor))
void hello2_2() {
    puts("This is coming from a Library 2 constructor!");
}

__attribute__((destructor))
void bye2_1() {
    puts("Goodbye from Library 2!");
}

__attribute__((destructor))
void bye2_2() {
    puts("This is coming from a Library 2 destructor!");
}
