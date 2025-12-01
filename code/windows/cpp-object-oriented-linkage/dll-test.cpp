#include <iostream>

#define DLLTEST_EXPORTS
#include "dll-test.hpp"

void MyClass::hello() {
    std::cout << "An object-oriented hello from DLL!\n";
}