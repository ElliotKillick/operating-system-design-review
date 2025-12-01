#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        puts("DllMain: DLL_PROCESS_ATTACH");
        break;
    case DLL_PROCESS_DETACH:
        puts("DllMain: DLL_PROCESS_DETACH");
		__debugbreak();
        break;
    }

    return TRUE;
}

void libraryAtexitRoutine(void) {
	puts("Running atexit routine of library!");
}

EXTERN_C __declspec(dllexport) void TestExport() {
	puts("Library test export: registering atexit routine");
	atexit(libraryAtexitRoutine);
}
