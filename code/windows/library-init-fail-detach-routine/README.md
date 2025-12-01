# The Affect of Library Initialization Failure on Whether Module Destruction Occurs

We test if the destructor of a Windows library that failed initialization will run.

## Result

```
DllMain: DLL_PROCESS_ATTACH
DllMain: DLL_PROCESS_DETACH
```

Yes, the destructor of a library that failed initialization will run. This behavior is incongruent to how every programming language and foreseeably system that implements failable constructors.

The Windows library loader will run the library's `DLL_PROCESS_DETACH` directly after its `DLL_PROCESS_ATTACH` if the latter fails:

```
dll_test!DllMain+0x37:
00007ff8`76001037 cc              int     3
0:000> k
 # Child-SP          RetAddr               Call Site
00 00000023`ecdaf070 00007ff8`760012f3     dll_test!DllMain+0x37 [C:\Users\user\Documents\QubesIncoming\lab-projects\library-init-fail-detach-routine\dll-test.c @ 9] 
01 00000023`ecdaf0b0 00007ff8`7b8e9a1d     dll_test!dllmain_dispatch+0x8f [d:\a01\_work\20\s\src\vctools\crt\vcstartup\src\startup\dll_dllmain.cpp @ 281] 
02 00000023`ecdaf110 00007ff8`7b93d2f7     ntdll!LdrpCallInitRoutine+0x61
03 00000023`ecdaf180 00007ff8`7b93d08a     ntdll!LdrpInitializeNode+0x1d3
04 00000023`ecdaf2d0 00007ff8`7b93d110     ntdll!LdrpInitializeGraphRecurse+0x42
05 00000023`ecdaf310 00007ff8`7b9a3cb2     ntdll!LdrpInitializeGraphRecurse+0xc8
06 00000023`ecdaf350 00007ff8`7b945deb     ntdll!LdrpInitializeProcess+0x1f62
07 00000023`ecdaf770 00007ff8`7b945c73     ntdll!LdrpInitialize+0x15f
08 00000023`ecdaf810 00007ff8`7b945c1e     ntdll!LdrpInitialize+0x3b
09 00000023`ecdaf840 00000000`00000000     ntdll!LdrInitializeThunk+0xe
0:000> g
(1878.1684): Break instruction exception - code 80000003 (first chance)
dll_test!DllMain+0x4b:
00007ff8`7600104b cc              int     3
0:000> k
 # Child-SP          RetAddr               Call Site
00 00000023`ecdaf070 00007ff8`7600130f     dll_test!DllMain+0x4b [C:\Users\user\Documents\QubesIncoming\lab-projects\library-init-fail-detach-routine\dll-test.c @ 15] 
01 00000023`ecdaf0b0 00007ff8`7b8e9a1d     dll_test!dllmain_dispatch+0xab [d:\a01\_work\20\s\src\vctools\crt\vcstartup\src\startup\dll_dllmain.cpp @ 287] 
02 00000023`ecdaf110 00007ff8`7b93d2f7     ntdll!LdrpCallInitRoutine+0x61
03 00000023`ecdaf180 00007ff8`7b93d08a     ntdll!LdrpInitializeNode+0x1d3
04 00000023`ecdaf2d0 00007ff8`7b93d110     ntdll!LdrpInitializeGraphRecurse+0x42
05 00000023`ecdaf310 00007ff8`7b9a3cb2     ntdll!LdrpInitializeGraphRecurse+0xc8
06 00000023`ecdaf350 00007ff8`7b945deb     ntdll!LdrpInitializeProcess+0x1f62
07 00000023`ecdaf770 00007ff8`7b945c73     ntdll!LdrpInitialize+0x15f
08 00000023`ecdaf810 00007ff8`7b945c1e     ntdll!LdrpInitialize+0x3b
09 00000023`ecdaf840 00000000`00000000     ntdll!LdrInitializeThunk+0xe
```

## Hypothetical: The Opposite Result

If Windows implemented the library loader's reponse to a failing library initialization correctly, then the below code sample demonstrates what doing properly programmed resource initialization with `DllMain` could look like.

For handling an error mid-initialization with multiple resource allocations, it would be acceptable to back out by doing some centeralized cleanup with `goto`, freeing only the prior successful resource allocations in reverse order before returning a failed module initialization:

```c
// WARNING: Hypothical code. Not correct for practical use.

LPVOID alloc1;
LPVOID alloc2;
LPVOID alloc3;

BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        alloc1 = HeapAlloc(GetProcessHeap(), 0, 1024);
        if (alloc1 == NULL) return FALSE;
        alloc2 = HeapAlloc(GetProcessHeap(), 0, 1024);
        if (alloc2 == NULL) goto cleanup_and_fail_1;
        alloc3 = HeapAlloc(GetProcessHeap(), 0, 1024);
        if (alloc3 == NULL) goto cleanup_and_fail_2;
        return TRUE;

        // Cleanup and fail early
        cleanup_and_fail_2:
            HeapFree(GetProcessHeap(), 0, alloc2);
        cleanup_and_fail_1:
            HeapFree(GetProcessHeap(), 0, alloc1);
        return FALSE;

        break;
    case DLL_PROCESS_DETACH:
        HeapFree(GetProcessHeap(), 0, alloc1);
        HeapFree(GetProcessHeap(), 0, alloc2);
        HeapFree(GetProcessHeap(), 0, alloc3);
        break;
    }

    return TRUE;
}
```
