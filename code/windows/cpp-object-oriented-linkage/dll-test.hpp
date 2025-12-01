#pragma once

#ifdef DLLTEST_EXPORTS
#define DLLTEST_API __declspec(dllexport)
#else
#define DLLTEST_API __declspec(dllimport)
#endif

// In a CPP/HPP file, symbols export with C++ linkage (extern "C++") as opposed to C linkage by default
class DLLTEST_API MyClass {
public:
    void hello();
};