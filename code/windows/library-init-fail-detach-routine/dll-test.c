#include <windows.h>
#include <stdio.h>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        puts("DllMain: DLL_PROCESS_ATTACH");
        //__debugbreak();

		// DLL_PROCESS_ATTACH fails
		return FALSE;
        break;
    case DLL_PROCESS_DETACH:
        puts("DllMain: DLL_PROCESS_DETACH");
        //__debugbreak();
        break;
    }

    return TRUE;
}

EXTERN_C __declspec(dllexport) void DummyExport() {
    // Exported function that does nothing so we can dynamically link
    puts("Test export");
}
