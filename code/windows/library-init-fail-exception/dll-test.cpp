#include <windows.h>
#include <stdio.h>
#include <iostream>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        puts("DllMain: DLL_PROCESS_ATTACH");

        // Similate C++ constructor failing (specifically, the "new" keyword fails to allocate memory)
        // On Windows, C++ exceptions are implemented using SEH
        throw std::bad_alloc();

/*
        // Ensure catch handler runs within the same module
        try {
            throw std::bad_alloc();
        }
        catch (const std::bad_alloc& e) {
            std::cerr << "Memory allocation failed: " << e.what() << '\n';
        }
*/

        return TRUE;
        break;
    case DLL_PROCESS_DETACH:
        puts("DllMain: DLL_PROCESS_DETACH");
        //__debugbreak();
        break;
    }

    return TRUE;
}

// NOTE:
// WinDbg appears to be weird with exceptions that have a catcher in another module (or maybe just at process startup between modules?).
// WinDbg will break then print the following when continuing from an exceptionsssss: "WARNING: Continuing a non-continuable exception"
// Then, WinDbg will continue code execution as if the exeception had never been raised - without running the code that is supposed to catch the exception. Thus, the loader will not be aware that the module failed initialization.
// This issue does not occur outside of WinDbg. The loader will catch the exception during module initialization, fail the module's initialization, then exit the process early because the module initialization failure occurred during process startup. This is the correct behavior.
// This issue does not occur when the exception is caught within the module scope of the module that raised the exception. The catch handler will run as it should in that case before code execution continues (although the aforementioned WinDbg message will still print).
// I played around with various "sxe" and other WinDbg commands but to no avail.
// Seems like a WinDbg bug to me.
// So, run this test outside of WinDbg to get the correct result.

EXTERN_C __declspec(dllexport) void DummyExport() {
    // Exported function that does nothing so we can dynamically link
    puts("Test export");
}
