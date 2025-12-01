# `GetModuleHandleEx` + `GetProcAddress` Library Initialization Experiment

We demonstrate the ability `GetProcAddress` has to perform module initializaiton.

## Result

```
Start: Library 1 Init
Got handle to partially loaded (uninitialized) library 2: 00007FFAC2530000
Locating a function in library 2 with GetProcAddress...
Start: Library 2 Init
End: Library 2 Init
Library 2: DllFunc called
End: Library 1 Init
Test export 1
Test export 2
```

`GetProcAddress` library initialization call stack:

```
 # Child-SP          RetAddr               Call Site
00 000000b6`0657e720 00007ffa`c2b21303     lib2!DllMain+0x30 [C:\Users\user\Documents\getmodulehandleex-getprocaddress-library-init\lib2.c @ 9] 
01 000000b6`0657e760 00007ffa`cf529a1d     lib2!dllmain_dispatch+0x8f [d:\a01\_work\20\s\src\vctools\crt\vcstartup\src\startup\dll_dllmain.cpp @ 281] 
02 000000b6`0657e7c0 00007ffa`cf57d2f7     ntdll!LdrpCallInitRoutine+0x61
03 000000b6`0657e830 00007ffa`cf57d08a     ntdll!LdrpInitializeNode+0x1d3
04 000000b6`0657e980 00007ffa`cf54010d     ntdll!LdrpInitializeGraphRecurse+0x42
05 000000b6`0657e9c0 00007ffa`cd297e3c     ntdll!LdrGetProcedureAddressForCaller+0x34d
06 000000b6`0657eb20 00007ffa`c7311083     KERNELBASE!GetProcAddressForCaller+0x6c
07 000000b6`0657eb70 00007ffa`c7311413     lib1!DllMain+0x83 [C:\Users\user\Documents\getmodulehandleex-getprocaddress-library-init\lib1.c @ 30] 
08 000000b6`0657ebc0 00007ffa`cf529a1d     lib1!dllmain_dispatch+0x8f [d:\a01\_work\20\s\src\vctools\crt\vcstartup\src\startup\dll_dllmain.cpp @ 281] 
09 000000b6`0657ec20 00007ffa`cf57d2f7     ntdll!LdrpCallInitRoutine+0x61
0a 000000b6`0657ec90 00007ffa`cf57d08a     ntdll!LdrpInitializeNode+0x1d3
0b 000000b6`0657ede0 00007ffa`cf57d110     ntdll!LdrpInitializeGraphRecurse+0x42
0c 000000b6`0657ee20 00007ffa`cf5e3cb2     ntdll!LdrpInitializeGraphRecurse+0xc8
0d 000000b6`0657ee60 00007ffa`cf585deb     ntdll!LdrpInitializeProcess+0x1f62
0e 000000b6`0657f280 00007ffa`cf585c73     ntdll!LdrpInitialize+0x15f
0f 000000b6`0657f320 00007ffa`cf585c1e     ntdll!LdrpInitialize+0x3b
10 000000b6`0657f350 00000000`00000000     ntdll!LdrInitializeThunk+0xe
```

The `GetProcAddress` symbol exported by `KERNEL32` resolves to the `KERNEL32!GetProcAddressStub` function. `KERNEL32!GetProcAddressStub` does not show up in the call stack due to a tail call optimization.
