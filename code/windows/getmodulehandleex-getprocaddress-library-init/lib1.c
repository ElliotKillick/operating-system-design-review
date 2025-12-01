#include <windows.h>
#include <stdio.h>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        puts("Start: Library 1 Init");

	    // Get module handle
		// The GetModuleHandle family of functions allow us to retrieve a module handle to a partially loaded library using the public Windows API
        HMODULE lib2;
	    BOOL ret = GetModuleHandleEx(0, L"lib2.dll", &lib2);
	    if (!ret)
	    	__debugbreak();

		// We got a handle to the partially loaded module (this is bad)!
		//
		// On the modern or legacy Windows loader, this can only happen on the same thread that is performing library loading because:
		// 1. At process startup, the Windows loader efficiently performs recursive mapping and snapping work on all the application's (EXE's) dependencies upfront, which means we can get a handle to those still uninitialized dependencies from our library in the same process
		//   - Getting a module handle in this case can be valid if the intended functionality was only to perform some given actions if the broader application depends on a module, or invalid if your code assumes the application is depending on a module that it might not be
		//   - We are testing this case
        // 2. An immediate circular dependency exists because then no initialization order can satisfy both libraries
	    printf("Got handle to partially loaded (uninitialized) library 2: %p\n", lib2);

	    // Now, test if the public GetProcAddress function will give us back the address of an exported function to call in the still partially loaded library
        // Get function exported by DLL
		puts("Locating a function in library 2 with GetProcAddress...");
	    typedef void (*DllFunc_t)(void);
	    DllFunc_t DllFunc = (DllFunc_t)GetProcAddress(lib2, "DllFunc");
	    if (!DllFunc)
	    	__debugbreak();

	    DllFunc();

        // Free the library (in our case, this just decrements the library's reference count after GetModuleHandleEx incremented it instead of actually unloading it from the process)
        FreeLibrary(lib2);

        puts("End: Library 1 Init");
        break;
    }

    return TRUE;
}

EXTERN_C __declspec(dllexport) void DummyExport() {
    // Exported function that does nothing so we can dynamically link
    puts("Test export 1");
}
