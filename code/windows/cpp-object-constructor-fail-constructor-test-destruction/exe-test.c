#include <windows.h>

// Dynamically link to test DLL
__declspec(dllimport) void DemoExport();

int main() {
    DemoExport();

    return EXIT_SUCCESS;
}
