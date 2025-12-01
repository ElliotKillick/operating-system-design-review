#include <windows.h>

// Dynamically link to test DLL
__declspec(dllimport) void DummyExport();
__declspec(dllimport) void DummyExport2();

int main() {
    // Call dummy exports so our link to the DLLs are not optimized out
	DummyExport();
	DummyExport2();

    return EXIT_SUCCESS;
}
