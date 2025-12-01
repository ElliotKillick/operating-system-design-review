#include <windows.h>
#include <stdio.h>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        puts("Start: Library 2 Init");

        //__debugbreak();

        puts("End: Library 2 Init");
        break;
    }

    return TRUE;
}

EXTERN_C __declspec(dllexport) void DummyExport2() {
    // Exported function that does nothing so we can dynamically link
    puts("Test export 2");
}

EXTERN_C __declspec(dllexport) void DllFunc() {
    puts("Library 2: DllFunc called");
}
