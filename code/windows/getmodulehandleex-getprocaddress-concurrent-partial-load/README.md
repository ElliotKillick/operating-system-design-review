# `GetModuleHandleEx` + `GetProcAddress` Concurrent Partial Load Experiment

We demonstrate the ability `GetProcAddress` has to wait for a load to complete when the module it is locating a symbol in is not done loading.

## Result

```
DllMain: DLL_PROCESS_ATTACH: Start
Got handle to partially loaded module: 00007FFAC8090000
Locating a function in library with GetProcAddress...
```

`GetProcAddress` waiting for ongoing library load to complete:

```
0:005> k
 # Child-SP          RetAddr               Call Site
00 000000d5`50aff9a8 00007ffa`cf570022     ntdll!NtWaitForSingleObject+0x14
01 000000d5`50aff9b0 00007ffa`cf540035     ntdll!LdrpDrainWorkQueue+0x15e
02 000000d5`50aff9f0 00007ffa`cd297e3c     ntdll!LdrGetProcedureAddressForCaller+0x275
03 000000d5`50affb50 00007ff7`2d0c10c1     KERNELBASE!GetProcAddressForCaller+0x6c
04 000000d5`50affba0 00007ffa`ce6f7374     exe_test!getmodulehandleex_thread+0x81 [C:\Users\user\Documents\getmodulehandleex-getprocaddress-concurrent-partial-load\exe-test.c @ 35] 
05 000000d5`50affbf0 00007ffa`cf55cc91     KERNEL32!BaseThreadInitThunk+0x14
06 000000d5`50affc20 00000000`00000000     ntdll!RtlUserThreadStart+0x21
0:005> !handle @$argreg 8
Handle 40
  Object Specific Information
    Event Type Auto Reset
    Event is Waiting
0:005> dp ntdll!LdrpLoadCompleteEvent L1
00007ffa`cf67c380  00000000`00000040
```

The `GetProcAddress` symbol exported by `KERNEL32` resolves to the `KERNEL32!GetProcAddressStub` function. `KERNEL32!GetProcAddressStub` does not show up in the call stack due to a tail call optimization.