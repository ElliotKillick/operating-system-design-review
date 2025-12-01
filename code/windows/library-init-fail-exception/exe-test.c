#include <windows.h>

// Dynamically link to test DLL
__declspec(dllimport) void DummyExport();

int main() {
    puts("Hello from main()!");

    // Call dummy export so our link to the DLL isn't optimized out
    DummyExport();

    return EXIT_SUCCESS;
}
