#include <windows.h>
#include <stdio.h>

HANDLE run_threads;

DWORD WINAPI loadlibrary_thread(LPVOID thread_started) {
    SetEvent(thread_started);
    WaitForSingleObject(run_threads, INFINITE);

    LoadLibrary(L"dll-test.dll");

    return 0;
}

DWORD WINAPI getmodulehandleex_thread(LPVOID thread_started) {
    SetEvent(thread_started);
    WaitForSingleObject(run_threads, INFINITE);

    Sleep(3000);
	
	// Get module handle
	// The GetModuleHandle family of functions allow us to retrieve a module handle to a partially loaded library using the public Windows API
    HMODULE module;
	BOOL ret = GetModuleHandleEx(0, L"dll-test.dll", &module);
	if (!ret)
		__debugbreak();
	
	// We got a handle to the partially loaded module (this is bad)!
	printf("Got handle to partially loaded module: %p\n", module);
	
	// Now, test if the public GetProcAddress function will give us back the address of an exported function to call in the still partially loaded library
    // Get function exported by DLL
	puts("Locating a function in library with GetProcAddress...");
	typedef void (*DllFunc_t)(void);
	DllFunc_t DllFunc = (DllFunc_t)GetProcAddress(module, "DllFunc");
	if (!DllFunc)
		__debugbreak();
	
	DllFunc();
	
	// Free the library (in our case, this just decrements the library's reference count after GetModuleHandleEx incremented it instead of actually unloading it from the process)
	FreeLibrary(module);

    return 0;
}

#define NUM_THREADS 2

int main() {
    // Create event for starting library loads
    run_threads = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!run_threads) {
        printf("Failed to create event: %lu\n", GetLastError());
        return EXIT_FAILURE;
    }

    // Create an event for signalling when each thread has started
    HANDLE thread_started_events[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; ++i) {
        thread_started_events[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!thread_started_events[i]) {
            printf("Failed to create event: %lu\n", GetLastError());
            return EXIT_FAILURE;
        }
    }

    // Create threads
    HANDLE threads[NUM_THREADS];
    PVOID routines[NUM_THREADS] = { loadlibrary_thread, getmodulehandleex_thread };
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads[i] = CreateThread(NULL, 0, routines[i], thread_started_events[i], 0, NULL);
        if (!threads[i]) {
            printf("Failed to create thread: %lu\n", GetLastError());
            return EXIT_FAILURE;
        }
    }

    // Wait for all threads to start
    DWORD result = WaitForMultipleObjects(NUM_THREADS, thread_started_events, TRUE, INFINITE);

    // Run any debugger commands for experimenting here
    //__debugbreak();

    // Let the threads run loose!
    if (result == WAIT_OBJECT_0)
        SetEvent(run_threads);

    // Join threads before application exits
    result = WaitForMultipleObjects(NUM_THREADS, threads, TRUE, INFINITE);
}
