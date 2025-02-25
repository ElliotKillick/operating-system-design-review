#include <stdio.h>
#include <dlfcn.h>

__attribute__((constructor))
void init1_1() {
    puts("Start: Library 1, init1_1");

    // Dynamically load a library with a circular dependency on the currently initializing library
    void* lib = dlopen("lib2.so", RTLD_NOW);
    if (!lib) {
        fprintf(stderr, "%s\n", dlerror());
        return;
    }

    puts("End: Library 1, init1_1");
}

__attribute__((constructor))
void init1_2() {
    puts("Library 1, init1_2");
}

__attribute__((constructor))
void init1_3() {
    puts("Library 1, init1_3");
}


void func1_1() {
    puts("Library 1, func1_1");
}
