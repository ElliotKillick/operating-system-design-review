#include <windows.h>
#include <stdio.h>

// Dynamically link to func from lib1
__declspec(dllimport) void func();

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        puts("Start: DLL 2 Init");

        // Creating the circular dependency via dynamic linking or dynamic loading (or doubling down by doing both) gives the same result, which essentially means this LoadLibrary is a no-op because library 1 is already in its initialization phase (and code cannot arbitrarily be interrupted or preempted)
        //HMODULE lib1 = LoadLibrary(L"lib1.dll");
        //if (!lib1) {
        //    __debugbreak();
        //}

        func();

        puts("End: DLL 2 Init");
        break;
    }

    return TRUE;
}
