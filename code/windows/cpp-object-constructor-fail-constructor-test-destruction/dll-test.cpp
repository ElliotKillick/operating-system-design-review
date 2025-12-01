#include <windows.h>
#include <stdio.h>
#include <iostream>

struct A {
    A() {
        puts("A: C++ constructor");
        // C++ constructor fails (simulate the "new" keyword failing to allocate memory)
        throw std::bad_alloc();
    }

    void test() {
        puts("A: Test method");
    }

    ~A() {
        puts("A: C++ destructor");
    }
};

EXTERN_C __declspec(dllexport) void DemoExport() {
    // The destructor of an object that failed initialization will never run no matter the scope the it is created in
    try {
        A a;
    }
    catch (const std::bad_alloc& e) {
        std::cerr << "Memory allocation failed: " << e.what() << '\n';
    }
}
