#include <dlfcn.h>

int main() {
    dlopen("lib1.so", RTLD_NOW);
    dlopen("lib2.so", RTLD_NOW);
    dlopen("lib3.so", RTLD_NOW);
}
