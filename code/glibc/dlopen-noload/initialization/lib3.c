#include <stdio.h>
#include <dlfcn.h>

// Differing external function names matters when dynamic linking due to ELF having a flat symbol namespace
__attribute__((constructor))
void init3() {
    puts("Start: Library 3 initialization");

    // Library 2 is loaded in memory, but it hasn't initialized yet
    // Test what happens when we try to get a handle to the uninitialized library
    //
    // Note that specifying one of RTLD_LAZY or RTLD_NOW is a requirement (otherwise, we get an "Invalid argument" error)
    // Specify RTLD_LAZY because, as indicated by the manual, specifying RTLD_NOW can promote the library to that binding (promotion can also occur with other flags) and we only want to test RTLD_NOLOAD
    void* lib = dlopen("lib2.so", RTLD_LAZY | RTLD_NOLOAD);

    puts("Still inside: Libary 3 initialization");

    if (!lib) {
        fprintf(stderr, "%s\n", dlerror());
        return;
    }

    printf("Library 3 got handle to Library 2: %p\n", lib);
}
