# `FeatureStateManager` Worker Threads Randomly Spawning Into Idle Processes

I have noticed that following operations such as a `ShellExecute` API call and after some time goes by (like a few minutes), the kernel or a system component triggers the creation of timed worker threads with short lifetimes into the process that look like this:

```
0:001> k
 # Child-SP          RetAddr               Call Site
00 000000ee`c78fe448 00007ff8`79443a3e     ntdll!NtQueryWnfStateData
01 000000ee`c78fe450 00007ff8`793f4ea7     KERNELBASE!wil_details_NtQueryWnfStateData+0x6e
02 000000ee`c78fe4a0 00007ff8`793f46f2     KERNELBASE!wil::details_abi::RecordWnfUsageIndex+0x8f
03 000000ee`c78ff610 00007ff8`793f4ac2     KERNELBASE!wil::details_abi::UsageIndexes::Record+0x56
04 000000ee`c78ff680 00007ff8`793f3c3b     KERNELBASE!wil::details_abi::FeatureStateData::RecordUsage+0x56
05 000000ee`c78ff790 00007ff8`793f4422     KERNELBASE!wil::details::FeatureStateManager::FlushUsage+0x37
06 000000ee`c78ff7c0 00007ff8`7b942719     KERNELBASE!wil::details::FeatureStateManager::OnTimer+0x32
07 000000ee`c78ff7f0 00007ff8`7b91d79a     ntdll!TppTimerpExecuteCallback+0xa9
08 000000ee`c78ff840 00007ff8`7a737374     ntdll!TppWorkerThread+0x68a
09 000000ee`c78ffb40 00007ff8`7b91cc91     KERNEL32!BaseThreadInitThunk+0x14
0a 000000ee`c78ffb70 00000000`00000000     ntdll!RtlUserThreadStart+0x21
```

Here, the DLL is `KERNELBASE.dll`, but I have also seen this behavior occur for other DLLs like `combase.dll`, `iertutil.dll`, `urlmon.dll`, and `SHCORE.dll`. `FeatureStateManager` is an internal object with no public documentation. The created instance of `FeatureStateManager` is a C++ object that is wrapped by a [shutdown-aware object](https://github.com/microsoft/wil/wiki/Shutdown-aware-objects#shutdown-aware-objects) from the Windows Implementation Library (WIL), which is why some branches on the state of `PEB_LDR_DATA.ShutdownInProgress` can be seen around the objects in its disassembly. These threads are probably callbacks from some other earlier operation, possibly [Windows Notification Framework (WNF)](https://pwnedcoffee.com/blog/wnf-chronicles-i-introduction/#what-is-wnf) callbacks since we can see this callback is making the `ntdll!NtQueryWnfStateData` system call here, but possibly not (I have not gone out of my way to research it). Anyway, the intent of what we see here is probably some generic state management of Windows components or possibly even a telemetry part of it that records usage data (a reasonable idea given the `FeatureStateData::RecordUsage` class and method names).
