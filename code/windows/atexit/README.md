# `atexit` Implementation on Windows

`atexit` is a call defined by the C standard for registering a function to be called on program exit. [See the glibc writing about `atexit` semantics for a review of this interface.](/code/glibc/atexit/README.md)

What follows details some of how C runtimes (CRTs) on the Windows platform implement `atexit`.

The code sample here only covers the modern [Universal CRT (UCRT)](https://learn.microsoft.com/en-us/cpp/windows/universal-crt-deployment). See the [LdrLockLiberator project](https://github.com/ElliotKillick/LdrLockLiberator) for the `atexit` on MSVCRT sample code.

Sample program output:

```
DllMain: DLL_PROCESS_ATTACH
Program main: registering atexit routine
Library test export: registering atexit routine
Running atexit routine of program!
DllMain: DLL_PROCESS_DETACH
Running atexit routine of library!
```

Of note is that the DLL's `DLL_PROCESS_DETACH` routine runs before its `atexit` routine. To me, this seems like poor behavior since a DLL's `DLL_PROCESS_ATTACH` will always run before it registers its first `atexit` routine. Module destructors are always run in the reverse order of module constructors. The same should be true for `atexit` routines. I also find it violates [POLS](https://en.wikipedia.org/wiki/Principle_of_least_astonishment) for the DLL to be deinitialized when its `atexit` routines run.

Call stack when program `atexit` routine is executed:

```
0:000> k
 # Child-SP          RetAddr               Call Site
00 000000b4`ac5afb88 00007ffb`ed1b42d6     exe_test!programAtexitRoutine [C:\Users\user\Documents\QubesIncoming\lab-projects\atexit\exe-test.c @ 6]
01 000000b4`ac5afb90 00007ffb`ed1b41fb     ucrtbase!<lambda_f03950bc5685219e0bcd2087efbe011e>::operator()+0xa6
02 000000b4`ac5afbe0 00007ffb`ed1b41b4     ucrtbase!__crt_seh_guarded_call<int>::operator()<<lambda_7777bce6b2f8c936911f934f8298dc43>,<lambda_f03950bc5685219e0bcd2087efbe011e> &,<lambda_3883c3dff614d5e0c5f61bb1ac94921c> >+0x3b
03 000000b4`ac5afc10 00007ffb`ed1c0522     ucrtbase!execute_onexit_table+0x34
04 000000b4`ac5afc40 00007ffb`ed1c04ab     ucrtbase!<lambda_ad52fe89635f51ec3b38e9c3ac6dac81>::operator()+0x4e
05 000000b4`ac5afc80 00007ffb`ed1c044e     ucrtbase!__crt_seh_guarded_call<void>::operator()<<lambda_123965863b7b46a3332720573f9ce793>,<lambda_ad52fe89635f51ec3b38e9c3ac6dac81> &,<lambda_8d528b66de6ae1e796d7f5e3101fca72> >+0x3b
06 000000b4`ac5afcb0 00007ff6`8c3116ff     ucrtbase!common_exit+0x5e
07 000000b4`ac5afd00 00007ffb`eeb77374     exe_test!__scrt_common_main_seh+0x173 [d:\a01\_work\20\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl @ 295]
08 000000b4`ac5afd40 00007ffb`ef2bcc91     KERNEL32!BaseThreadInitThunk+0x14
09 000000b4`ac5afd70 00000000`00000000     ntdll!RtlUserThreadStart+0x21
```

Call stack when library `atexit` routine is executed:

```
0:000> k
 # Child-SP          RetAddr               Call Site
00 000000b4`ac5af948 00007ffb`ed1b42d6     dll_test!libraryAtexitRoutine [C:\Users\user\Documents\QubesIncoming\lab-projects\atexit\dll-test.c @ 16]
01 000000b4`ac5af950 00007ffb`ed1b41fb     ucrtbase!<lambda_f03950bc5685219e0bcd2087efbe011e>::operator()+0xa6
02 000000b4`ac5af9a0 00007ffb`ed1b41b4     ucrtbase!__crt_seh_guarded_call<int>::operator()<<lambda_7777bce6b2f8c936911f934f8298dc43>,<lambda_f03950bc5685219e0bcd2087efbe011e> &,<lambda_3883c3dff614d5e0c5f61bb1ac94921c> >+0x3b
03 000000b4`ac5af9d0 00007ffb`d40d1695     ucrtbase!execute_onexit_table+0x34
04 000000b4`ac5afa00 00007ffb`d40d17ba     dll_test!dllmain_crt_process_detach+0x45 [d:\a01\_work\20\s\src\vctools\crt\vcstartup\src\startup\dll_dllmain.cpp @ 182]
05 000000b4`ac5afa40 00007ffb`ef289a1d     dll_test!dllmain_dispatch+0xe6 [d:\a01\_work\20\s\src\vctools\crt\vcstartup\src\startup\dll_dllmain.cpp @ 293]
06 000000b4`ac5afaa0 00007ffb`ef2cf1ca     ntdll!LdrpCallInitRoutine+0x61
07 000000b4`ac5afb10 00007ffb`ef2cef7d     ntdll!LdrShutdownProcess+0x22a
08 000000b4`ac5afc20 00007ffb`eeb7e3eb     ntdll!RtlExitUserProcess+0xad
09 000000b4`ac5afc50 00007ffb`ed1c05bc     KERNEL32!ExitProcessImplementation+0xb
0a 000000b4`ac5afc80 00007ffb`ed1c045f     ucrtbase!exit_or_terminate_process+0x44
0b 000000b4`ac5afcb0 00007ff6`8c3116ff     ucrtbase!common_exit+0x6f
0c 000000b4`ac5afd00 00007ffb`eeb77374     exe_test!__scrt_common_main_seh+0x173 [d:\a01\_work\20\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl @ 295]
0d 000000b4`ac5afd40 00007ffb`ef2bcc91     KERNEL32!BaseThreadInitThunk+0x14
0e 000000b4`ac5afd70 00000000`00000000     ntdll!RtlUserThreadStart+0x21
```

MSVC, when compiling for the UCRT, secretly creates a stub that internally branches to either call the CRT `atexit` with a program `atexit` routine table that the CRT keeps state of or a library `atexit` routine table that the DLL track of using its own module-local table:

```
0:000> k
 # Child-SP          RetAddr               Call Site
00 000000bb`d7cffe48 00007ffb`d40d14d9     dll_test!_onexit [d:\a01\_work\20\s\src\vctools\crt\vcstartup\src\utility\utility.cpp @ 256]
01 000000bb`d7cffe50 00007ffb`d40d109c     dll_test!atexit+0x9 [d:\a01\_work\20\s\src\vctools\crt\vcstartup\src\utility\utility.cpp @ 275]
02 000000bb`d7cffe80 00007ff7`30961042     dll_test!TestExport+0x1c [C:\Users\user\Documents\atexit\dll-test.c @ 23]
03 000000bb`d7cffeb0 00007ff7`30961698     exe_test!main+0x22 [C:\Users\user\Documents\atexit\exe-test.c @ 16]
04 (Inline Function) --------`--------     exe_test!invoke_main+0x22 [d:\a01\_work\20\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl @ 78]
05 000000bb`d7cffee0 00007ffb`eeb77374     exe_test!__scrt_common_main_seh+0x10c [d:\a01\_work\20\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl @ 288]
06 000000bb`d7cfff20 00007ffb`ef2bcc91     KERNEL32!BaseThreadInitThunk+0x14
07 000000bb`d7cfff50 00000000`00000000     ntdll!RtlUserThreadStart+0x21
```

```asm
; See comments to the right
    dll_test!_onexit: CFG
00007ffb`d40d1474 4053             push    rbx
00007ffb`d40d1476 4883ec20         sub     rsp, 20h
00007ffb`d40d147a 48833d762c0000ff cmp     qword ptr [dll_test!module_local_atexit_table{._first} (7ffbd40d40f8)], 0FFFFFFFFFFFFFFFFh ; Test if the module has a local atexit table or not by comparing the value of <MODULE>!module_local_atexit_table to -1
                                                                                                                                      ; A program (EXE) will have this global variable set to -1; a library (DLL) will have this global variable to some actual data (it is set this way by the MSVC compiler at compile-time)
00007ffb`d40d1482 488bd9           mov     rbx, function (rcx)
00007ffb`d40d1485 7507             jne     dll_test!_onexit+0x1a (7ffbd40d148e)
00007ffb`d40d1487 e8400c0000       call    dll_test!_crt_atexit (7ffbd40d20cc)                                                        ; Register atexit routine for a program: calls ucrtbase!crt_atexit
00007ffb`d40d148c eb0f             jmp     dll_test!_onexit+0x29 (7ffbd40d149d)
00007ffb`d40d148e 488bd3           mov     rdx, function (rbx)
00007ffb`d40d1491 488d0d602c0000   lea     rcx, [dll_test!module_local_atexit_table{._first} (7ffbd40d40f8)]                          ; Library-specifc data holding its atexit routines: MSVC compiled-in stub code with in this DLL's module destructor will iterate over this in reverse to call each routine
00007ffb`d40d1498 e8230c0000       call    dll_test!_register_onexit_function (7ffbd40d20c0)                                          ; Register atexit routine for library: calls ucrtbase!_register_onexit_function
00007ffb`d40d149d 33d2             xor     edx, edx
00007ffb`d40d149f 85c0             test    eax, eax
00007ffb`d40d14a1 480f44d3         cmove   rdx, function (rbx)
00007ffb`d40d14a5 488bc2           mov     rax, rdx
00007ffb`d40d14a8 4883c420         add     rsp, 20h
00007ffb`d40d14ac 5b               pop     function (rbx)
00007ffb`d40d14ad c3               ret
00007ffb`d40d14ae cc               int     3
00007ffb`d40d14af cc               int     3
```

```
0:000> k
 # Child-SP          RetAddr               Call Site
00 00000065`e79cf778 00007ff7`30961485     exe_test!_onexit [d:\a01\_work\20\s\src\vctools\crt\vcstartup\src\utility\utility.cpp @ 256]
01 00000065`e79cf780 00007ff7`3096103c     exe_test!atexit+0x9 [d:\a01\_work\20\s\src\vctools\crt\vcstartup\src\utility\utility.cpp @ 275]
02 00000065`e79cf7b0 00007ff7`30961698     exe_test!main+0x1c [C:\Users\user\Documents\\atexit\exe-test.c @ 14]
03 (Inline Function) --------`--------     exe_test!invoke_main+0x22 [d:\a01\_work\20\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl @ 78]
04 00000065`e79cf7e0 00007ffb`eeb77374     exe_test!__scrt_common_main_seh+0x10c [d:\a01\_work\20\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl @ 288]
05 00000065`e79cf820 00007ffb`ef2bcc91     KERNEL32!BaseThreadInitThunk+0x14
06 00000065`e79cf850 00000000`00000000     ntdll!RtlUserThreadStart+0x21
```

```asm
    exe_test!_onexit: CFG
00007ff7`30961420 4053             push    rbx
00007ff7`30961422 4883ec20         sub     rsp, 20h
00007ff7`30961426 48833d8a3c0000ff cmp     qword ptr [exe_test!module_local_atexit_table{._first} (7ff7309650b8)], 0FFFFFFFFFFFFFFFFh
00007ff7`3096142e 488bd9           mov     rbx, function (rcx)
00007ff7`30961431 7507             jne     exe_test!_onexit+0x1a (7ff73096143a)
00007ff7`30961433 e8f40b0000       call    exe_test!_crt_atexit (7ff73096202c)
00007ff7`30961438 eb0f             jmp     exe_test!_onexit+0x29 (7ff730961449)
00007ff7`3096143a 488bd3           mov     rdx, function (rbx)
00007ff7`3096143d 488d0d743c0000   lea     rcx, [exe_test!module_local_atexit_table{._first} (7ff7309650b8)]
00007ff7`30961444 e8d70b0000       call    exe_test!_register_onexit_function (7ff730962020)
00007ff7`30961449 33d2             xor     edx, edx
00007ff7`3096144b 85c0             test    eax, eax
00007ff7`3096144d 480f44d3         cmove   rdx, function (rbx)
00007ff7`30961451 488bc2           mov     rax, rdx
00007ff7`30961454 4883c420         add     rsp, 20h
00007ff7`30961458 5b               pop     function (rbx)
00007ff7`30961459 c3               ret
00007ff7`3096145a cc               int     3
00007ff7`3096145b cc               int     3
```

MSVCRT `atexit` does not differentiate between being called by a program or library: how it behaves assumes it is always called by a program.

**Note:** [An Old New Thing article covering when `atexit` routines registered under different contexts are executed](https://devblogs.microsoft.com/oldnewthing/20160930-00/?p=94425) claims that DLL-registered `atexit` routines run from the module destructor of the CRT not the DLL that registered them. However, I have proved this to be false in my own testing. After all, if that were true then many of the dependencies of the DLL that created the `atexit` routine could have been deinitialzied by now since the CRT is widely depended on meaning it usually will be one of the first DLLs to initialize in the process and therefore one of the last DLLs to deinitialize.

## `atexit` Concurrency Implementation

Here is a quick overview of what synchronization mechanisms are used to protect shared `atexit` data and `atexit` routine execution:

UCRT: Critical section lock covering CRT exit (`ucrtbase!common_exit` function), EXE `atexit` (registration and routine execution), and DLL `atexit` (registration and routine execution): `ucrtbase!environ_table+0x70`
  - This lock is broad, protecting all of CRT exit, `atexit` registration (EXE or DLL), and `atexit` routine execution (EXE or DLL)
  - Set a watchpoint on this lock: `ba r4 @@C++(&((ntdll!_RTL_CRITICAL_SECTION *)@@(ucrtbase!environ_table+0x70))->LockCount)`
  - [Source code](https://github.com/huangqinjin/ucrt/blob/master/startup/exit.cpp#L195) (the UCRT is source available)

MSVCRT: Critical section lock covering CRT exit (`msvcrt!doexit` function), EXE `atexit` (registration and routine execution), and DLL `atexit` (registration and routine execution): `msvcrt!CrtLock_Exit`
  - MSVCRT is an ancient C runtime, but it is the one Windows internally links to for applications and libraries that Microsoft includes with the operating system (likely for backward compatibility reasons), with few exceptions throughout the operating system

When calling `atexit` from an EXE, UCRT uses a program `atexit` table
  - UCRT calls `ucrtbase!crt_atexit` which uses this table: `ucrtbase!__acrt_atexit_table`
  - `atexit` routines run as part of CRT exit, with the loader exit coming later, thus these routines are run without loader lock
  - **Exception:** An `atexit` routine registered by a Windows EXE can run in the module destructors of the CRT library, and therefore under loader lock, if process exit occurs by calling the Windows API `ExitProcess` function, as opposed to the CRT `exit` function or the `main` function returning normally

When calling `atexit` from a DLL, UCRT uses the the module's local `atexit` table
  - On MSVC, with the UCRT, `_onexit` calls `ucrtbase!register_onexit_function` which uses the table passed in as an argument by its DLL caller: `<DLL>!module_local_atexit_table`
  - This `atexit` routine runs as part of the DLL's module destructors (just after its `DLL_PROCESS_DETACH`) and thus under loader lock