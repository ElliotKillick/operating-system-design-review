#include <windows.h>
#include <stdio.h>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        puts("DllMain: DLL_PROCESS_ATTACH: Start");
		// Suspend the thread here in WinDbg to test the concurrent scenario (~n) or we can sleep
		// The module is partially loaded right now; specifically, it is partially initialized.
		// We test the partially initialized concurrent scenario, but the same also applies to other partially loaded states like if the module is still undergoing mapping and snapping work.
		Sleep(INFINITE);
		puts("DllMain: DLL_PROCESS_ATTACH: Done");
        break;
    }

    return TRUE;
}

EXTERN_C __declspec(dllexport) void DllFunc() {
    puts("DllFunc: Library function called!");
}
