#include <windows.h>
#include <stdio.h>

EXTERN_C __declspec(dllexport) void func() {
    puts("DLL 1, func");
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        puts("Start: DLL 1 Init");

        HMODULE lib2 = LoadLibrary(L"lib2.dll");
        if (!lib2) {
            __debugbreak();
        }

        puts("End: DLL 1 Init");
        break;
    }

    return TRUE;
}

EXTERN_C __declspec(dllexport) void DummyExport() {
    // Exported function that does nothing so we can dynamically link
    //puts("Test export");
}
