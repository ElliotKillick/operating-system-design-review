#include <windows.h>
#include <stdio.h>
#include <lmcons.h> // For UNLEN

BOOL print_username() {
    // Buffer to store the username
    WCHAR username[UNLEN + 1]; // UNLEN is the maximum length of a username
    DWORD username_len = sizeof(username) / sizeof(username[0]); // Size of the buffer

    // Call GetUserNameW to retrieve the username
    // This causes advapi32.dll (public Windows DLL) to delay load sspicli.dll (priavte/internal Windows DLL)
    if (GetUserNameW(username, &username_len)) {
        wprintf(L"Username: %ls\n", username);
    }
    else {
        wprintf(L"Error retrieving username. Error code: %lu\n", GetLastError());
        return 1;
    }

    return 0;
}


int main() {
    print_username();

    // Output:
    // Username: user
    // (or your username)
}
