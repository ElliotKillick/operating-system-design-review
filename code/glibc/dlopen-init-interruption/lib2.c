#include <stdio.h>
#include <dlfcn.h>

extern void func1_1();

__attribute__((constructor))
void init2_1() {
    puts("Start: Library 2, init2_1");

    // The other constructors in library 1 still will not initialize before library 2 initialization even if we dynamically load library 1 explicitly
    //void* lib1 = dlopen("lib1.so", RTLD_NOW);
    //if (!lib1) {
    //    fprintf(stderr, "%s\n", dlerror());
    //    return;
    //}

    func1_1();

    puts("End: Library 2, init2_1");
}
