#include <windows.h>

// Dynamically link to test DLL
__declspec(dllimport) void TestExport();

void programAtexitRoutine(void) {
	puts("Running atexit routine of program!");
}

int main() {
	puts("Program main: registering atexit routine");
	atexit(programAtexitRoutine);
	
	TestExport();

    return EXIT_SUCCESS;
}
