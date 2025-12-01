# Operating System Design Review

Operating System Design Review is a modern exploration of operating system architecture focusing primarily on user-mode, starting at its origin: [the loader](#defining-loader-and-linker-terminology), and investigating other subsystems from there.

The intentions of this research paper are to:

1. Compare the Windows, Linux, and MacOS user-mode environments
    - Providing perspective on architectural and ecosystem differences, how they coincide with the loader and the broader system, then drawing conclusions and creating solutions based on our findings
2. Focus on the [concurrent](#concurrency-and-parallelism-in-the-loader) design and properties of the system
    - Including formal documentation on how the modern Windows loader functions in contrast to current open source Windows implementations, including Wine and ReactOS (they lack support for the "parallel loading" ability present in a modern Windows loader)
3. Educate, satisfy curiosity, and help fellow reverse engineers
    - Establish the foundation for whitepapers derived from this research paper

All of the information contained here covers Windows 10 22H2 and glibc 2.38 on Linux. In certain cases, facts were also verified on a fully up-to-date release of Windows 11. Some sections of this write-up additionally touch on MacOS, and now other operating systems, too.

**Author:** Elliot Killick (@ElliotKillick)

## Table of Contents

- [Operating System Design Review](#operating-system-design-review)
  - [Table of Contents](#table-of-contents)
  - [Parallel Loader Overview](#parallel-loader-overview)
  - [High-Level Loader Synchronization](#high-level-loader-synchronization)
  - [Windows Loader Module State Transitions Overview](#windows-loader-module-state-transitions-overview)
  - [Constructors and Destructors Overview](#constructors-and-destructors-overview)
    - [Legacy Windows](#legacy-windows)
    - [C# and .NET](#c-and-net)
  - [The Root of `DllMain` Problems](#the-root-of-dllmain-problems)
  - [The Problem with How Windows Uses Libraries](#the-problem-with-how-windows-uses-libraries)
  - [Dependency Breakdown](#dependency-breakdown)
    - [Modularity](#modularity)
    - [Fearful Concurrency](#fearful-concurrency)
    - [Dangerous Destructors](#dangerous-destructors)
    - [Non-Evictable Libraries](#non-evictable-libraries)
    - [Reference Cycles Overhead](#reference-cycles-overhead)
    - [Page Fault Surplus](#page-fault-surplus)
    - [Hidden Dependencies](#hidden-dependencies)
    - [Vendor Lock-In](#vendor-lock-in)
    - [Repairability](#repairability)
    - [Disrupts Per-Module Synchronization for Library Initialization](#disrupts-per-module-synchronization-for-library-initialization)
    - [Conclusion](#conclusion)
  - [The Lazy Loading Liability](#the-lazy-loading-liability)
    - [DLL Devaluation](#dll-devaluation)
      - [DLL Upgradability](#dll-upgradability)
      - [Modularity](#modularity-1)
      - [Uses fewer resources](#uses-fewer-resources)
    - [Breaks Module Initialization](#breaks-module-initialization)
    - [Dangerous Destructors](#dangerous-destructors-1)
    - [Breaks Dynamic Linking of Data Symbols](#breaks-dynamic-linking-of-data-symbols)
    - [Security](#security)
    - [Performance](#performance)
    - [Introduces Unexpected Failure Points](#introduces-unexpected-failure-points)
    - [Disadvantages Static Linking](#disadvantages-static-linking)
    - [Operating System Destruction](#operating-system-destruction)
    - [Conclusion](#conclusion-1)
  - [Further Research on Windows' Usage of DLLs](#further-research-on-windows-usage-of-dlls)
    - [The DLL Host](#the-dll-host)
    - [DLL Procurement](#dll-procurement)
    - [One DLL, One Base Address](#one-dll-one-base-address)
    - [DLLs as Data](#dlls-as-data)
  - [Library Loading Locations Across Operating Systems](#library-loading-locations-across-operating-systems)
  - [`LoadLibrary` vs `dlopen` Return Type](#loadlibrary-vs-dlopen-return-type)
  - [Exploring Fine-Grained Module Initialization Thread Safety](#exploring-fine-grained-module-initialization-thread-safety)
  - [The Problem with How Windows Uses Threads](#the-problem-with-how-windows-uses-threads)
  - [Thread Lifecycle Mismanagement Case Study with `ShellExecute`](#thread-lifecycle-mismanagement-case-study-with-shellexecute)
  - [Process Meltdown](#process-meltdown)
    - [In-Process Inconsistencies](#in-process-inconsistencies)
    - [Process Hanging Open](#process-hanging-open)
    - [Crash](#crash)
    - [Out-of-Process Inconsistencies](#out-of-process-inconsistencies)
    - [Kernel Object Cleanup Performance Degradation](#kernel-object-cleanup-performance-degradation)
    - [Conclusion](#conclusion-2)
  - [Further Research on Windows' Usage of Threads](#further-research-on-windows-usage-of-threads)
    - [Securable Threads](#securable-threads)
    - [Multithreading is Insecure](#multithreading-is-insecure)
    - [Expensive Threads](#expensive-threads)
  - [DLL Thread Routines Anti-Feature](#dll-thread-routines-anti-feature)
    - [Synchronization Requirements](#synchronization-requirements)
  - [Flimsy Thread-Local Data](#flimsy-thread-local-data)
  - [The PEB Problem](#the-peb-problem)
    - [Adds Process Startup Overhead](#adds-process-startup-overhead)
    - [Promotes Centralization](#promotes-centralization)
    - [Elevates Backward Compatibility Risk](#elevates-backward-compatibility-risk)
    - [Weakens Security](#weakens-security)
    - [Accessing the PEB is Slow](#accessing-the-peb-is-slow)
    - [Conclusion](#conclusion-3)
  - [When Initialization Fails](#when-initialization-fails)
    - [Unix](#unix)
    - [Windows](#windows)
    - [C++](#c)
      - [Replacing Exceptions with Returning the Error](#replacing-exceptions-with-returning-the-error)
    - [Rust](#rust)
  - [Historical Windows Library Loader Issues with Module Initialization](#historical-windows-library-loader-issues-with-module-initialization)
    - [Improper State Management of Library Dependencies](#improper-state-management-of-library-dependencies)
      - [Problems in Reentrancy During Module Initialization](#problems-in-reentrancy-during-module-initialization)
      - [Difficulty Processing Library Unloads](#difficulty-processing-library-unloads)
    - [The Windows `GetModuleHandle` function is broken](#the-windows-getmodulehandle-function-is-broken)
  - [COM is Bloatware](#com-is-bloatware)
    - [In-Process COM Reinvents C++ and Dynamic Linking](#in-process-com-reinvents-c-and-dynamic-linking)
    - [COM Does Nothing to Help with Versioning](#com-does-nothing-to-help-with-versioning)
    - [COM is Unintuitive](#com-is-unintuitive)
    - [Callback Bloat](#callback-bloat)
    - [Single-Threaded COM Servers are Expensive](#single-threaded-com-servers-are-expensive)
    - [COM is a Security Liability](#com-is-a-security-liability)
    - [Distributed Objects are Bad](#distributed-objects-are-bad)
    - [Conclusion](#conclusion-4)
  - [Symbol Lookup Operating System Comparison](#symbol-lookup-operating-system-comparison)
  - [How Does `GetProcAddress`/`dlsym` Handle Concurrent Library Unload?](#how-does-getprocaddressdlsym-handle-concurrent-library-unload)
  - [ELF Flat Symbol Namespace (Global Symbols, GNU Namespaces, and Unique Symbols)](#elf-flat-symbol-namespace-global-symbols-gnu-namespaces-and-unique-symbols)
  - [GNU Loader Global Scope Symbol Protection](#gnu-loader-global-scope-symbol-protection)
  - [Assessing the Brokenness of `GetModuleHandle` and `GetModuleHandleEx` Functions](#assessing-the-brokenness-of-getmodulehandle-and-getmodulehandleex-functions)
    - [Recommendations for Developers](#recommendations-for-developers)
  - [`GetProcAddress` Workarounds for `GetModuleHandle` and `GetModuleHandleEx` Functions](#getprocaddress-workarounds-for-getmodulehandle-and-getmodulehandleex-functions)
    - [`GetProcAddress` with Uninitialized Module Mitigation](#getprocaddress-with-uninitialized-module-mitigation)
      - [Modern and Legacy Loader Implementations Analysis](#modern-and-legacy-loader-implementations-analysis)
    - [Concurrent `GetProcAddress` with Partially Loaded Module Mitigation](#concurrent-getprocaddress-with-partially-loaded-module-mitigation)
    - [Holes in the Workarounds](#holes-in-the-workarounds)
  - [Windows Loader Initialization Locking Requirements](#windows-loader-initialization-locking-requirements)
  - [Loader Enclaves](#loader-enclaves)
  - [Investigating the COM Server Deadlock from `DllMain`](#investigating-the-com-server-deadlock-from-dllmain)
  - [GNU Loader Lock Hierarchy and Synchronization Strategy](#gnu-loader-lock-hierarchy-and-synchronization-strategy)
  - [Thread Creation is Not Thread-Safe on Windows: A Concurrency Bug in the Windows Loader](#thread-creation-is-not-thread-safe-on-windows-a-concurrency-bug-in-the-windows-loader)
  - [Library Lazy Loading and Lazy Linking Overview](#library-lazy-loading-and-lazy-linking-overview)
    - [Lazy Loading](#lazy-loading)
    - [Lazy Linking](#lazy-linking)
    - [Windows Delay Loading](#windows-delay-loading)
  - [Module Information Data Structures](#module-information-data-structures)
  - [Loader Components](#loader-components)
    - [Locks](#locks)
    - [State](#state)
    - [Atomic State](#atomic-state)
  - [Reverse Engineered Windows Loader Functions](#reverse-engineered-windows-loader-functions)
    - [`LdrpDrainWorkQueue`](#ldrpdrainworkqueue)
    - [`LdrpDecrementModuleLoadCountEx`](#ldrpdecrementmoduleloadcountex)
    - [`LdrpDropLastInProgressCount`](#ldrpdroplastinprogresscount)
    - [`LdrpProcessWork`](#ldrpprocesswork)
  - [What is COM?](#what-is-com)
  - [Object-Oriented Software Frameworks Overview](#object-oriented-software-frameworks-overview)
    - [Microsoft Component Object Model (COM)](#microsoft-component-object-model-com)
    - [Common Object Request Broker Architecture (CORBA)](#common-object-request-broker-architecture-corba)
    - [GNU/Linux Component Models and History](#gnulinux-component-models-and-history)
    - [MacOS Distributed Objects and NSXPCConnection](#macos-distributed-objects-and-nsxpcconnection)
    - [Fun Facts](#fun-facts)
  - [The Process Lifetime](#the-process-lifetime)
  - [Defining Loader and Linker Terminology](#defining-loader-and-linker-terminology)
  - [Concurrency and Parallelism in the Loader](#concurrency-and-parallelism-in-the-loader)
  - [ABBA Deadlock](#abba-deadlock)
  - [ABA Problem](#aba-problem)
  - [Dining Philosophers Problem](#dining-philosophers-problem)
  - [License](#license)

## Parallel Loader Overview

When a library load contains more than one work item (i.e. a library with at least one dependency that is not already loaded into the process), the Windows loader will use its parallel loading ability to speed up library loading. The first work item of a load will always happen sequentially, on the same thread that called `LoadLibrary`, because the loader must begin to map and snap one library before it can find dependencies that it also needs to map and snap. To start, see what a [trace of one library with no new dependencies looks like](data/windows/loadlibrary-trace.log), and get acquainted with how [concurrency and parallelism in the loader](#concurrency-and-parallelism-in-the-loader).

Put simply, the parallel loader is a layer on top of the regular loader that calls `ntdll!LdrpQueueWork` to offload library loading work to other loader threads:

```
0:000> # "call    ntdll!LdrpQueueWork" <NTDLL_ADDRESS> L9999999
ntdll!LdrpSignalModuleMapped+0x54:
00007ffa`56b208e0 e83bebffff      call    ntdll!LdrpQueueWork (00007ffa`56b1f420)
ntdll!LdrpMapAndSnapDependency+0x20d:
00007ffa`56b27b9d e87e78ffff      call    ntdll!LdrpQueueWork (00007ffa`56b1f420)
ntdll!LdrpLoadDependentModule+0xd63:
00007ffa`56b28943 e8d86affff      call    ntdll!LdrpQueueWork (00007ffa`56b1f420)
ntdll!LdrpLoadContextReplaceModule+0x126:
00007ffa`56b718e2 e839dbfaff      call    ntdll!LdrpQueueWork (00007ffa`56b1f420)
```

Everything else is infrastructure to support this work offloading mechanism.

The loader calls solely the `ntdll!LdrpQueueWork` function to add modules to the `ntdll!LdrpWorkQueue` linked list data structure. Work processors (i.e. callers of `ntdll!LdrpProcessWork`) such as loader worker threads or the `ntdll!LdrpDrainWorkQueue` function, for instance in a [concurrent `LoadLibrary`](code/windows/loadlibrary-concurrent-work/README.md), access the `ntdll!LdrpWorkQueue` list to pick up a work item. Access to the `ntdll!LdrpWorkQueue` shared data structure is protected by the `ntdll!LdrpWorkQueueLock` critical section lock.

Each list entry in the `ntdll!LdrpWorkQueue` data structure is a `LDRP_LOAD_CONTEXT` structure. This structure is undocumented by Microsoft because its contents are not in the public debug symbols. Each `LDRP_LOAD_CONTEXT` structure relates directly to one module because a module's `LDR_DATA_TABLE_ENTRY` structure is allocated at the same time as its `LDRP_LOAD_CONTEXT` structure in the `LdrpAllocatePlaceHolder` function. In addition, the first member of each `LDRP_LOAD_CONTEXT` structure is a `UNICODE_STRING` of the `BaseDllName` according to the module that it relates to.

Loader worker threads are dedicated threads that are part of a thread pool for parallelizing loader work. These threads can be identified by checking whether the `LoaderWorker` flag is present the `TEB.SameTebFlags` of a thread.

Only mapping and snapping work can be offloaded for parallelized processing because [module initialization routines must execute sequentially](#concurrency-and-parallelism-in-the-loader).

## High-Level Loader Synchronization

The high-level loader synchronization mechanisms responsible for controlling the loader are the `LdrpLoadCompleteEvent` and `LdrpWorkCompleteEvent` loader events in NTDLL.

When the loader sets the `LdrpLoadCompleteEvent` event, it is signalling the completion of a full library load or unload, or the completion of loader thread initialization. When `LdrpLoadCompleteEvent` is signalled, it directly correlates with `ntdllLdrpWorkInProgress` equalling zero and the decommissioning of the current thread as the load owner (`LoadOwner` flag in `TEB.SameTebFlags`). Here is a minimal reverse engineering of the `ntdll!LdrpDropLastInProgressCount` function showing this:

```c
NTSTATUS LdrpDropLastInProgressCount()
{
    // Remove thread's load owner flag
    PTEB CurrentTeb = NtCurrentTeb();
    CurrentTeb->SameTebFlags &= ~LoadOwner; // 0x1000

    // Load/unload is now complete
    RtlEnterCriticalSection(&LdrpWorkQueueLock);
    LdrpWorkInProgress = 0;
    RtlLeaveCriticalSection(&LdrpWorkQueueLock);

    // Signal completion of load/unload to any waiting threads
    return NtSetEvent(LdrpLoadCompleteEvent, NULL);
}
```

When the loader sets the `LdrpWorkCompleteEvent` event, it is signalling that the loader has completed the mapping and snapping work on the entire work queue across all of the currently processing loader worker threads. When a loader worker thread starts, it atomically increments `ntdll!LdrpWorkInProgress` (in the `ntdll!LdrpWorkCallback` function) and when a loader worker thread ends, it atomically decrements `ntdll!LdrpWorkInProgress` (at the end of the `ntdll!LdrpProcessWork` function). This means that every increment to the `ntdll!LdrpWorkInProgress` reference counter past `1`, since that is the value `ntdll!LdrpDrainWorkQueue` initially sets `ntdll!LdrpWorkInProgress` to, indicates another loader worker thread processing a work item in parallel. Here is a minimal reverse engineering of where the `ntdll!LdrpProcessWork` function returns showing this:

```c
   // Second argument of LdrpProcessWork: isCurrentThreadLoadOwner
   // If the current thread is a loader worker (i.e. not a load owner)
   if (!isCurrentThreadLoadOwner)
   {
       RtlEnterCriticalSection(&LdrpWorkQueueLock);
       // If the work queue is empty AND we we are the last loader worker thread processing work
       // There were some double negatives I had to sort out here in the reverse engineering
       BOOL doSetEvent = &LdrpWorkQueue == LdrpWorkQueue.Flink && --LdrpWorkInProgress == 1
       Status = RtlLeaveCriticalSection(&LdrpWorkQueueLock);
       if ( doSetEvent )
           return NtSetEvent(LdrpWorkCompleteEvent, NULL);
    }

    return Status;
```

Here are all the loader's usages of `LdrpLoadCompleteEvent` and `LdrpWorkCompleteEvent`:

```
0:000> # "ntdll!LdrpLoadCompleteEvent" <NTDLL_ADDRESS> L9999999
ntdll!LdrpDropLastInProgressCount+0x38:
00007ffd`2896d9c4 488b0db5e91000  mov     rcx,qword ptr [ntdll!LdrpLoadCompleteEvent (00007ffd`28a7c380)]
ntdll!LdrpDrainWorkQueue+0x2d:
00007ffd`2896ea01 4c0f443577d91000 cmove   r14,qword ptr [ntdll!LdrpLoadCompleteEvent (00007ffd`28a7c380)]
ntdll!LdrpCreateLoaderEvents+0x12:
00007ffd`2898e182 488d0df7e10e00  lea     rcx,[ntdll!LdrpLoadCompleteEvent (00007ffd`28a7c380)]
```

```
0:000> # "ntdll!LdrpWorkCompleteEvent" <NTDLL_ADDRESS> L9999999
ntdll!LdrpDrainWorkQueue+0x18:
00007ffd`2896e9ec 4c8b35bdd91000  mov     r14,qword ptr [ntdll!LdrpWorkCompleteEvent (00007ffd`28a7c3b0)]
ntdll!LdrpProcessWork+0x1e4:
00007ffd`2896ede0 488b0dc9d51000  mov     rcx,qword ptr [ntdll!LdrpWorkCompleteEvent (00007ffd`28a7c3b0)]
ntdll!LdrpCreateLoaderEvents+0x35:
00007ffd`2898e1a5 488d0d04e20e00  lea     rcx,[ntdll!LdrpWorkCompleteEvent (00007ffd`28a7c3b0)]
ntdll!LdrpProcessWork$fin$0+0x7c:
00007ffd`289b5ad7 488b0dd2680c00  mov     rcx,qword ptr [ntdll!LdrpWorkCompleteEvent (00007ffd`28a7c3b0)]
```

The `ntdll!LdrpCreateLoaderEvents` function creates both events. Only the `ntdll!LdrpDrainWorkQueue` function can wait (calling `ntdll!NtWaitForSingleObject`) on the `LdrpLoadCompleteEvent` or `LdrpWorkCompleteEvent` loader events. Only the `ntdll!LdrpDropLastInProgressCount` function sets `LdrpLoadCompleteEvent`. Only the `ntdll!LdrpProcessWork` function sets `LdrpWorkCompleteEvent`.

At event creation (`ntdll!NtCreateEvent`), `LdrpLoadCompleteEvent` and `LdrpWorkCompleteEvent` are configured to be [auto-reset events](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-resetevent#:~:text=Auto%2Dreset%20event%20objects%20automatically%20change%20from%20signaled%20to%20nonsignaled%20after%20a%20single%20waiting%20thread%20is%20released.).

The loader never manually resets the `LdrpLoadCompleteEvent` and `LdrpWorkCompleteEvent` events (with `ntdll!NtResetEvent`).

The `ntdll!LdrpDrainWorkQueue` function takes one argument called `LoadContext`. This argument is a flag that allows the function to determine whether it should synchronize on the `LdrpLoadCompleteEvent` or `LdrpWorkCompleteEvent` loader event, if necessary, before letting execution proceed.

What follows documents the parts of the loader that call `ntdll!LdrpDrainWorkQueue` (data gathered by searching disassembly for calls to the `ntdll!LdrpDrainWorkQueue` function) in either the load owner or load worker load context:

```
ntdll!LdrUnloadDll+0x80:                          OWNER
ntdll!RtlQueryInformationActivationContext+0x43c: OWNER
ntdll!LdrShutdownThread+0x98:                     OWNER
ntdll!LdrpInitializeThread+0x86:                  OWNER
ntdll!LdrpLoadDllInternal+0xbe:                   OWNER
ntdll!LdrpLoadDllInternal+0x144:                  WORKER
ntdll!LdrpLoadDllInternal$fin$0+0x38:             WORKER
ntdll!LdrGetProcedureAddressForCaller+0x270:      OWNER
ntdll!LdrEnumerateLoadedModules+0xa7:             OWNER
ntdll!RtlExitUserProcess+0x23:                    OWNER or WORKER
  - Depends on `TEB.SameTebFlags`, typically `OWNER` if `LoadOwner` or `LoaderWorker` flags are absent, `TRUE` if either of these flags are present
ntdll!RtlPrepareForProcessCloning+0x23:           OWNER
ntdll!LdrpFindLoadedDll+0x9127a:                  OWNER
ntdll!LdrpFastpthReloadedDll+0x9033a:             OWNER
ntdll!LdrpInitializeImportRedirection+0x46d44:    OWNER
ntdll!LdrInitShimEngineDynamic+0x3c:              OWNER
ntdll!LdrpInitializeProcess+0x130a:               OWNER
ntdll!LdrpInitializeProcess+0x1d0d:               OWNER
ntdll!LdrpInitializeProcess+0x1e22:               WORKER
ntdll!LdrpInitializeProcess+0x1f33:               OWNER
ntdll!RtlCloneUserProcess+0x71:                   OWNER
```

Calls to the `ntdll!LdrpDrainWorkQueue` function do not always result in synchronizing on the associated loader event for that load context.

Notably, there are many more instances of the loader potentially synchronizing on the entire load's completion rather than just the completion of mapping and snapping work. For example, thread initialization (`ntdll!LdrpInitializeThread`) always synchronizes on the `LdrpLoadCompleteEvent` loader event. The only parts of the loader that may synchronize on `LdrpWorkCompleteEvent` are `ntdll!LdrpLoadDllInternal`, `ntdll!LdrpInitializeProcess`, and `ntdll!RtlExitUserProcess`.

Here are the places where the loader completes all loader work (`ntdll!LdrpDropLastInProgressCount` function), which is where the `LdrpLoadCompleteEvent` is set. Although, many of these are edge cases with the invocations by `ntdll!LdrpLoadDllInternal`, or loader thread initialization/deinitialization by the `ntdll!!LdrpInitializeThread` and `ntdll!LdrShutdownThread` functions being the most common:

```
0:000> # "call    ntdll!LdrpDropLastInProgressCount" <NTDLL_ADDRESS> L9999999
ntdll!LdrUnloadDll+0x99:
00007ffa`56b1fc89 e8eef10400      call    ntdll!LdrpDropLastInProgressCount (00007ffa`56b6ee7c)
ntdll!RtlQueryInformationActivationContext+0x463:
00007ffa`56b23243 e834bc0400      call    ntdll!LdrpDropLastInProgressCount (00007ffa`56b6ee7c)
ntdll!LdrShutdownThread+0x20b:
00007ffa`56b2765b e81c780400      call    ntdll!LdrpDropLastInProgressCount (00007ffa`56b6ee7c)
ntdll!LdrpInitializeThread+0x218:
00007ffa`56b27950 e827750400      call    ntdll!LdrpDropLastInProgressCount (00007ffa`56b6ee7c)
ntdll!LdrpLoadDllInternal+0x24b:
00007ffa`56b2fc5f e818f20300      call    ntdll!LdrpDropLastInProgressCount (00007ffa`56b6ee7c)
ntdll!LdrGetProcedureAddressForCaller+0x275:
00007ffa`56b40035 e842ee0200      call    ntdll!LdrpDropLastInProgressCount (00007ffa`56b6ee7c)
ntdll!LdrEnumerateLoadedModules+0xae:
00007ffa`56b6ee6e e809000000      call    ntdll!LdrpDropLastInProgressCount (00007ffa`56b6ee7c)
ntdll!LdrShutdownThread$fin$2+0x1e:
00007ffa`56bb4f95 e8e29efbff      call    ntdll!LdrpDropLastInProgressCount (00007ffa`56b6ee7c)
ntdll!LdrpInitializeThread$fin$2+0x15:
00007ffa`56bb4ff4 e8839efbff      call    ntdll!LdrpDropLastInProgressCount (00007ffa`56b6ee7c)
ntdll!LdrpLoadDllInternal$fin$0+0x47:
00007ffa`56bb526e e8099cfbff      call    ntdll!LdrpDropLastInProgressCount (00007ffa`56b6ee7c)
ntdll!LdrEnumerateLoadedModules$fin$0+0x1b:
00007ffa`56bb5ee9 e88e8ffbff      call    ntdll!LdrpDropLastInProgressCount (00007ffa`56b6ee7c)
ntdll!LdrpFindLoadedDll+0x917ae:
00007ffa`56bbf2ce e8a9fbfaff      call    ntdll!LdrpDropLastInProgressCount (00007ffa`56b6ee7c)
ntdll!LdrpFastpthReloadedDll+0x90862:
00007ffa`56bc04e2 e895e9faff      call    ntdll!LdrpDropLastInProgressCount (00007ffa`56b6ee7c)
ntdll!LdrpInitializeImportRedirection+0x464cf:
00007ffa`56bd89b3 e8c464f9ff      call    ntdll!LdrpDropLastInProgressCount (00007ffa`56b6ee7c)
ntdll!LdrInitShimEngineDynamic+0xe8:
00007ffa`56be0528 e84fe9f8ff      call    ntdll!LdrpDropLastInProgressCount (00007ffa`56b6ee7c)
ntdll!LdrpInitializeProcess+0x183c:
00007ffa`56be358c e8ebb8f8ff      call    ntdll!LdrpDropLastInProgressCount (00007ffa`56b6ee7c)
ntdll!LdrpInitializeProcess+0x1eda:
00007ffa`56be3c2a e84db2f8ff      call    ntdll!LdrpDropLastInProgressCount (00007ffa`56b6ee7c)
ntdll!LdrpInitializeProcess+0x1f8e:
00007ffa`56be3cde e899b1f8ff      call    ntdll!LdrpDropLastInProgressCount (00007ffa`56b6ee7c)
```

Here are the few places where the loader processes mapping and snapping work (`ntdll!LdrpProcessWork` function), which is where the `LdrpWorkCompleteEvent` is set:

```
0:000> # "call    ntdll!LdrpProcessWork" <NTDLL_ADDRESS> L9999999
ntdll!LdrpLoadDependentModule+0x184c:
00007ffa`56b2942c e8bb6c0400      call    ntdll!LdrpProcessWork (00007ffa`56b700ec)
ntdll!LdrpLoadDllInternal+0x13a:
00007ffa`56b2fb4e e899050400      call    ntdll!LdrpProcessWork (00007ffa`56b700ec)
ntdll!LdrpDrainWorkQueue+0x17f:
00007ffa`56b70043 e8a4000000      call    ntdll!LdrpProcessWork (00007ffa`56b700ec)
ntdll!LdrpWorkCallback+0x6e:
00007ffa`56b700ce e819000000      call    ntdll!LdrpProcessWork (00007ffa`56b700ec)
```

## Windows Loader Module State Transitions Overview

`LDR_DDAG_NODE.State` or `LDR_DDAG_STATE` tracks a module's **entire lifetime** from beginning to end. With this analysis, I intend to extrapolate information based on the [known types given to us by Microsoft](https://www.geoffchappell.com/studies/windows/km/ntoskrnl/inc/api/ntldr/ldr_ddag_state.htm) (`dt _LDR_DDAG_STATE` command in WinDbg).

Each state represents a stage of loader work on a module. This table comprehensively documents where these state changes occur throughout the loader and which locks are present

A typical library load ranges from <code>LdrModulesPlaceHolder</code> to <code>LdrModulesReadyToRun</code> (may also include `LdrModulesMerged`), and a typical library unload ranges from <code>LdrModulesUnloading</code> to <code>LdrModulesUnloaded</code>.

<table summary="All LDR_DDAG_NODE.LDR_DDAG_STATE states, the function(s) responsible for each state change, and more information">
  <tr>
    <th><code>LDR_DDAG_STATE</code> States</th>
    <th>State Changing Function(s)</th>
    <th>Remarks</th>
  </tr>
  <tr>
    <th>LdrModulesMerged (-5)</th>
    <td><code>LdrpMergeNodes</code></td>
    <td><code>LdrpModuleDatatableLock</code> is held during this state change. See <code>LdrModulesCondensed</code> state for more information.</td>
  </tr>
  <tr>
    <th>LdrModulesInitError (-4)</th>
    <td><code>LdrpInitializeGraphRecurse</code></td>
    <td>During <code>DLL_PROCESS_ATTACH</code>, if a module's <code>DllMain</code> <a href="https://learn.microsoft.com/en-us/windows/win32/dlls/dllmain#return-value" target="_blank">returns <code>FALSE</code> for failure</a> then this module state is set (any other return value counts as success). <code>LdrpLoaderLock</code> is held here.</td>
  </tr>
  <tr>
    <th>LdrModulesSnapError (-3)</th>
    <td><code>LdrpCondenseGraphRecurse</code></td>
    <td>This function may set this state on a module if a snap error occurs. See the <code>LdrModulesCondensed</code> state for more information.</td>
  </tr>
  <tr>
    <th>LdrModulesUnloaded (-2)</th>
    <td><code>LdrpUnloadNode</code></td>
    <td>Before setting this state, <code>LdrpUnloadNode</code> may walk <code>LDR_DDAG_NODE.Dependencies</code>, holding <code>LdrpModuleDataTableLock</code> to call <code>LdrpDecrementNodeLoadCountLockHeld</code> thus decrementing the <code>LDR_DDAG_NODE.LoadCount</code> of dependencies and recursively calling <code>LdrpUnloadNode</code> to unload dependencies. Loader lock (<code>LdrpLoaderLock</code>) is held here.</td>
  </tr>
  <tr>
    <th>LdrModulesUnloading (-1)</th>
    <td><code>LdrpUnloadNode</code></td>
    <td>Set near the start of this function. This function checks for <code>LdrModulesInitError</code>, <code>LdrModulesReadyToInit</code>, and <code>LdrModulesReadyToRun</code> states before setting this new state. After setting state, this function calls <code>LdrpProcessDetachNode</code>. Loader lock (<code>LdrpLoaderLock</code>) is held here.</td>
  </tr>
  <tr>
    <th>LdrModulesPlaceHolder (0)</th>
    <td><code>LdrpAllocateModuleEntry</code></td>
    <td>The loader directly calls <code>LdrpAllocateModuleEntry</code> until parallel loader initialization (<code>LdrpInitParallelLoadingSupport</code>) occurs at process startup. At which point (with exception to directly calling <code>LdrpAllocateModuleEntry</code> once more soon after parallel loader initialization to allocate a module entry for the EXE), the loader calls <code>LdrpAllocatePlaceHolder</code> (this function first allocates a <code>LDRP_LOAD_CONTEXT</code> structure), which calls through to <code>LdrpAllocateModuleEntry</code> (this function places a pointer to this module's <code>LDRP_LOAD_CONTEXT</code> structure at <code>LDR_DATA_TABLE_ENTRY.LoadContext</code>). The <code>LdrpAllocateModuleEntry</code> function, along with creating the module's <code>LDR_DATA_TABLE_ENTRY</code> structure, allocates its <code>LDR_DDAG_NODE</code> structure with zero-initialized heap memory. <strong>The module's data structures have been allocated with basic initialization.</strong></td>
  </tr>
  <tr>
    <th>LdrModulesMapping (1)</th>
    <td><code>LdrpMapCleanModuleView</code></td>
    <td>I've never seen this function get called; the state typically jumps from 0 to 2. Only the <code>LdrpGetImportDescriptorForSnap</code> function may call this function which itself may only be called by <code>LdrpMapAndSnapDependency</code> (according to a disassembly search). <code>LdrpMapAndSnapDependency</code> typically calls <code>LdrpGetImportDescriptorForSnap</code>; however, <code>LdrpGetImportDescriptorForSnap</code> doesn't typically call <code>LdrpMapCleanModuleView</code>. This state is set before mapping a memory section (<code>NtMapViewOfSection</code>). <strong>Mapping is the process of loading a file from disk into memory.</strong></td>
  </tr>
  <tr>
    <th>LdrModulesMapped (2)</th>
    <td><code>LdrpProcessMappedModule</code></td>
    <td><code>LdrpModuleDatatableLock</code> is held during this state change. <strong>Mapping is complete.</strong></td>
  </tr>
  <tr>
    <th>LdrModulesWaitingForDependencies (3)</th>
    <td><code>LdrpLoadDependentModule</code></td>
    <td>This state isn't typically set, but during a trace, I was able to observe the loader set it by launching a web browser (Google Chrome) under WinDbg, which triggered the watchpoint in this function when loading app compatibility DLL <code>C:\Windows\System32\ACLayers.dll</code>. Interestingly, the <code>LDR_DDAG_STATE</code> decreases by one here from <code>LdrModulesSnapping</code> to <code>LdrModulesWaitingForDependencies</code>; the only time I've observed this. <code>LdrpModuleDatatableLock</code> is held during this state change.</td>
  </tr>
  <tr>
    <th>LdrModulesSnapping (4)</th>
    <td><code>LdrpSignalModuleMapped</code> or <code>LdrpMapAndSnapDependency</code></td>
    <td>In the <code>LdrpMapAndSnapDependency</code> case, a jump from <code>LdrModulesMapped</code> to <code>LdrModulesSnapping</code> may happen. <code>LdrpModuleDatatableLock</code> is held during state change in <code>LdrpSignalModuleMapped</code>, but not in <code>LdrpMapAndSnapDependency</code>. <strong>Snapping is the process of resolving the library's import address table (module imports and exports) to addresses in memory.</strong></td>
  </tr>
  <tr>
    <th>LdrModulesSnapped (5)</th>
    <td><code>LdrpSnapModule</code> or <code>LdrpMapAndSnapDependency</code></td>
    <td>In the <code>LdrpMapAndSnapDependency</code> case, a jump from <code>LdrModulesMapped</code> to <code>LdrModulesSnapped</code> may happen, which indicates the loader doesn't always bother recording the in-between <code>LdrModulesSnapping</code> state transition. <code>LdrpModuleDatatableLock</code> isn't held here in either case. <strong>Snapping is complete.</strong></td>
  </tr>
  <tr>
    <th>LdrModulesCondensed (6)</th>
    <td><code>LdrpCondenseGraphRecurse</code</td>
    <td>This function receives a <code>LDR_DDAG_NODE</code> as its first argument and recursively calls itself to walk <code>LDR_DDAG_NODE.Dependencies</code>. On every recursion, this function checks whether it can remove the passed <code>LDR_DDAG_NODE</code> from the graph. If so, this function acquires <code>LdrpModuleDataTableLock</code> to call the <code>LdrpMergeNodes</code> function, which receives the same first argument, then releasing <code>LdrpModuleDataTableLock</code> after it returns. <code>LdrpMergeNodes</code> discards the uneeded node from the <code>LDR_DDAG_NODE.Dependencies</code> and <code>LDR_DDAG_NODE.IncomingDependencies</code> DAG adjacency lists of any modules starting from the given parent node (first function argument), decrements <code>LDR_DDAG_NODE.LoadCount</code> to zero, and calls <code>RtlFreeHeap</code> to deallocate <code>LDR_DDAG_NODE</code> DAG nodes. After <code>LdrpMergeNodes</code> returns, <code>LdrpCondenseGraphRecurse</code> calls <code>LdrpDestroyNode</code> to deallocate any DAG nodes in the <code>LDR_DDAG_NODE.ServiceTagList</code> list of the parent <code>LDR_DDAG_NODE</code> then deallocate the parent <code>LDR_DDAG_NODE</code> itself. <code>LdrpCondenseGraphRecurse</code> sets the state to <code>LdrModulesCondensed</code> before returning. <strong>Note:</strong> The <code>LdrpCondenseGraphRecurse</code> function and its callees rely heavily on all members of the <code>LDR_DDAG_NODE</code> structure, which needs further reverse engineering to fully understand the inner workings and "whys" of what's occurring here. <strong>Condensing is the process of discarding unnecessary nodes from the dependency graph.</strong></td>
  </tr>
  <tr>
    <th>LdrModulesReadyToInit (7)</th>
    <td><code>LdrpNotifyLoadOfGraph</code></td>
    <td>This state is set immediately before this function calls <code>LdrpSendPostSnapNotifications</code> to run post-snap DLL notification callbacks. As the loader initializes nodes (i.e. modules) in the dependency graph (while loader lock is held), each node's state will transition to <code>LdrModulesInitializing</code> then <code>LdrModulesReadyToRun</code> (or <code>LdrModulesInitError</code> if initialization fails). <strong>The module is mapped and snapped but pending initialization (which includes any form of running code from the module).</strong></td>
  </tr>
  <tr>
    <th>LdrModulesInitializing (8)</th>
    <td><code>LdrpInitializeNode</code></td>
    <td>Set at the start of this function, immediately before linking a module into the <code>InInitializationOrderModuleList</code> list. After linking the module into the initialization order list, the loader calls the module's <code>LDR_DATA_TABLE_ENTRY.EntryPoint</code>. Loader lock (<code>LdrpLoaderLock</code>) is held here. <strong>Initializing is the process of running a module's initialization routines (i.e. module initializer including Windows <code>DllMain</code>).</strong></td>
  </tr>
  <tr>
    <th>LdrModulesReadyToRun (9)</th>
    <td><code>LdrpInitializeNode</code></td>
    <td>Set at the end of this function, before it returns. Loader lock (<code>LdrpLoaderLock</code>) is held here. <strong>The module is ready for use.</strong></td>
  </tr>
</table>

Findings were gathered by [tracing all `LDR_DDAG_STATE.State` values at load-time](code/windows/scripts/debugging-commands.md#ldr_ddag_node-analysis) and tracing a library unload, as well as searching disassembly. See what a [LDR_DDAG_STATE trace log](data/windows/load-many-modules-ldr-ddag-node-state-trace.log) looks like.

## Constructors and Destructors Overview

Constructors and destructors exist to facilitate dynamic initialization. Dynamic initialization is custom code that runs before accessing a resource. In the module scope, this code executes before the `main()` function or when a module is loaded.

Module constructors and destructors are the operating system and language agnostic terms for describing this feature. On Unix, these may be referred to as initialization and finalization or termination routines/functions. In Windows DLLs, the functionally equivalent idea exists as `DLL_PROCESS_ATTACH` and `DLL_PROCESS_DETACH` calls to the `DllMain` function. Initialization and deinitialization/uninitialization routines or simply initializer and finalizer is also common terminology.

In addition to module load and unload, the Windows loader may call each module's `DllMain` at `DLL_THREAD_ATTACH` and `DLL_THREAD_DETACH` times. The Windows loader only calls these routines at thread start and exit. Windows doesn't run the `DLL_THREAD_ATTACH` of a DLL following `DLL_PROCESS_ATTACH`. Additionally, a [DLL loaded after thread start won't preempt that thread to run its `DllMain` with `DLL_THREAD_ATTACH`](https://learn.microsoft.com/en-us/windows/win32/dlls/dllmain#parameters). These calls can be disabled per-library as a performance optimization by calling `DisableThreadLibraryCalls` at `DLL_PROCESS_ATTACH` time.

Compilers commonly provide access to module initialization/deinitialization functions through compiler-specific syntax. In GCC or Clang, a programmer can create module constructors/destructors using the `__attribute__((constructor))` and `__attribute__((destructor))` functions or the [`_init` and `_fini` functions, historically](https://man7.org/linux/man-pages/man3/dlopen.3.html#NOTES). Modern GCC or Clang module constructors and destructors support specifying a priority like `__attribute__((constructor(101)))` or `__attribute__((destructor(101)))` (priorities of 100 and below are reserved for use by the operating system) in case a particular execution order is desired.

In C++, the constructor of an object is invoked whenever an instance of a class is created. Creating an instance of a class returns an object pointing to that instance. If an object is created in the global scope (C++ terminology) or the module scope (OS terminology) then its constructor is called during program or library initialization ([code example](code/windows/library-constructors-init-order-test/dll-test.cpp)). If an object is created in a local scope like in a function, its constructor is called when program execution creates that object in the function. A constructor or class itself is neither inherently global nor local, it entirely depends on what context the object is created in.

Common use cases for dynamic initialization can include: [Communication with](https://github.com/reactos/reactos/blob/f10d40f9122b926bf01b5409a6d3c3d9d06806c3/dll/win32/kernel32/client/dllmain.c#L138) [another process](https://github.com/reactos/reactos/blob/3ecd2363a6d045a38aa68a1b5f17bb53ffaad3e4/win32ss/user/user32/misc/dllmain.c#L510) (for instance, the Windows API relies on a system-wide [`csrss.exe`](https://en.wikipedia.org/wiki/Client/Server_Runtime_Subsystem) [server](https://en.wikipedia.org/wiki/Client%E2%80%93server_model), which requires dynamic initialization on the side of the client), [creating an inter-process synchronization mechanism](https://learn.microsoft.com/en-us/windows/win32/sync/interprocess-synchronization) (Windows commonly uses inter-process event synchronization objects even when predominantly or only intra-process synchronization is or should be required), or initializing an implementation-dependent data structure such as a [critical section](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-initializecriticalsection) (rather than storing the internal [POD](https://stackoverflow.com/a/146464) directly in your module, which would necessitate that ABI remain backward compatible forever or be versioned). The apparent reason for Microsoft not providing a method for statically initializing a Windows critical section is that developers [butchered the original POD definition](https://devblogs.microsoft.com/oldnewthing/20160826-00/?p=94185#:~:text=CRITICAL_SECTION) and when they wanted to go back and change it to something more sensible (i.e. [simply initializing to all zeros by default like GNU does](https://elixir.bootlin.com/glibc/glibc-2.38/source/nptl/pthread_mutex_init.c#L142-L147)), they were not able to without breaking [bug compatibility](https://en.wikipedia.org/wiki/Bug_compatibility). Further, I can confirm that the `InitializeCriticalSection` function introduced in the [first Windows NT release](data/windows/legacy-software-analysis.md#operating-systems) did not create any associated kernel object for waiting at the time it was called, as these objects were only created when and if waiting on that critical section occurred thus saving on kernel resources (modern operating systems implement waiting through the use of a [futex](https://en.wikipedia.org/wiki/Futex)). So, there was no practical reason why critical sections could not have supported static initialization at the time they were introduced. In the common case where default mutex attributes are appropriate, [POSIX mutexes can be statically initialized with the `PTHREAD_MUTEX_INITIALIZER` macro](https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread_mutex_init.html); otherwise, or when creating a mutex in dynamically allocated memory, dynamic initialization with the `pthread_mutex_init` function is necessary. A POSIX mutex is equivalent to a Windows critical section, whereas a Windows mutex object differs due to being an inter-process synchronization mechanism. Other dynamic initialization operations could include: Setting up a thread pool, background thread, or event loop to prepare early for concurrent operations. Reading configuration data from some persistent data source (e.g. an environment variable, a file, or a registry key). Tracing or logging events or setting it up. Controlling resource lifecycle (at initialization and destruction time). In addition, various domain-specific initialization and registration tasks. Generally, constructors effectively address [cross-cutting concerns](https://en.wikipedia.org/wiki/Cross-cutting_concern#Examples) in initialization.

Due to the useful position of constructors and destructors when run in the global scope, they may sometimes be used outside of dynamic initialization like for auditing or hooking purposes. The GNU loader specially provides the `LD_ADUIT` and `LD_PRELOAD` mechanisms for these purposes (with the latter having broad support across Unix-like systems). The GNU loader calls into an `LD_AUDIT` library at library load/unload and symbol resolution times to allow for hooking or monitoring. `LD_PRELOAD` allows easily hooking global scope symbol resolution. Windows allows for DLL notifications registration, offering similar functionality, though it is more limited. The always and early execution style of constructors when in the module scope also makes them an [attractive target for attackers](https://man.openbsd.org/dlopen.3#CAVEATS).

Constructors and destructors originate from object-oriented programming (OOP), a programming paradigm first introduced by the [Simula 67](https://en.wikipedia.org/wiki/Simula) language in 1962. C++, a modern object-oriented language, was originally designed in the early 1980s as an extension of C and received initial standardization in 1998 with ISO/IEC 14882:1998. Constructors and destructors do not exist in the C standard. On Unix systems, the concept of code that runs when a module loads and unloads goes back to the 1990 [System V Application Binary Interface Version 4](https://www.bitsavers.org/pdf/att/unix/System_V_Release_4/0-13-933706-7_Unix_System_V_Rel4_Programmers_Guide_ANSI_C_and_Programming_Support_Tools_1990.pdf) (`DT_INIT` and `DT_FINI` section types, as well as `.init` and `.fini` special section names).

In the ELF executable format, module constructors and destructors are formally specified by the System V ABI to be in the `.init` and `.fini` sections. Modern systems use the [de facto standard](https://en.wikipedia.org/wiki/De_facto_standard) (but never formally specified in the System V ABI) [`.init_array`/`.fini_array` sections](https://maskray.me/blog/2021-11-07-init-ctors-init-array), or before that the deprecated `.ctors`/`.dtors` sections. Modern GCC built binaries only include `.init_array`/`.fini_array` and `.init`/`.fini` sections, they don't include the `.ctors`/`.dtors` sections (verified with `objdump -h` and `readelf --sections`). Individually exposing each routine in an array within the ELF file grants more control over initialization and finalization routine execution to a Unix-like loader over calling an opaque function for handling all initialization/finalization. This control and transparency lends itself to a pluggable interace that is useful in concepts such as constructor and destructor priority control ([the glibc loader does not use this per-routine knowledge to compensate for circular dependencies during module initialization](code/glibc/dlopen-init-interruption/README.md)). [A Unix-like loader loops through these routines contained in the ELF file.](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/dl-init.c#L58-L71)

The PE (Windows) executable format standard [does not define any sections specific to module initialization](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#special-sections); instead, a `DllMain` function or any module constructors/destructors are included with the rest of the program code in the `.text` section. MSVC optionally provides the [`init_seg`](https://learn.microsoft.com/en-us/cpp/preprocessor/init-seg) pragma to specify a section name with module constructors to run frist when compiling C++ code. However, such a section is only used if this pragma is explicitly specified by the programmer (unlikely) or in the niche cases MSVC will generate one itself (as stated by the documentation). The granularity this pragma provides is low with only `compiler`, `lib`, and `user` options. In contrast, the `.init_array`/`.fini_array` sections and `__attribute__((constructor(priority)))`/`__attribute__((destructor(priority)))` on Unix-like systems serve as a modular and robust means for controlling dynamic initializiation order.

The Windows loader calls a module's `LDR_DATA_TABLE_ENTRY.EntryPoint` at module initialization or deinitialization with the respective `fdwReason` argument (`DLL_PROCESS_ATTACH` or `DLL_PROCESS_DETACH`); it has no knowledge of `DllMain` or C++ constructors/destructors in the module scope. Merging these into one callable `EntryPoint` is the job of a compiler. For instance, [MSVC compiles a stub into your DLL (`dllmain_dispatch`) that calls any module constructors followed by `DllMain` with the `DLL_PROCESS_ATTACH` argument](code/windows/library-constructors-init-order-test/exe-test.c) (and destructors, of course, in the reverse order). Constructors other than `DllMain`, of course, initialize in the order they are laid out in code. The word `Main` in `DllMain` indicates that `DllMain` will run as the last constructor in the module similar to how the `main` function of a program runs after all constructors. Still, I find `DllMain` to generally be a bad name because it may lead people to use constructors in ways that one might use the `main` function of a program due to the similar name (like `DllMain` is just `main` but in a DLL, which is not the case). I also find Microsoft's use of the term "entry point" (e.g. in `LDR_DATA_TABLE_ENTRY.EntryPoint`) to describe calling a module's constructor and destructor routines bad because an [entry point has a specific definition that refers to the start of program execution](https://en.wikipedia.org/wiki/Entry_point). This reason for this name stems from both an EXE and its DLLs having a `LDR_DATA_TABLE_ENTRY`. Especially since the Windows loader does accurately set the EXE's `EntryPoint` set to the program's main function (then [just above `EntryPoint` is the `DllBase` member of `LDR_DATA_TABLE_ENTRY`](https://www.geoffchappell.com/studies/windows/km/ntoskrnl/inc/api/ntldr/ldr_data_table_entry.htm), which conflates EXEs with DLLs but the other way around). So, a good question is posed by asking why the `LDR_DATA_TABLE_ENTRY` structure definition should be shared between an EXE and its DLLs at all seeing as the [as the GNU loader does not conflate these concepts](code/glibc/scripts/debugging-commands.md#link_map-analysis) because, besides both being some code with data that is mapped into memory, these are completely different things. Up until one point in Windows history, [the `LDR_DATA_TABLE_ENTRY` structure definition was even shared between kernel and user-mode modules until separating into the `KLDR_DATA_TABLE_ENTRY` structure](https://www.geoffchappell.com/studies/windows/km/ntoskrnl/inc/api/ntldr/ldr_data_table_entry/index.htm): "The `LDR_DATA_TABLE_ENTRY` structure is NTDLL's record of how a DLL is loaded into a process. In early Windows versions, this structure is similarly the kernel's record of each module that is loaded for kernel-mode execution. The different demands of kernel and user modes eventually led to the separate definition of a `KLDR_DATA_TABLE_ENTRY`." The GNU loader calls legacy [`init` before going through the `init_array` functions](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/dl-init.c#L56) (the opposite of Windows where `DllMain` comes last after all other constructors, similar to how a `main` function would). All of these facts come together to paint a picture of Windows being too kernel-centric and monolithic, not considering the unique requirements of user-mode and correctly distinguishing between execution environments.

### Legacy Windows

The name `DllMain` is inherited from [`LibMain`](https://learn.microsoft.com/en-us/archive/msdn-magazine/2000/july/under-the-hood-happy-10th-anniversary-windows#dlls-and-module-management), which along with Windows Exit Procedure (WEP) for exit, was its name in the 16-bit DLLs used by Windows 3.x (non-NT). When Windows was built for 16-bit applications (before Windows NT 3.1 and Windows 95 based on MS-DOS), [multitasking was cooperative, not preemptive](https://web.archive.org/web/20150619005446/https://support.microsoft.com/en-us/kb/117567). So, predictable scheduling meant there was no need for synchronization mechanisms such as loader lock and hence no lock hierarchy concerns regarding library initialization and finalization. System libraries were also typically already loaded in the single shared address space and that was the level tasks (what we now call "processes" were widely referred to as "tasks" before each application had an independent address space and execution context) would reference-count them on. By nature then, the libraries a program relied on were typically already loaded in the shared system address which meant those libraries has previously already done their initialization at a predictable time. Still, the MS-DOS EXE format allowed for an [MS-DOS library](data/windows/timeline-verification/README.md#windows-31-files) to specify whether module initialization should be "Global" or "Per-Process" (this information can be gathered using the old `exehdr` tool), which would have only lessened the room for module initialization issues based on when it occurs in the global case. There was no dynamic linker in MS-DOS meaning the libraries of that time did not support specifying imports as they did starting with Windows NT. [DOS Extenders](https://en.wikipedia.org/wiki/DOS_extender) allowed specially written applications to use protected mode without true privilege separation; however, operating system libraries could not be loaded outside of real mode, instead requiring that the application running in protected mode makes DOS system calls via an interface such as [DOS Protected Mode Interface (DPMI)](https://en.wikipedia.org/wiki/DOS_Protected_Mode_Interface) or [Virtual Control Program Interface (VCPI)](https://en.wikipedia.org/wiki/Virtual_Control_Program_Interface) to interact with operating system libraries. No dynamic linker and no threading or preemptive multitasking obviously meant delay loading did not exist in Windows 3.x. These properties of older systems largely meant module constructor or destructor issues never occurred with `LibMain` on early Windows versions.

[Windows 3.x (non-NT) books](https://bitsavers.computerhistory.org/pdf/microsoft/windows_3.1/) at the time published by Microsoft (specifically "Windows Programmers Reference Volume 2 Functions" released in 1992), provided no guidance on `LibMain` besides that the "`LibMain` function is called by the system to initialize a dynamic-link library (DLL)". Although, there was a note for WEP that explictly stated "The `FreeLibrary` function should not be called from within a WEP function"—likely due to reentrancy limitations of the loader at that time.

We know from [Matt Pietrek's Windows Internals book](https://bitsavers.computerhistory.org/pdf/microsoft/windows_3.1/Pietrek_-_Windows_Internals_1993.pdf) (released in 1993, shortly before Windows NT 3.1 came out and long before the author [later became a Microsoft employee](https://en.wikipedia.org/wiki/Matt_Pietrek)) that "A common problem programmers encounter is that functions like `MessageBox()` won't work inside the `LibMain()` of an implicitly-linked DLL". The reason is that creating a window to [show a message box](https://elliotonsecurity.com/perfect-dll-hijacking/offlinescannershell-mpclient-dll-missing-export-error.png) requires initialization of the USER application message queue by the `InitApp()` function in USER. This message queue is not initialized in the `LibMain` of USER but by some setup work done before calling `WinMain` in the EXE (the book provides the relevant reverse engineered pseudocode of `C0W.ASM` to prove this): "For EXEs, the important parts of the startup code involves calling `InitTask()` and then `InitApp()`, which we cover momentarily. After those functions have been called, the EXE is completely initialized and ready to start its work as a Windows program." The book notes that initialization is done this way because a DLL "cannot own things that Windows associates with a task, like message queues" (i.e. a DLL may not exist for the full application lifetime) so it cannot own the application message queue. However, the core issue here is that there each task had a single, global application message queue and a DLL couldn't create and tear down its own, independent message queue instance to perform a GUI operation detached from the application (obviously, this is no longer the case in modern Windows). Instead of the application lifetime (that of the EXE), the message box can live [in the instance lifetime](#the-process-lifetime) (from when birth when the call to `MessageBox` is made to death when it returns, since `MessageBox` is a synchronous function), or better yet in [in the lifetime of the DLL](#the-process-lifetime) (from birth at `DLL_PROCESS_ATTACH` to death at `DLL_PROCESS_DETACH` for modern `DllMain`, which works since our DLL depends on the GUI subsystem DLL). Therefore, GUI operations not working from the `LibMain` of libraries that were dependencies of a program's startup were a consequence of tight coupling between the GUI subsystem and the program: Microsoft's compiler added a stub into the program that required execution for the GUI subsystem to be fully initialized and thus able to create GUI elements.

[The deadly `NtTerminateProcess` was first added beginning in Windows NT 3.1](https://www.geoffchappell.com/studies/windows/win32/ntdll/history/names310.htm#:~:text=NtTerminateProcess) seemingly as an incredibly poor and hasty but deliberate hack posing as a valid design choice. MS-DOS-based Windows 95, releasing two years after Windows NT 3.1 in 1993, had a function called `TerminateProcess` which fulfilled the same purpose as `NtTerminateProcess` for programs that ran in its Win32 environment. The Win32 environment of Windows 95 allowed MS-DOS-based Windows to run Win32 programs made for Windows NT.

The Component Object Model (COM)—which tightly couples with the loader by placing itself at the top of the lock hierarchy in `CoFreeUnusedLibraries` and potenitally other places—was not a foundational Windows technology used pervasively within the Windows API [until Windows NT 4.0](https://bitsavers.computerhistory.org/pdf/microsoft/windows_NT_4.0/Solomon_-_Inside_Windows_NT_2ed_1998.pdf#:~:text=Component%20Object%20Model) (released in 1996).

From Windows NT 3.1 to Windows NT 4.0, the lock responsible for protecting loader operations like module initialization and finalization was called the ["process mutant" lock](https://www.geoffchappell.com/studies/windows/km/ntoskrnl/inc/ntos/ps/eprocess/index.htm#:~:text=KMUTANT%20ProcessMutant;), not "loader lock". The process mutant lock lived in the kernel as part of the `EPROCESS` structure and NTDLL would make the `ZwWaitForProcessMutant` and `ZwReleaseProcessMutant` system calls to lock and unlock this synchronization mechanism respectively, unlike loader lock which is a regular user-mode critical section.

### C# and .NET

The [Common Language Runtime (CLR) loader](https://www.oreilly.com/library/view/essential-net-volume/0201734117/0201734117_ch02lev1sec5.html) uses a module's [`.cctor` section](https://web.archive.org/web/20170317220947/https://msdn.microsoft.com/en-us/library/aa290048(VS.71).aspx#vcconmixeddllloadingproblemanchor6) to initialize .NET assemblies. A .NET assembly is a layer of abstraction over an underlying native library—they map 1:1 with each other. Each module `.cctor` section is the "managed module initializer" (i.e. assembly initializer). Microsoft uses the [managed module initializer to work around Windows issues surrounding loader lock](https://learn.microsoft.com/en-us/cpp/dotnet/initialization-of-mixed-assemblies#code) in .NET applications. When building a .NET project, compiling a binary with the `/clr` option causes the MSVC compiler to put module initializers and finalizers in this `.cctor` section. Now, the high-level CLR loader will perform module or assembly initialization and deinitialization instead of the native loader, thus working around "loader lock issues"—as the Microsoft documentation puts it—that are symptomatic of Windows architectural problems. Of course, `DllMain` or any constructors/destructors explicitly specified as unmanaged will still be run by the native loader under the traditional loader lock.

A static constructor in C# is unique from its C++ counterpart because [C# specifies](https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/classes#1512-static-constructors) that the static constructor of a library, even when instance creation happens at the module scope, will initialize on-demand instead of when the library loads:

> The static constructor for a closed class executes at most once in a given application domain. The execution of a static constructor is triggered by the first of the following events to occur within an application domain:
>
> - An instance of the class is created.
> - Any of the static members of the class are referenced.
>
> If a class contains the `Main` method (§7.1) in which execution begins, the static constructor for that class executes before the `Main` method is called.

C# also has finalizers (historically referred to as destructors in C#). The finalizer of an object will run if the garbage collector decides it can destroy the given object. Unlike low-level languages with manual memory mangement like C++, finalization is not typically necessary because the garbage collector traces memory allocations to do clean up. Garbage collectors delay resources cleanup like freeing memory as a function of how they work, it is a trade-off they make in exchange for easier programming. This delay extends to finalizers or destructors where these routines will not run until the garbage collector destroys the object. For unmanaged or system resources such as "windows, files, and network connections" (e.g. closing a database connection) [Microsoft documentation](https://learn.microsoft.com/en-us/dotnet/csharp/programming-guide/classes-and-structs/finalizers#using-finalizers-to-release-resources) condones the use of finalizers saying "you should use finalizers to free those resources". However, [starting with .NET 5, finalizers are not run at application exit](https://learn.microsoft.com/en-us/dotnet/csharp/programming-guide/classes-and-structs/finalizers#:~:text=.NET%205%20(including%20.NET%20Core)%20and%20later%20versions%20don%27t%20call%20finalizers%20as%20part%20of%20application%20termination.). The decision not to call destructors or finalizers at .NET runtime exit is a workaround solution that stems from a lifetime management issue. The issue arises when there are [reachable objects with unjoined background threads](https://github.com/dotnet/runtime/issues/16028) that are using those reachable objects. So, if process exit included running destructors on all objects, including reachable ones, then a background thread could still be using an object while or after the destructor for that object has been run. The root issue in this case is the unjoined thread that was spawned by .NET code but that remains still running even past when .NET shutdown occurs because that is a lifetime management violation. Also for releasing unmanaged resources (i.e. external to the .NET runtime so they won't be garbage collected, like a Windows API file handle), an application can register for the [`AppDomain.ProcessExit`](https://learn.microsoft.com/en-us/dotnet/api/system.appdomain.processexit) event to perform cleanup before the .NET runtime exits in the process and a library assembly can use the [`AppDomain.DomainUnload`](https://learn.microsoft.com/en-us/dotnet/api/system.appdomain.domainunload) event to get the same functionality for its lifetime (this works because [an individual .NET assembly cannot unload without unloading the entire domain of assemblies](https://learn.microsoft.com/en-us/dotnet/standard/assembly/load-unload)). Starting with .NET 5, an assembly can be dynamically loaded into a `AssemblyLoadContext`, which on `Unload`, will free all the assemblies in that load context and call [`Unloading` events](https://learn.microsoft.com/en-us/dotnet/api/system.runtime.loader.assemblyloadcontext.unloading) for cleanup. Assemblies in the [default assembly load context](https://learn.microsoft.com/en-us/dotnet/api/system.runtime.loader.assemblyloadcontext.default) cannot be unloaded. Garbage collected languages are a special case because they [conceptually simulate infinite memory](https://devblogs.microsoft.com/oldnewthing/20100809-00/?p=13203) when, in actuality, memory is finite. So naturally, destructors, the use of system resources requiring well-defined lifetimes (e.g. threads), and other common requirements of systems code fit poorly into the garbage collection model. Due to the nature of garbage collected languages waiting an arbitrary amount of time before performing resource cleanup while an application is running, the deletion of especially expensive or contested system resources is best performed by prescribing that users of your subsystem call a `Shutdown`, `Close`, `Disconnect`, etc. method on the relevant object when they are done using it, if possible. Although this approach does not scale well with resources owned by a library because libraries depend on each other and must be destructed in the reverse order they were constructed (while a programmer could throw in some delicate and non-composable hack by essentially creating an extension of the loader that does its own reference counting to know when the destruction of a shared resource should occur, this approach falls apart with circular references or reference cycles between two resources), applications can use this technique. If an application heavily consumes limited or contended system resources, a programmer might generally want to reconsider using a garbage-collected language, as a system-level language is typically better suited for such scenarios. A destructor in a garbage-collected language must also not acquire any or only limited locks because garbage collection suspends application execution at an arbitrary point. So, if an application thread is suspended while holding a lock then a destructor running on a garbage collector thread tries to acquire the same lock, deadlock will occur. Alternatively, creating a [mixed assembly](https://learn.microsoft.com/en-us/cpp/dotnet/mixed-native-and-managed-assemblies) by integrating in some native/unmanaged (system-level) code into the managed (high-level, garbage-collected) code can offer a balanced, middle-ground solution. The [trade-offs garbage-collected programming languages make](https://en.wikipedia.org/wiki/Garbage_collection_(computer_science)#Disadvantages) in exchange for ease of managing the resources consumed by a high-level application such as their stop-the-world method of reclaiming resources and running garbage collection at an unpredictable times mean that they are not well fit to building system or high throughput components. Destructors at any scope do not work well under these constraints, but that is the fault of garbage collection, not destructors.

C# supports the [`ModuleInitializer` attribute](https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/attributes/general#moduleinitializer-attribute) for initialization code that is to run when the assembly loads even when that assembly is a library (like traditional static constructors). Presumably, C# module initializers require protection from a global CLR initialization lock (like the native loader in NTDLL has with loader lock). In C# 9 and .NET 5 (released together in 2020), [module initializers were added to the language and runtime out of necessity](https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/proposals/csharp-9.0/module-initializers#summary).

The unexpected initialization time of C# static constructors can cause unforseen problems similar to how Windows delay loading does for operating system initializers. For instance, a static constructor ["call is made in a locked region based on the specific type of the class"](https://learn.microsoft.com/en-us/dotnet/csharp/programming-guide/classes-and-structs/static-constructors#remarks), in other words per-module locking or per-class locking as it pertains to the object-oriented paradigm. So, if creating an instance of a class for the first time happens at an unexpected time (perhaps via proxy through another call) like when the thread is holding a lock, and there exists another class which has a static constructor that acquires the same external lock in the reverse order, then lock order inversion and consequently ABBA deadlock can occur. CLR lazy loading/initialization does have a couple significant mitigating factors that make it safer than native library lazy loading, namely: Firstly, lazy initialization can only occur upon instance creation which is necessarily more expected because it's already known that typical per-instance object constructors will run at instance creation time (unlike native library lazy loading where the initialization can potentially happen on every call to a DLL import). Though, this does leave the other, less common, static constructor trigger of referencing a member in a static class somewhat up in the air as to its safety at the given time. Secondly, static constructor synchronization occurs at a per-class level instead of a broadly serializing "CLR static constructor lock", thus decreasing the chance of deadlock by a shared lock. Lazy initialization can still become problematic if your lazy initializer accidentally tries to lazily initialize itself again thus leading to a deadlock (this issue is typically an artifact of circular dependencies). In reagard to libary loading, a synchronized, lazily initializing global type (e.g. a C# static constructor) should never load or unload libraries (or higher level .NET assemblies) to ensure that the OS loader (also CLR loader for .NET assemblies) sensibly remains at the top of the lock hierarchy. This steadfast rule must be in place to maintain lock hierarchy. If some data is only accessed from a single threaded, though, then lazy initialization may not require synchronization (synchronization is mandatory for C# static constructors and is the [the default for `Lazy<T>` types](https://learn.microsoft.com/en-us/dotnet/api/system.lazy-1?view=net-9.0#thread-safety)). Note that Microsoft documentation breaks this sensible idea on lock hierarchy by [recommending programmers call `LoadLibrary` from lazy static constructors](https://learn.microsoft.com/en-us/dotnet/csharp/programming-guide/classes-and-structs/static-constructors#usage). Regardless of synchronization, modules with significant [cross-cutting concerns](https://en.wikipedia.org/wiki/Cross-cutting_concern#Examples) should [never lazily initialize](https://devblogs.microsoft.com/oldnewthing/20070815-00/?p=25573), instead initializing at module load-time, or preferably initializing at compile-time if possible while minimizing unnecessary dependencies. From purely a performance point of view, lazy initializers introduce ["measurable overhead"](https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/proposals/csharp-9.0/module-initializers#motivation) in the form of a constant and fixed execution cost because the language or runtime must internally perform atomic or synchronized checks to decide whether or not the initializer needs to be run on every pass. With all these factors in mind, it can generally be safe, while still not performent, to use a syncrhonized, lazily initializing global type as long as an application has a clear structure that ensures a lazy initializer will not depend on itself through some means (directly or indirectly), and that this thinking extends to subsystems your code depends on (e.g. the OS loader).

The CLR loader, particularly the fact that it intentionally runs outside the OS loader, is a hack because only one of these two components can be at the top of the lock hierarchy and since the OS loader starts first, it should take precedence. By the CLR loader placing itself higher in the lock hierarchy than the OS loader, the CLR becomes tighly coupled with the OS loader. Ideally, the CLR under C# should be able to, as a modular subsystem, safely abstract from the OS without worrying about low-level concerns within the native loader. In particular, it should ideally be possible for C# to use the same constructors and destructors as C++ because Microsoft has tighly integrated .NET into Windows thus making it possible to accidentally utilize the technology when the programmer did not intend to, such as via [COM interop](https://en.wikipedia.org/wiki/COM_Interop) (there are likely some cases where the Windows API internally uses .NET through COM interop with an in-process server).

## The Root of `DllMain` Problems

The native constructor and destructor routines provided by the operating system are broken on the Windows platform. The brokeness of module initialization and finalization routines in the context of a library or dynamic-link library (DLL) on Windows can lead to correctness isses, deadlocks, and crash scenarios that stem from a variety of architectural flaws throughout the foundation of the operating sytem. *The Root of `DllMain` Problems*, or [*`DllMain` Rules Rewritten*](/data/windows/rationale-for-rewriting-dllmain-rules.md), provides a fundamental understanding of hurdles affecting module initialization and finalization or the `DllMain` function on Windows and why they exist. The architectural reasons for `DllMain` issues on Windows are:

- Windows is the ultimate monolith
  - The [broadness of the Windows API](https://en.wikipedia.org/wiki/Criticism_of_Microsoft#Vendor_lock-in) (thousands of DLLs in `C:\Windows\System32`, including everything from file creation to WinHTTP) in combination with its [lack of a clear separation between components](#the-problem-with-how-windows-uses-libraries) leads to operating-system-wide dependency breakdown
    - Circular dependencies are highly problematic for DLL initialization order because it means no initialization order can satisfy the requirements of two or more libraries: if the libraries rely on each other in their module constructor or destructor functions, then a crash or other erroneous behavior can result
    - Modern Windows employs lazy library loading throughout virtually every part of the operating system as a hack to workaround Microsoft's abysmal API design, which can inherently cause a library to load at any time thus forcing the loader to bottom of any external (i.e. unused by NTDLL or outside of the initializing module) lock hierarchy and making any action taken from there, such as by a module constructor or destructor, constantly at risk of trigerring an [ABBA deadlock](#abba-deadlock)
  - The monolithic architecture of the Windows API may cause the loader's lock hierarchy to become nested within the lock hierarchy of a separate subsystem; if this nesting interleaves with another thread nesting in the opposite order: ABBA deadlock is the result
    - The COM and loader subsystems exhibit tight coupling whereby [Microsoft's implementation of COM may interact with the loader while holding a COM apartment lock](https://github.com/reactos/reactos/blob/cfcc8d85b2b3065d895925eb32837a83019eed98/dll/win32/ole32/compobj.c#L1135-L1157) ([a single COM apartment can house multiple COM objects](https://learn.microsoft.com/en-us/windows/win32/com/processes--threads--and-apartments#the-apartment-and-the-com-threading-architecture)), an issue that becomes increasingly problematic due to the [Windows API's extensive use of COM behind the scenes](data/windows/com-components.txt)
  - Despite Windows prioritizing libraries and shared processes over programs and small processes at the operating-system-level, its library dependency infrastructure is significantly less robust and more tightly coupled than its Unix counterpart
  - Windows kernel mode and user mode closely integrate (NT and NTDLL), whereas [Unix began with modularity as a core value](https://en.wikipedia.org/wiki/Unix_philosophy)
    - This value carried through to the formalization of Unix in the POSIX and C standards, and its System V ABI specifications
- Thread lifetime mismangement and constraints
  - The [Windows API misuses threads](#the-problem-with-how-windows-uses-threads) by improperly controlling thread lifecycles in the scope of a process
    - Abrupt thread termination at process exit means the process is in an unknowable state when module destructors run at this time
  - The Windows threading implementation [meshes with the loader at thread startup and exit](#dll-thread-routines-anti-feature) (`DLL_THREAD_ATTACH` and `DLL_THREAD_DETACH`) thus breaking the library subsystem lifetime for threads
    - This anti-feature results in threads being unable to join from within a module destructor even when library unload occurs outside of process exit
  - The Windows API internally makes heavy use of [thread-local data](#flimsy-thread-local-data), which can locks users to the lifetime of an unspecified thread that loaded a given library
  - The loader may run each library's initialization and finalization routines under a per-library activation context (`LDR_DATA_TABLE_ENTRY.EntryPointActivationContext`)—an application compatibility mechanism applied as thread state (`TEB.ActivationContextStack`)—which makes it susceptible to thread-affinity issues
  - Despite Windows prioritizing multithreading over multiprocessing at the operating-system-level, its use of threads and threading implementation is significantly less robust and more prone to deadlocks than its Unix counterpart
- Misuse of and heavy reliance on dynamic library loading
  - The Windows API heavily relying on dynamic library loading past its intended use case of loading extension libraries as opposed to the core operating system components (further, even the intended use case can be unsuitable if isolation from the extension is desirable), leads to libraries loading at unexpected times
  - Inherently, the library lazy loading or delay loading ability of Windows may unexpectedly cause library loading when a programmer did not intend
    - MacOS previously supported lazy loading until Apple removed it, likely due to scenarios where it becomes an anti-feature and because more holistic solutions exist for improving process startup performance
  - Windows institutes that [creating a process can load libraries into the existing process](#library-loading-locations-across-operating-systems)
- Overreliance on dynamic initialization
  - It is always best practice for robustness and performance to initialize statically (i.e. at compile time) over dynamically (using module initializers and finalizers including Windows `DllMain`) if feasible
  - Windows commonly requires dynamic initialization even for core system functionality, such as [initializing a critical section](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-initializecriticalsection)
  - Windows API design quirks artifically enforce dynamic initialization when none should be necessary
    - Allocating or freeing heap memory with `HeapAlloc` or `HeapFree` requires passing in a heap handle, usually to the process heap, which is retrieved by calling `GetProcessHeap`; as a performance optimization for removing this redundant `GetProcessHeap` call ([made slower by its PEB implementation](#the-peb-problem)), some Windows DLLs store the process heap handle in a global variable during module initialization (the best API design solution here was using a flag to indicate allocation to the process heap instead of exposing its handle)
  - In contrast, POSIX data structures commonly provide a [static initialization option](#constructors-and-destructors-overview) and the Unix philosophy emphasizes simple interfaces that are intuitive to use
- Historical library loader issues
  - [Improper state management of library dependencies](#improper-state-management-of-library-dependencies)
  - [The Windows `GetModuleHandle` function is broken](#the-windows-getmodulehandle-function-is-broken)

## The Problem with How Windows Uses Libraries

A library or DLL is a [modular](https://learn.microsoft.com/en-us/troubleshoot/windows-client/setup-upgrade-and-drivers/dynamic-link-library#dll-advantages) unit of code that processes can load to use the contained functionality. A linker can connect libraries together to create dependencies between them. Defining dependencies between libraries requires conscientious management of the dependency tree to avoid creating dependency conflicts.

How the Windows operating system uses DLLs to make up the Windows API is problematic because the elements inside these modules have low [cohesion](https://en.wikipedia.org/wiki/Cohesion_(computer_science)). This disorganization results in high coupling between DLLs, causing nearly *everything to depend on everything else* (if not directly, then by proxy through a dependent DLL). Thus, a type of dependency conflict is born: a circular dependency. This combination of disorganization and close interdependency that Windows libraries exhibit dooms the modular unit of functionality a library is supposed to represent and transforms the Windows API into a monolithic beast. Moreover, the abstract interfaces exposed by the Windows API often do not clearly fit into any particular layer of the system architecture, which is problematic because Windows was never designed with a clear hierarchy or layering of its components in mind.

This issue has been apparent in Windows NT, what we know today simply as Windows, ever since its first release as Windows NT 3.1 in 1993. The Advanced Windows 32 Base API DLL or `advapi32.dll` is one module that has existed ever since the debut of Windows NT and still exists in modern Windows operating systems today. From the 1993 introduction of Windows to today, this central DLL has been responsible for [security calls](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getusernamew#requirements), [providing access to the Windows Registry](https://en.wikipedia.org/wiki/Windows_Registry#Programs_or_scripts), [managaing Windows services](https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-createservicea#requirements), and more ([all of this functionality was included in the original ADVAPI32 definition](data/windows/timeline-verification/dll-imports-exports/windows-nt-31/ADVAPI32-exports.txt)). The Advanced Windows 32 Base API DLL is a non-sensical grouping of "advanced" Windows APIs and undoubtedly exhibits [coincidental cohesion](https://en.wikipedia.org/wiki/Cohesion_(computer_science)#Types_of_cohesion), the worst type of cohesion. Interdependency between `advapi32.dll` and another new Windows component at the time that also still exists in modern Windows today, `rpcrt4.dll`, [formed a dependency cycle that has existed ever since the first Windows release in 1993](data/windows/timeline-verification/dll-imports-exports/windows-nt-31). This early cycle exemplifies the absence of architectural layering that has pervaded Windows NT from the moment it hit the market in core components that the rest of modern Windows has been built atop.

As a hack to workaround this root issue beginning with Windows 2000, Microsoft (ab)uses the ["delay loading"](#library-lazy-loading-and-lazy-linking-overview) Windows feature to stop immediate dependency loops. However, delay loading or library lazy loading is an inherently broken feature at the operating system level. Thus, delay loading only moves the issue while also creating many new ones. This delay loading hack is pervasive throughout virtually all parts of the Windows API.

We will now give a quick walkthrough of common DLLs, core to the functioning of a modern Windows system, which exemplify the problems and hacks we described:

```
> dumpbin /imports C:\Windows\System32\kernel32.dll
...

  Section contains the following delay load imports:

    RPCRT4.dll
              00000001 Characteristics
      00000001800B7A48 Address of HMODULE
      00000001800BF000 Import Address Table
      000000018009D0E0 Import Name Table
      000000018009D268 Bound Import Name Table
      0000000000000000 Unload Import Name Table
                     0 time date stamp

        0000000180025D2D   16C RpcAsyncCompleteCall
        0000000180025D09   211 RpcStringBindingComposeW
        0000000180025CF7   176 RpcBindingFromStringBindingW
        0000000180025C6C   16E RpcAsyncInitializeHandle
        0000000180025D1B    2E I_RpcExceptionFilter
        0000000180025D3F   186 RpcBindingSetAuthInfoExW
        0000000180025D87    94 Ndr64AsyncClientCall
        0000000180025D63   16B RpcAsyncCancelCall
        0000000180025D75   174 RpcBindingFree
        0000000180025D51   215 RpcStringFreeW

...
```

The most common Windows DLL after `NTDLL.dll`, `KERNEL32.dll`, contains one of these dependency hacks for loading `RPCRT4.dll`, the RPC runtime. `RPCRT4.dll` immediately depends on `KERNEL32.dll`, and Microsoft chose `KERNEL32.dll` as the DLL to break the immediate dependency loop through library lazy loading. `KERNEL32.dll` delays the loading of its `RPCRT4.dll` dependency to ensure the RPC runtime and its dependencies are not unnecessarily loaded into all processes that load `KERNEL32.dll` (which is all standard Windows processes, which excludes pico processes).

Worse, `KERNEL32.dll` immediately depends on `KernelBase.dll` starting with Windows 7, which in turn depends on `ntdll.dll`. In modern Windows, we can see `KernelBase.dll` is stuffed with delay loading hacks that lead back to an astounding 18 DLLs including: `KERNEL32.dll` (*a direct circular dependency*), `advapi32.dll`, `apisethost.appexecutionalias.dll`, `appxdeploymentclient.dll`, `bcryptPrimitives.dll`, `capauthz.dll`, `daxexec.dll`, `deviceaccess.dll`, `efswrt.dll`, `feclient.dll`, `gpapi.dll`, `mrmcorer.dll`, `ntdsapi.dll`, `sechost.dll`, `twnapi.appcore.dll`, `user32.dll`, `windows.staterepositoryclient.dll`, `windows.staterepositorycore.dll`, and `windows.storage.dll`.

Here is the same hack in the Advanced Windows 32 Base API DLL that is distributed with Windows today:

```
advapi32.dll Delay Loads:
    CRYPTSP.dll
    WINTRUST.dll
    CRYPTBASE.dll
    SspiCli.dll
    USER32.dll
    CRYPT32.dll
    bcrypt.dll
    api-ms-win-security-lsalookup-l1-1-0.dll -> sechost.dll
    api-ms-win-security-credentials-l1-1-0.dll -> sechost.dll
    api-ms-win-security-credentials-l2-1-0.dll -> sechost.dll
    api-ms-win-security-provider-l1-1-0.dll -> ntmarta.dll
    api-ms-win-devices-config-l1-1-1.dll -> cfgmgr32.dll
```

In the core remote procedure call (RPC) library, which provides inter-process and remote communication support to the operating system:

```
rpcrt4.dll Delay Loads:
    ext-ms-win-core-winrt-remote-l1-1-0.dll -> (not found)
    ext-ms-win-rpc-ssl-l1-1-0.dll -> rpcrtremote.dll
    api-ms-win-security-lsalookup-l1-1-0.dll -> sechost.dll
    SspiCli.dll
    WS2_32.dll
    IPHLPAPI.DLL
    ext-ms-win-authz-context-l1-1-0.dll -> authz.dll
    api-ms-win-security-sddl-l1-1-0.dll -> sechost.dll
    bcryptPrimitives.dll
```

And in the Shell Lightweight Utility Functions library, introduced with the second release of Windows NT:

```
shlwapi.dll Delay Loads:
    MPR.dll
    SHELL32.dll
    PROPSYS.dll
    api-ms-win-shcore-registry-l1-1-1.dll -> SHCORE.dll
    api-ms-win-shcore-registry-l1-1-0.dll -> SHCORE.dll
    api-ms-win-shcore-thread-l1-1-0.dll -> SHCORE.dll
    api-ms-win-shcore-comhelpers-l1-1-0.dll -> SHCORE.dll
    api-ms-win-shcore-stream-l1-1-0.dll -> SHCORE.dll
    api-ms-win-shcore-unicodeansi-l1-1-0.dll -> SHCORE.dll
    api-ms-win-shcore-path-l1-1-0.dll -> SHCORE.dll
    api-ms-win-shcore-obsolete-l1-1-0.dll -> SHCORE.dll
    api-ms-win-shcore-sysinfo-l1-1-0.dll -> SHCORE.dll
    SHCORE.dll
    USERENV.dll
    api-ms-win-core-com-l1-1-0.dll -> combase.dll
    OLEAUT32.dll
    api-ms-win-core-winrt-error-l1-1-0.dll -> combase.dll
    api-ms-win-core-registry-l2-1-0.dll -> advapi32.dll
    ext-ms-win-advapi32-safer-l1-1-0.dll -> advapi32.dll
    ext-ms-win-ntuser-windowclass-l1-1-0.dll -> user32.dll
    ext-ms-win-rtcore-gdi-devcaps-l1-1-0.dll -> gdi32.dll
    msiltcfg.dll
    apphelp.dll
    MrmCoreR.dll
    ADVAPI32.dll
    GDI32.dll
    ole32.dll
    SETUPAPI.dll
    USER32.dll
```

The Shell Lightweight Utility Functions library is a textbook case of coincidental cohesion.

Practically every notable DLL in the Windows API is swamped with circular dependencies, typically covered up by delay loading hacks. Specifically, there are ~3000 DLLs in the `C:\Windows\System32` of a modern Windows system (3144 exactly in [this measurement](code/windows/scripts/dll-deps-research/get-resource-dll-count.ps1), not including subdirectories or resource-only DLLs). Of those DLLs, **over half** (1663 exactly) exhibit a delay loading hack. Additionally, the remaining DLLs that do not directly have a delay loaded dependency often still trigger delay loading through a transitive dependency on a delay loaded DLL. A comprehensive [list of directly affected DLLs](data/windows/dll-deps-research/delay-loads.txt) is available.

High cohesion and low coupling is a staple of good API design. The Windows API exhibits the opposite of this design principle with low cohesion and high coupling. One of the many consequences of these undesirable traits is the creation of dependency cycles because: low cohesion + high coupling = circular dependencies.

Ensuring the simplest case does not require the overhead of the most complex case is another key tenant of API design. Therefore, loading the libraries of many subsystems for even the simplest "Hello, World!" class of applications would be an unwelcome trait for performance and resource utilization. So, to improve process creation time and reduce memory usage in later Windows versions while working in the constraints of low cohesion and high coupling, Microsoft introduced delay loading to load dependencies on the first call into libraries. Delay loading worked as a quick fix to help keep the number of loaded libraries down and, as a bonus, gave the loader a clear order for initializing libraries in.

Circular dependencies, compounded by delay loading, comes with a myriad of consequences for the operating system, the software running on it, its developers, and its users. Continue to [*Dependency Breakdown*](#dependency-breakdown) and [*The Lazy Loading Liability*](#the-lazy-loading-liability) to learn about these poor outcomes.

For further research on Windows' misuse of DLLs, [see here](#further-research-on-windows-usage-of-dlls).

## Dependency Breakdown

**NOTE:** Work in progress. Not in the final state. Due not take anything here as done or complete.

A circular dependency is when two or more components depend on each other in a loop. This cycle is an anti-pattern because it creates a feedback loop between the affected components leading to many negative consequences for a system. Some of these well-known adverse impacts include having a false sense of modularity, no defined initialization order, and decreased predictability in how the complexity introduced by a dependency loop will cause a system to behave. These poor outcomes can reveal themselves in several ways, including complications such as:

### Modularity

The vast quantity of circular dependencies all through out the DLLs that make up the Windows API breaks the vital and commonly ascribed modularity benefit of the DLL.

(expand on this lots... how it impacts everyone, including windows devs)

### Fearful Concurrency

Rust does not allow circular dependencies between packages or executable modules. Attempting to create a circular dependency between libraries in Rust will explicitly raise a blocking compliation error:

```
> cargo build
...
error: cyclic package dependency: package `package_a v1.3.1` depends on itself. Cycle:
package `package_a v1.3.1`
    ... which satisfies dependency `package_a = "^1.3.0"` of package `package_b v4.0.8`
    ... which satisfies dependency `package_b = "^4.0.8"` of package `package_a v1.3.1`
```

Rust does not allow dependency cycles for multiple reasons, including:

1. Determining build order
    - The Rust package manager, Cargo, resolves dependencies using a directed acyclic graph (DAG). A cycle in dependencies would break this structure, making it impossible for Cargo to determine a build order.
2. Version resolution issues
    - If different package versions are specified by the packages with the dependency loop then Cargo cannot determine which version to use.
3. Borrow checker ownership and lifetime issues
    - On Rust, cyclic references across crates can create data ownership conflicts by making variables own each other in a loop thus creating infinitely recursive types if the cycling packages define types that contain each other, logically impossible lifetime requirements in cyclic borrowing cases because borrowing variables must outlive each other, and more issues.
4. Best practices
    - Acyclic dependencies are the best practice for modularity.

If Rust was able to build a package with a circular dependency, which it cannot, run-time issues such as lock order inversion leading to ABBA deadlock could happen uncontrollably between independent crates thus breaking the fearless concurrency model that Rust is known for.

All together, circular dependencies between libraries make code incompatible with modern, memory-safe system languages and serve as a significant "rewrite it in Rust" blocker for unsafe C code.

### Dangerous Destructors

The existence of circular dependencies between modules makes it impossible for the affected modules to run their module destructor routines without possibly calling into a dependency or using a resource from a dependency that has already undergone its module destruction. Calling into an uninitialized library has undefined behavior that could cause a crash or practially any outcome. The hacks Microsoft commonly employs to fix module constructors does not extend to module destructors because all of the libraries are already loaded. A consequence of this fact is that effectively makes running any finalization code beyond what can certainly be ascertained to only depend on NTDLL, typically called into from a public wrapper function (e.g. `CloseHandle`), unsafe to do from a module destructor if that module is circularly depended on.

### Non-Evictable Libraries

Blocks Library Unloading, permenatnely loaded

Essentially would require a tracing algorithm?

### Reference Cycles Overhead

Windows internally does extra ref counting in some places as a workaround where the two-step initializer tries to carefully to avoid circularly depending on itself -> reference cycles can still break reference counting meaning finalization will never happen

Rust rc arc for ref cycle, does not work well with times

Rust will not block, with a compilation error, reference cycles between variables like it will for modules

### Page Fault Surplus

Windows DLLs form a highly tangled and interconnected web of dependencies. Once Windows APIs are called for the first time, their lazy loads will be resolved thus loading many libraries into a process that will stay in its memory until that process exits. With lazy loads resolved, this optimization has surpassed its ability to be a useful as this point. The large quantity of memory mapped regions in the form of libraries can decrease memory efficiency thus leading to higher turnover in the memory pager and an increased page fault rate. Page faults force the operating system to swap memory from disk I/O thus resulting in excessive storage reads and writes, slowing down performance.

### Hidden Dependencies

Before the introduction of lazy loading (and occasionally still in modern Windows), Microsoft commonly employed dynamic loading from a DLL's `DLL_PROCESS_ATTACH` or module constructor to control the initialization order of circular dependencies. Examples of this include, in modern and legacy Windows, [when `user32.dll` dynamically loads `imm32.dll` from its `DLL_PROCESS_ATTACH`](https://github.com/reactos/reactos/blob/656a5dcc1fd6fac455a469a39383ceeb4533f1f7/win32ss/user/user32/misc/imm.c#L86-L93); `user32.dll` does not declare this dependency in the import table and `imm32.dll` holds a circular dependency on `user32.dll`. When used beyond its intended use case for loading extension libraries, dynamic loading is poor practice because it gives a DLL hidden dependencies that cannot be known until application run-time. Hidden dependencies are bad because they increase the chance of accidentally forming dependency cycles and are incompatible with dependency scanning for use by package managers, security scanners, and other tools. Dynamic loading and consequently locating symbol addresses at process run-time will also give worse performance than doing these operations at process load-time.

### Vendor Lock-In

The tight coupling that led to circular dependecies provide makes Windows harder to rip off. Hugely monolithic API is hostile design. Circular dependencies allowed Microsoft to move fast and break things. While circular depenendencies were not intentionally introduced to make the Windows API harder to copy, they could serve a business purpose as they exist now. Closely integrate components makes MS harder to separate (US v. Microsoft "Browser tie in"). Harder and more arguous to create an alternative implementation due to tight coupling. Short-term business advantage

### Repairability

Repairability or debugging

Modularity is important in software as it is in hardware. Consumers should advocate for Unix.

### Disrupts Per-Module Synchronization for Library Initialization

In a hypothetical loader that implements [fine-grained per-module synchronization for its initialization and finalization stage](#exploring-fine-grained-library-initialization-thread-safety), the existence of circular dependencies makes the fine-grained design no better than a broadly synchronizing loader by introducing potential deadlocks and reducing concurrent performance.

### Conclusion

Windows accepted circular dependencies in its core components ever since its original release. No general-purpose operating system that ships with such architectural flaws in its foundational layers can reasonably be attributed as having a competent design. For circular dependencies replace the architectural layering that would constitute real design with a spaghetti architecture. Windows NT was—and remains—an example of systematic design failure.

## The Lazy Loading Liability

**NOTE:** Work in progress. Not in the final state. Due not take anything here as done or complete.

[Library lazy loading](#library-lazy-loading-and-lazy-linking-overview) is an anti-feature that devalues dynamic-link libraries (DLLs), disrupts good patterns, destroys an operating system, and breaks everything it touches.

### DLL Devaluation

The use of DLLs is commonly prescribed, by Microsoft and others, as having [multiple notable advantages](https://learn.microsoft.com/en-us/troubleshoot/windows-client/setup-upgrade-and-drivers/dynamic-link-library#dll-advantages) over statically linking libraries—which it indeed does. Lazy loading chips away at the very libraries it exists to load by eroding every one of the benefits associated with DLLs:

#### DLL Upgradability

Prescribed DLL benefit: "Eases deployment and installation"

Lazy loading can secretly cause a running application to load a library at any time during its execution. The DLLs that existed on disk when the application started may not be the same DLLs that exist on disk when a lazy load occurs at an unknown point in the future. As a result, deploying a new version of a lazy loaded DLL while a system is running can break running applications. This outcome hurts ease of deployment by enforcing a full system restart to deploy new DLLs when rebooting would have otherwise not been necessary if applications loaded their dependencies at process load-time.

The Windows API is composed of thousands of closely knit DLLs, between which it would be easy for the described scenario to unfold between any one of them. In particular, an internal Windows API DLL that applications do not directly depend on are at risk of receiving a breaking change during an upgrade thus sabotaging all running processes on the system. Indeed, it is this danger, stemming from lazy loading, that is responsible for the infamously dreaded and agonizing [Windows update process](https://www.youtube.com/watch?v=Ho3Lw90kzrY) that entirely locks up a user's computer, thereby increasing system downtime, until it is complete.

#### Modularity

Prescribed DLL benefit: "Promotes modular architecture"

Lazy loading is an optimization that encourages giving libraries more dependencies by allowing those dependencies to be loaded only when needed. Each additional dependency a library has, whether lazy or not, reduces the effective modularity of that component. If any dependency of a library is missing upon its use then that library will typically not be able to function correctly. In particular, how Microsoft uses lazy loading as a compensating factor for circular dependencies eliminates all library modularity benefits.

#### Uses fewer resources

Prescribed DLL benefit: "Uses fewer resources"

Putting off when a DLL is loaded is a worse solution for minimizing the resource usage of libraries than simply reducing the number of DLL dependencies. Taking away library dependencies, merging them when appropriate, is the best solution for not only reducing resource usage but also improving performance: [GNU is taking precisely this approach to speed up process creation](https://developers.redhat.com/articles/2021/12/17/why-glibc-234-removed-libpthread).

### Breaks Module Initialization

Presumably, one of Microsoft's primary motivators for adding library lazy loading and heavily utilizing it throughout the Windows API, other than a short-sighted aim to improve process startup performance, was restoring an order for modules that have dependency cycles to begin their initialization phase in. The goal was to have a predictable initialization order. Although, library lazy loading falls short of even that goal in the case that a module accidentally triggers a lazy load from its module initialization routine, potentially before fully initializing itself while the now lazy loading library holds a circular dependency (directly or indirectly) on the former library.

Ironically though, the introduction of library lazy loading creates a much greater issue than determining the order to start module initialization in because an outcome of lazy loading is that module initialization code can interrupt code execution any time one DLL calls into another DLL. Thus, lazy loading trades an unpredictable initialiazation order between modules for an unknown module initialization time. The unknown time of when a module initialization routine could be running *significantly* restricts what can safely be done in a module initialization routine, practically reducing it to use for only the most primitive initialization tasks or tasks that should have been possible to evaluate at compile time, anyway. The foremost technical reason underlying this outcome is that interrupting code execution at an unknown time can easily mix up lock acquisition order, thus resulting in lock hierarchy violation and consequently [ABBA deadlock](#abba-deadlock).

Winhttp example

Interrupting code execution to run other code is hardly ever safe and comes with a barrage of consequences for what can safely be done during module initialization.

As a workaround, two-phase init

Cross-cutting concerns makes and constant perf cost makes lazy init bad

### Dangerous Destructors

The Windows loader runs module destructors by simply walking the `PEB_LDR_DATA.InInitializationOrderModuleList` linked list backwards. Lazy loading causes a dependency to be loaded after the module that depends on it has already loaded. As a result, lazy loading will cause the Windows loader to run the dependency's module destructor before the destructor of the module that takes a dependency on the lazy loaded module.

Microsoft could be fix this issue by using their new `LDR_DDAG_NODE` structure in the loader instead of their current naive approach that can run module destructors in an incorrect order due to violating the formation of the dependency graph. However, it seems Microsoft has given up on trying to make module destructors not broken.

### Breaks Dynamic Linking of Data Symbols

Library lazy loading is incompatible with dynamically linking to data symbols, thus breaking libraries that wish to use the fastest, most robust and modular method for sharing data between modules. As a workaround, libraries must expose dummy functions that do nothing but return a pointer to some data. This added layer of indirection often results in artifically enforced dynamic initialization when initialization could have been done earlier in the dynamic linking phase. Another outcome of library lazy loading sabotaging data symbols is that it strengthens to the incentive to [centralize data in one relatively quick and easily to access data structure](#the-peb-problem).

### Security

Timing attack. [Sizable time window](data/windows/loadlibrary-trace.log#L496-756) and if the operation is offloaded to a loader worker thread then that thread is responsible for resetting memory protection on the loading module's IAT after the thread that began the library load originally made the loading module's IAT writable. Timing attack is definitely feasible. Overwriting these code pointer is a CFG bypass because CFG only protects indirect calls by default (there exists support for protecting calls to library exports using CFG, but all binaries must be built to support this and in practice nobody does it including throughout the Windows system libraries; it would probably also tank performance).

`QueryOptionalDelayLoadedAPI` + DLL load locations causes real vulnerabilities. Lazy loading results in using the feature as a version check or to check for extensions. DLL could be hijacked.

### Performance

When lazy library loads are resolved then resource usage will increase causing more page faults than if there were fewer library dependencies

### Introduces Unexpected Failure Points

If a library is not present or otherwise inaccessible when the lazy load occurs at an undefined point during an application's run-time then the process will have no choice but to exit abruptly. Implementing any form of error handling on whether each call into a lazy loaded DLL fails, in case a given call is first call into that DLL thus triggering a lazy load, would be infeasible. This ever-present possiblity of failure effectively turns any library function into a [partial function](https://wiki.haskell.org/Partial_functions) where in this context, the function would become undefined for the implicit argument that states the required library is not accessible on disk.

### Disadvantages Static Linking

Static linking is useful for creating portable software applications when the exact host operating system is unknown. Builing an application to statically link with its dependencies would have to include lazily loaded dependencies. Lazy loading encourages giving modules more dependencies thus leading to larger binary sizes in a statically linked application. In addition, using lazy loading as hack for controlling library initialization order precludes compatability with static linking because dependencies will all be loaded at the same time as one bundle.

In the context of Windows, virtually every DLL depends on every other DLL, so statically linking would essentially mean providing the full Windows API in one executable. However, Microsoft does not tend to statically link its libraries and the practice is not is not permitted by the license or doable without source code access. Still, there could potentially be open source projects from Microsoft or other vendors that use lazy loading to the disadvantage of static linking.

### Operating System Destruction

Lazy loading makes the first call into a library is asynchronous signal unsafe. Any call into a library me be async signal unsafe then.

Breaks custom calling conventions.

lazy loading liability

### Conclusion

Microsoft added module lazy loading to Windows because it worked as a quick fix to help alleviate some of the negative outcomes that stem from a poorly designing an API that has low cohesion and high coupling. However, arbitrarily postponing when to load code is an untenable feature for all the reasons we described here—especially at the system-level. It is for this this same reason that [Apple made the decision to axe lazy loading from its Xcode linker](https://developer.apple.com/forums/thread/131252)—although lazy loading was at one point added to the MacOS development tools due to mistaking it as a feature, Apple never utilized the anti-feature throughout its system components as Microsoft did with Windows. But, the core system components that made up the API of Windows NT were fundamentally broken ever since they were introduced, which made the addition of lazy loading somewhat inevitable as the system grew with everything continuing to depend on everything else. As a result, Windows will likely have to keep its lazy loading baggage indefinitely to stay compatible with its own complete design failure. For everyone else with an interest in good computer systems, let's continue to keep lazy loading out of our operating systems and subsystems, and support projects and products that have a modular design.

## Further Research on Windows' Usage of DLLs

### The DLL Host

A DLL or library is modular code that processes can load to use the contained functionality. If this was the extent to how Windows, like any other operating system, utilized DLLs, then it would be correct. However, Windows' usage of DLLs goes far beyond their intended use. Introducing, the DLL host.

In Windows, DLL hosts are programs that serve only to host other DLLs that provide the core functionality of an application or service. Common DLL hosts include `rundll32.exe`, `svchost.exe`, `taskhostw.exe`, and [COM surrogates](https://learn.microsoft.com/en-us/windows/win32/com/dll-surrogates) such as `dllhost.exe`.

DLL hosts are prevalent throughout Windows, with `svchost.exe` alone accounting for **over half** (55% or 70/126 processes by [my measurement](data/windows/dll-deps-research/process-count.ps1)) of all proceses on the system upon booting up Windows.

Clearly, Windows really likes DLL hosts and specifically [shared service processes](https://en.wikipedia.org/wiki/Svchost.exe). But why? No other operating system has the concept of a DLL host and they seem to get along just fine.

Well for a start, we know processes are more expensive on Windows than on Unix systems. Looking at the `Private Bytes` consumed by even the most minimal of proccesses in Process Explorer verifies this to be the case:

```
AggregatorHost.exe |  912K
smss.exe           | 1072K
svchost.exe        | 1284K
svchost.exe        | 1292K
svchost.exe        | 1384K
```

Even the smallest processes are eating up about 1 MiB or more of memory each! The plethora of highly interconnected DLLs making up the Windows API would also certainly certainly contribute to slower process start times.

Going further, another reason for shared services could be that being in the process allows for faster communication between similar services (especially since the base overhead of a Windows system call as well as the system calls themselves are generally known to be higher on Windows than on Unix systems). This was the same motivation for in-process COM servers. By running `tasklist /svc | findstr ,`, we can find shared service hosts containing multiple service:

```
lsass.exe                      860 KeyIso, SamSs, VaultSvc
svchost.exe                    984 BrokerInfrastructure, DcomLaunch, PlugPlay,
                                   Power, SystemEventsBroker
svchost.exe                    928 RpcEptMapper, RpcSs
svchost.exe                   2672 BFE, mpssvc
svchost.exe                    456 OneSyncSvc_59a4b,
                                   PimIndexMaintenanceSvc_59a4b,
                                   UnistoreSvc_59a4b, UserDataSvc_59a4b
```

That's interesting, out of all the shared service processes, **only five (including `lsass.exe`) are actually hosting multiple services in one process**. This is in stark contrast to [the large number of unrelated services that previous Windows versions packed into one process](https://web.archive.org/web/20190428105316/https://blogs.msdn.microsoft.com/larryosterman/2005/09/09/shared-services/):

```
1280 svchost.exe Svcs: AudioSrv,BITS,CryptSvc,Dhcp,dmserver,ERSvc,EventSystem,helpsvc,lanmanserver,lanmanworkstatio
n,Netman,Nla,RasMan,Schedule,seclogon,SENS,SharedAccess,ShellHWDetection,srservice,TapiSrv,Themes,W32Time,winmgmt,wuause
rv,WZCSVC
```

The simplest explanation for Microsoft no longer packing many services into one process like they use to is valuing the robustness of a separate virtual address space for each service over the expense that comes with it. One megabyte of memory, while not nothing, isn't nearly as valuable as it was when the average system was sporting fewer gigabytes of RAM than it was today. As a result, Windows shared services mostly appears to be a relic of the past and I wouldn't be surprised if Microsoft does away with them entirely at one point. Said in another way, using a separate process for each service brings Windows closer to [microservice architecture](https://azure.microsoft.com/en-ca/solutions/microservice-applications) because "one component’s failure won’t break the whole app" (broadly—the term microservice can take on more meaning in the cloud context).

Beyond robustness, multiple DLLs operating independently in a process with their own threads could actually hurt performance by causing unnecessary contention on in-demand resources like the process heap lock. Windows DLLs or threads sometimes use a private or local heap to help with this issue (see heaps in Windbg with `!heap` command). However, Windows API calls that create heap allocations implictly often makes full heap separation unattainable in practice. Concerns regarding process heap lock contention are especially pertinent because the Windows NT Heap implementation doesn't implement any measure to reduce blocking like the [glibc heap does with per-thread arenas](https://elixir.bootlin.com/glibc/glibc-2.38/source/malloc/malloc.c#L1) (and Microsoft's attempts at implementing a more concurrent and performant heap, [like the Segment Heap](https://github.com/microsoft/Windows-Dev-Performance/issues/39#issuecomment-729313323), [have not worked out](https://github.com/microsoft/Windows-Dev-Performance/issues/106)).

Another victim of the DLL host that cannot go understated is ease of debugging. There will always be bugs, so it's crucial to be proactive in maximizing correctness and minimizing complexity so bugs can be fixed as quickly as they're spotted. A DLL host stands in the way of debugging for multiple reasons, most obviously, a shared address space makes determining the source of memory corruption bug challenging if multiple compnents or services operate in a single address space. But also, [Microsoft won't be able to send Windows Error Reporting (WER) reports for crashes](https://devblogs.microsoft.com/oldnewthing/20130104-00/?p=5643) because that's tracked by the EXE hosting the DLL (as well as other notable concerns like making application compatibility more difficult).

Shared service processes use service DLLs. Since a service DLL exists solely for the purpose of allowing multiples services to exist in one process, one would not expect DLLs to take a dependency on a service DLL. A service DLL is more like an EXE in that `svchost.exe` delegates control of the application lifetime to it. So, depending on a DLL that works like its an EXE is surely a recipe for circular dependencies, which are bad. Alas, upon searching, [I did find some DLLs depending on service DLLs](data/windows/dll-deps-research/dlls-depending-on-service-dlls.txt) (this search only being in `C:\Windows\System32`, not including subdirectories).

Once again, DLLs provide a false promise of modularity.

### DLL Procurement

Windows will load and execute a DLL from practically anywhere, which as you can imagine, does not fare well for the security of the operating system and frequently invents security vulnerabilites that could never exist on other systems.

[See here for more information.](#library-loading-locations-across-operating-systems)

### One DLL, One Base Address

Today, Windows still does not support per-process address space layout randomization (ASLR) of libraries. It's absence effectively makes this crucial exploit mitigation useless for defending against privilege escalation, including sandbox escape (e.g. from a web browser), on Windows. This weakness markedly tips the scales in favor of the attacker (e.g. in a ROP attack).

This requirement exists because how Windows works, I believe in relation to the operating system's heavy usage of shared memory and historical reasons, mandates all image mappings to be at the same address in virtual memory across processes.

[See here for more information.](https://cloud.google.com/blog/topics/threat-intelligence/six-facts-about-address-space-layout-randomization-on-windows/#:~:text=Fact%202%3A%20Windows%20loads%20multiple%20instances%20of%20images%20at%20the%20same%20location%20across%20processes%20and%20even%20across%20users%3B%20only%20rebooting%20can%20guarantee%20a%20fresh%20random%20base%20address%20for%20all%20images)

### DLLs as Data

Microsoft confused memory-mapped files with libraries thus giving us the [resource-only DLL](https://learn.microsoft.com/en-us/cpp/build/creating-a-resource-only-dll).

Turning a pointer into a search through a lookup table for that pointer is a diabolical level of bloat.

[See here for more information.](#loadlibrary-vs-dlopen-return-type)

## Library Loading Locations Across Operating Systems

The Windows loader searches for DLLs to load in a [vast (and growing) number of places](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-search-order#standard-search-order-for-unpackaged-apps). Strangely, Windows uses the `PATH` environment variable for locating programs (similar to Unix-like systems) as well as DLLs. Microsoft's decision to retain the current working directory ("the current folder") in this list of places is an accident (or worse, a security incident) waiting to happen, particularly when running applications from untrusted CWDs in a shell (e.g. CMD or PowerShell). Using the current folder as a search location for modules of code is rooted in the CP/M origins of MS-DOS because CP/M did not have a hierarchical filesystem. However, Microsoft remains accountable because they could have yet did not phase out the dated functionality while maintaining application compatibility through versioning. The DLL search order Microsoft documentation still does not cover all the possible locations though, because while debugging the loader during a call to `LoadLibrary`, I saw `LdrpSendPostSnapNotifications` eventually calls through to `SbpRetrieveCompatibilityManifest` (this is not part of a notification callback). This `Sbp`-prefixed function searches for [application compatibility shims](https://doxygen.reactos.org/da/d25/dll_2appcompat_2apphelp_2apphelp_8c.html) in SDB files which [may result in a compat DLL loading](https://pentestlab.blog/2019/12/16/persistence-application-shimming/). Also to do with application compatibility, [WinSxS and activation contexts](https://learn.microsoft.com/en-us/windows/win32/sbscs/activation-contexts) (DLLs in `C:\Windows\WinSxS`) exist to load versioned DLLs typically based on the [application's manifest](https://learn.microsoft.com/en-us/windows/win32/sbscs/application-manifests) (these are usually embedded in the binary). A process calling the `CreateProcess` family of functions or `WinExec` is subject to loading [AppCert DLLs](https://attack.mitre.org/techniques/T1546/009/). When secure boot is disabled in Windows 8 or greater, [AppInit DLLs](https://learn.microsoft.com/en-us/windows/win32/dlls/secure-boot-and-appinit-dlls) can load DLLs into any process. The plethora of possible search locations contributes to [DLL Hell](https://en.wikipedia.org/wiki/DLL_Hell) and DLL hijacking (also known as DLL preloading or DLL sideloading) problems in Windows, the latter of which makes vulnerabilities due to a privileged process accidentally loading an attacker-controlled library more likely. Tripping up on this footgun in Windows and other Windows-specific security weaknesses happens all the time especially—speaking from experience—in line of business (LOB) applications that enterprises use.

On GNU/Linux, the trusted directories for loading libraries can be found in the [`ldconfig` manual](https://man7.org/linux/man-pages/man8/ldconfig.8.html):

```
/lib
/usr/lib
/lib64
/usr/lib64
```

Additonal library loading locations can be added by modify the `ld.so.conf` configuration file or adding a configuration file to `ld.so.conf.d`.

Beyond that, one can use the `LD_LIBRARY_PATH` environment variable to choose other places the loader should search for libraries, and `LD_PRELOAD` or `LD_AUDIT` to specify libraries to load before any other library (including `libc`) with the difference being that libraries specified by the latter run first and can receive callbacks to monitor the loader's actions such as symbol resolution. For security, loading libraries based on environment variables is always disabled for `setuid` binaries. Binaries can include an `rpath` to specify additional run-time library search paths.

## `LoadLibrary` vs `dlopen` Return Type

On Windows, `LoadLibrary` returns an officially opaque `HMODULE`, which is implemented as the base address of the loaded module. Windows searches for this module handle in a lookup table to obtain a pointer to that module's `LDR_DATA_TABLE_ENTRY`. A pointer was made for pointing, so this extra layer of indirection amounts to nothing more than bloat on top of a pointer with no benefit.

In POSIX, [`dlopen` returns a symbol table handle](https://pubs.opengroup.org/onlinepubs/9699919799/functions/dlopen.html#tag_16_95_04). On my GNU/Linux system, this [handle is a pointer to the object's own `link_map`](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/dl-sym.c#L152) structure located in the heap. (The returned handle is opaque, meaning you must not access the contents behind it directly since they could change between versions and it is implementation-dependent; instead, only pass this handle to other `dl*` functions.)

How cool is that? That's like if Windows serviced your `LoadLibrary` request by handing you back a pointer to the module's `LDR_DATA_TABLE_ENTRY`!

Microsoft's reasoning behind the return type of `LoadLibrary` stems from the 16-bit Windows API (going back to Windows 1.0 when the function was introduced), where [libraries became conflated with data files or memory-mapped files](https://learn.microsoft.com/en-us/previous-versions/ms810501(v=msdn.10)#:~:text=Whereas%20Windows%203.1%20needs%20to%20copy%20data%20from%20an%20executable%20file%20into%20main%20memory%20to%20build%20code%20segments%20or%20load%20resources%2C%20Windows%20NT%20only%20needs%20to%20create%20a%20memory%2Dmapped%20file%20object%20that%20is%20backed%20by%20the%20executable%20file%20instead%20of%20the%20system%20pagefile.) meaning "libraries" could exist for the sole purpose of containing data with no code. In the first release of Windows NT (i.e. Windows NT 3.1), Microsoft carried forward this unique mistake while also [adding an extension for specifying that a "library" must load only as a memory-mapped file](https://devblogs.microsoft.com/oldnewthing/20141120-00/?p=43573) and how to open this file. Consequently, the modern Windows loader is stuck with maintaining a red-black tree at `ntdll!LdrpModuleBaseAddressIndex` for speeding up base address ➜ `LDR_DATA_TABLE_ENTRY` lookups (the legacy Windows loader slowly [iterated the `InLoadOrderModuleList` linked list](https://github.com/reactos/reactos/blob/053939e27cbf4d6475fb33b6fc16199bd944880d/dll/ntdll/ldr/ldrutils.c#L2066-L2100) in the PEB to do these lookups). This indirection is a contibuting factor to slow process creation on Windows (simply set a read watchpoint on `ntdll!LdrpModuleBaseAddressIndex` during process startup to see what a hot data structure this is). The performance of Windows delay loading is also negatively affected by this indirection, and the synchronization it required to access the shared data structure, because `ntdll!LdrResolveDelayLoadedAPI` calls `ntdll!LdrpFindLoadedDllByHandle` every time it runs.

Given any amount of forethought, it is reasonable to expect that Windows' focus on dynamic loading of [executable modules](data/windows/timeline-verification)—unlike the prevalent static linking of the time—would have ensured a solid design for its core library loading API. Especially since [Multics](https://en.wikipedia.org/wiki/Multics) had already pioneered the idea of dynamic linking, [used memory-mapped files to share data](https://www.multicians.org/myths.html#nofile), and [supported dynamic loading](https://www.multicians.org/mspm/be-7-07.661209.loader.pdf) years before Microsoft was even founded. So, no hindsight was required to avoid this mistake but rather only minimal API design planning in regards to the end goal of dynamic linking, under which circumstance it would have made no sense for `LoadLibrary` to return the base address of the loaded library in memory. Indeed, the module handle implementation quirk in the library loader exemplifies an avoidable misstep—one that, like most Windows API oversights, exists as an outcome of Microsoft's expedient development style when creating their core technologies.

For new code, Microsoft could fix this issue by introducing a new set of library loader functions including `LibraryOpen` and `LibraryClose` (these names would also be more accurate since loading a library simply increases its reference count if a given library is already loaded, similar to the well-named `fopen` and `fclose` file functions in the standard C API); however, the Windows loader would internally still have to expend effort maintaining the legacy data structures for compatibility with the older library loader functions, at least in a compatability mode. These are changes that should have happened in the transition to Windows NT, with the old `LoadLibrary`, `FreeLibrary`, and other functions being designated as existing only for compatibility with 16-bit Windows (as Microsoft does in other places), but there is no time like the present.

An excerpt from *Windows Internals: System architecture, processes, threads, memory management, and more, Part 1 (7th edition)* states this regarding the `ntdll!LdrpModuleBaseAddressIndex` data structure (and `ntdll!LdrpMappingInfoIndex`):

> Additionally, because lookups in linked lists are algorithmically expensive (being done in linear time), the loader also maintains two red-black trees, which are efficient binary lookup trees. The first is sorted by base address, while the second is sorted by the hash of the module’s name. With these trees, the searching algorithm can run in logarithmic time, which is significantly more efficient and greatly speeds up process-creation performance in Windows 8 and later. Additionally, as a security precaution, the root of these two trees, unlike the linked lists, is not accessible in the PEB. This makes them harder to locate by shell code, which is operating in an environment where address space layout randomization (ASLR) is enabled.

While the message on performance is a true and prudent point to make, I also find that statement alone lacks relevant perspective on the fact that `ntdll!LdrpModuleBaseAddressIndex` only exists to begin with as a workaround for Microsoft's blunder with the `LoadLibrary` function API. The point regarding security is dubious because if the module linked lists are already in the PEB, one of which must, in practice, remain there indefinitely for backward compatibility since [Microsoft chose to share one these lists in the public `winternl.h` header](https://learn.microsoft.com/en-us/windows/win32/api/winternl/ns-winternl-peb_ldr_data), then excluding the red-black trees has no effect because security comes down to the lowest common denominator. The background information on trouble that arises from module linked lists residing in the PEB is nice (of course, there are a variety of ways to find other modules in the process but those methods would be a bit "harder" and likely not universal). Again though, there is a more relevant point to make that is not addressed by the book especially since it does cover the associated Windows history in some places, just not here.

## Exploring Fine-Grained Module Initialization Thread Safety

Traditionally, the loader protects its data structures and the initialization of all modules with a single global lock. Here, we will explore the idea of splitting up this lock to achieve [deadlock avoidance](code/glibc/dlopen-thread-join-dlopen/README.md) and boost concurrent loader performance.

The first part of this plan would be decoupling library mapping and snapping/linking from the initialization of that module (these operations are coupled under the `ntdll!LdrpLoadCompleteEvent` lock in the current modern Windows loader design). These two pieces would take place under separate locks that avoid nesting which each other: A mapping and snapping lock (like what already exists in the Windows loader with `ntdll!LdrpWorkCompleteEvent`) and the initialization and finalization lock for all modules.

Now, we can work on splitting up the initializtion/finalization lock for modules. This means turning the single global module initialization/finalization lock into a initialization/finalization lock for each module. In other words, we want module initialization/finalization synchronization to operate at a per-module granularity.

The idea is that modules should be able to initialize concurrently except when there are shared uninitialized dependencies between two or more dependency chains. During initialization of a given module, the loader would acquire the initialization/finalization lock for a given module, run the initialization for the module, mark the module as initialized, then release that lock.

**Potential Problems & Solutions:**

1. Circular dependencies
    - A [directed acyclic graph (DAG)](https://en.wikipedia.org/wiki/Directed_acyclic_graph) dependency formation is a must-have for any loader with fine-grained initializer syncrhonization to work and be useful
    - As long as dependencies are acyclic, an initializer deadlock can never happen because the intializer locks are being acquired in one direction
      - Some initializers may have to yield to other initializers, and this process would have to be opimized with initialization order algorithms to ensure yielding is minimized for perfomance, but no depedency cycles means the locks will always unlock
    - To work around circular dependencies, a loader with fine-grained initializer syncrhonization would have to recognize the circular dependency while still under protection of the lock used for protecting the loader's internal data structures then, when the initialization stage occurs, lock all the libraries that circularly depend on each other while running any of their initializers
      - These locks would have to be acquired in an arbitrary but agreed-upon order like which library's name comes dirst according to the codepoint of each character (e.g. on an ASCII table)
    - **Best solution:** Block circular dependencies from loading thereby stopping the issue at the source (e.g. Rust blocks the creation of circular dependencies)
2. Library initializer nesting the mapping and snapping lock due to performing a dynamic library load
    - The mapping and snapping lock must be kept at the bottom of any lock hierarchy (only above than the heap lock and PEB locks, since the Windows loader currently requires these locks for mapping and snapping to take place)
    - The start of a library load operation should check if it is being nested inside a library mapping and snapping operation by some detoured or hotpatched code, and if so fail, to enforce the lock hierarchy
    - Suboptimal for performance in a loader with fine-grained synchronziation
3. Library initializer nesting another library initializer lock due to performing a dynamic library load that reenters the loader
    - Exercise
      - Consider that "Library A" depends on "Library B" because the library initializer of "Library A" dynamically loads "Library B"
      - In the non-reentrant case where "Library A" depends on "Library B" via dynamic linking, "Library B" would initialize before "Library A"
      - In the reentrant dynamic loading case we have here "Library A" starts initializing before "Library B" (the reverse)
      - But, as long as "Library B" does not depend on "Library A" (i.e. there is no circular dependency), a "Library B" initializer will never try to acquire a "Library A" initializer lock, therefore no lock order inversion can occur, so we are safe
    - Suboptimal for performance in a loader with fine-grained synchronziation
4. Library load and free per-library locking requirement
    - Library deinitializers must run in the opposite order of library initializers, but if we lock and unlock before and after a library each individual library initialization/deinitialization in DAG formation then that is not a problem
    - Library free would start at the top of the given dependency chain. It would acquire the first per-library lock at the top of the chain. It would decrement the library's reference count. If it hit zero, it would deinitialize that library. We unlock the per-library lock. We repeat this process for dependencies of that node. We avoid a race condition by reference counting on a node-by-node basis so we can hold the per-library lock while decrementing the library's reference counter then possibly deinitializing that library in one protected breath.
    - Libraries could safely load or free a library from their own library deinitialization routines (this means lock nesting so care is required) as long as the loaded or freed library does not create a dependency cycle and is not unloading a library that it does not own a reference to, respectively
5. Performance considerations
    - Provides greater concurrency and could therefore improve overall performance depending on the workload
    - Slightly reduces single-threaded performance due to an increase in synchronization overhead (probably negligible)
    - Parallelized library initialization: Per-module initialization could be parallelized for independent chains of libraries (if this extra feature was desired)

Per-module synchronization is righteous because it empowers the module—the basic building block from which all other code stems. Ideally, a module should be completely abstracted from implementation details of the loader, and per-module synchronization for initialization code pushes us further towards the goal of truly modular development.

**Side Note:** Implementing a hypothetical per-routine initializer/finalizer synchronization model or splitting the initializtion/finalizaition lock so each corresponding pair of initialization/finalization routines (e.g. something like the the routines listed in the `.init_array` and `.fini_array` sections on Unix systems) has its own lock is not possible because each routine depends on the last routine having completed in series.

## The Problem with How Windows Uses Threads

**NOTE:** This section contains incomplete work and is subject to change. The middle part where I get into problematic examples has not been written yet.

A thread is the smallest unit of execution managed by an operating system. It runs code independently, sharing an address space with other threads in the same process. A process controls the lifetime of its containing threads, when the process exits, so do all its thread. The main thread of a process is the first thread in the process, which is typically the thread responsible for process exit. Thus, creating a new thread requires coordinating with the main thread to ensure all threads complete their work before exiting.

How the Windows operating system uses threads is problematic because the Windows API does not impose a synchronization model for managing the lifecycle of threads in the scope of a process. Rather, Windows often controls thread lifetimes by leaving them created then abruptly terminating all but the exiting thread at process exit, which is not a valid synchronization model because permanently interrupting the execution of code by an arbitrary thread in a process at an unspecified time is always unsafe. This mismanagement of lifecycles inside a process, with the main thread often being oblivious to other threads operating concurrently in the process, leads to the abrupt termination of running code mid-operation thereby resulting in an inconsistent or potentially corrupted state when a process exits and after a process ceases to exist.

This issue been apparent in Windows NT, what we know today simply as Windows, ever since its first release as Windows NT 3.1 in 1993: the same Windows release that introduced preemptive multitasking and threads to the operating system. In this original Windows version, the `ExitProcess` function called a routine named `NtTerminateProcess` that would abruptly terminate all threads except for the thread that called `NtTerminateProcess`. In the case of destructing a module due to library unload instead of process exit, core system libraries such as the Advanced Windows 32 Base API DLL or `advapi32.dll`, which still exists in modern Windows today, [explicitly called the `TerminateThread` function in its module destructor](data/windows/legacy-software-analysis.md#operating-systems) to kill a background thread owned by the module before unloading.

Ever since the inception of Window NT, dependency cycles baked into its API and the introduction of an [anti-feature known as DLL thread routines](#dll-thread-routines-anti-feature) have broken the library subsystem lifetime for threads or the library-thread lifetime. These defects in combination with Microsoft still trying to utilize the library-thread lifetime are what led to the implementation of thread termination hacks in Windows.

Starting with Windows 2000, Microsoft inverted the traditional library-thread lifetime model to create the thread-library lifetime model whereby a thread can own a library.

However, there are still many cases throughout the Windows API where it relies on the old thread ownership model, which is problematic becuase that lifetime model is broken on the Windows platform. Since the original Windows NT release, modules have stopped calling `TerminateThread` from their module destructors because that is completely untenable given the dependency cycles that are only grew more pervasive throughout the Windows API; instead, system libraries rely on staying loaded throughout the entire lifetime of the process. In contemporary renditions of this hack, a Windows module will typically create a thread then throw away its one and only handle to it thereby leaving it to be consumed via the thread termination procedure Windows does at process exit.

We will now give a quick walkthrough of thread misuse in common modules, core to the functioning of a modern Windows system, which exemplify the invalid thread synchronization model we described:

**Examples instances to put in of CreateThread then immediately closing the thread object or never joining the thread back - not a valid design (WORK IN PROGRESS):**

SECHOST, IMM32 (came out in NT 4.0), WINHTTP, DIRECTX, ShellExecute maybe

**Random blurbs (WORK IN PROGRESS):**

The library subsystem lifetime

Additionally, an [anti-feature known as DLL thread routines](#dll-thread-routines-anti-feature) was introduced to block concurrent operations such as creating and joining threads in the module scope. These API semantics combined to break the

... library-thread vs thread-library

On Windows, the [library subsystem lifetime](#the-process-lifetime) for threads is broken by the contention of `DLL_THREAD_DETACH` and `DLL_PROCESS_DETACH` synchronizing and are also be impacted by circular dependencies since a subsystem's worker thread could shutdown before while another dependency circularly depends on that subsystem.

In any case, a process is the container for threads, so process termination will cause forceful thread termination if threads do not operate within the scope of process lifetime.

Continue to [*Process Meltdown*](#process-meltdown) to learn about how Windows attempts to cope with the fallout of having no established thread synchonization model when the process exits, including its affects after the process ceases to exist.

**WORK IN PROGRESS!**

For further research on Windows' misuse of threads, [see here](#further-research-on-windows-usage-of-threads).

## Thread Lifecycle Mismanagement Case Study with `ShellExecute`

Let's walk through a commonly used and well-known API call that exemplifies the thread lifecycle mismanagement issues pervading the Windows API, `ShellExecute`.

As an example, here are all the threads that still exist threads following a `ShellExecute` on the main thread (after `ShellExecute` returns and we get back the control flow):

```
.  0  Id: 1e88.398 ntterminateprocess_test_harness!test5
   1  Id: 1e88.18f0 ntdll!TppWorkerThread (ntdll!LdrpWorkCallback)
   2  Id: 1e88.1d78 ntdll!TppWorkerThread (ntdll!LdrpWorkCallback)
   3  Id: 1e88.cf8  ntdll!TppWorkerThread (ntdll!LdrpWorkCallback)
   4  Id: 1e88.1dec SHCORE!_WrapperThreadProc
   5  Id: 1e88.8e0  ntdll!TppWorkerThread (SHCORE!ExecuteWorkItemThreadProc)
   6  Id: 1e88.1b90 ntdll!TppWorkerThread (windows_storage!_CallWithTimeoutThreadProc)
   7  Id: 1e88.1498 combase!CRpcThreadCache::RpcWorkerThreadEntry
   8  Id: 1e88.1098 ntdll!TppWorkerThread (RPCRT4!LrpcIoComplete)
   9  Id: 1e88.1f90 ntdll!TppWorkerThread (shared thread pool worker)
  10  Id: 1e88.1158 SHCORE!<lambda_9844335fc14345151eefcc3593dd6895>::<lambda_invoker_cdecl>
```

Windows never joins these background threads back to the main thread or allows them to exit before process exit, as is best practice. But, as long as all these threads are guaranteed to stay waiting, then this configuration is workable; however, this is not the case. In particular, the `SHCORE!_WrapperThreadProc` thread is still actively working to shutdown an in-process COM server (here we see `CoUninitialize` is left running where it could still be processing outstanding messages and is currently cleaning up so it can shut down):

```
0:000> k
 # Child-SP          RetAddr               Call Site
00 00000041`7f8ff218 00007ffe`9a12e939     win32u!NtUserGetProp+0x14
01 00000041`7f8ff220 00007ffe`9a12e843     uxtheme!CThemeWnd::RemoveWindowProperties+0xa5
02 00000041`7f8ff250 00007ffe`9a133b3e     uxtheme!CThemeWnd::Detach+0x5f
03 00000041`7f8ff280 00007ffe`9e96ef98     uxtheme!ThemePostWndProc+0x4be
04 00000041`7f8ff360 00007ffe`9e96e8cc     USER32!UserCallWinProcCheckWow+0x548
05 00000041`7f8ff4f0 00007ffe`9e9870c8     USER32!DispatchClientMessage+0x9c
06 00000041`7f8ff550 00007ffe`9f191374     USER32!_fnNCDESTROY+0x38
07 00000041`7f8ff5b0 00007ffe`9d042384     ntdll!KiUserCallbackDispatcherContinue
08 00000041`7f8ff638 00007ffe`9d541c47     win32u!NtUserDestroyWindow+0x14
09 00000041`7f8ff640 00007ffe`9d541bf4     combase!UninitMainThreadWnd+0x47 [onecore\com\combase\objact\mainthrd.cxx @ 323]
0a 00000041`7f8ff670 00007ffe`9d492b6a     combase!OXIDEntry::CleanupRemoting+0x12c [onecore\com\combase\dcomrem\ipidtbl.cxx @ 1365]
0b 00000041`7f8ff6a0 00007ffe`9d492a7f     combase!CComApartment::CleanupRemoting+0xd2 [onecore\com\combase\dcomrem\aprtmnt.cxx @ 1078]
0c 00000041`7f8ff830 00007ffe`9d492ec1     combase!ChannelThreadUninitialize+0x37 [onecore\com\combase\dcomrem\channelb.cxx @ 993]
0d 00000041`7f8ff860 00007ffe`9d4a85bd     combase!ApartmentUninitialize+0x131 [onecore\com\combase\class\compobj.cxx @ 2680]
0e 00000041`7f8ff8e0 00007ffe`9d4a7a34     combase!wCoUninitialize+0x209 [onecore\com\combase\class\compobj.cxx @ 4037]
0f 00000041`7f8ff950 00007ffe`9d84c955     combase!CoUninitialize+0x104 [onecore\com\combase\class\compobj.cxx @ 3957]
10 00000041`7f8ffa40 00007ffe`9eb5be4e     ole32!OleUninitialize+0x45 [com\ole32\ole232\base\ole2.cpp @ 557]
11 00000041`7f8ffa70 00007ffe`9d357374     SHCORE!_WrapperThreadProc+0x21e
12 00000041`7f8ffb50 00007ffe`9f13cc91     KERNEL32!BaseThreadInitThunk+0x14
13 00000041`7f8ffb80 00000000`00000000     ntdll!RtlUserThreadStart+0x21
```

At the center of a crucial Windows component, the Shell, lies one case where Windows fails to control a thread within the lifecycle of the application, instead leaving the thread running to be consumed by `NtTerminateProcess` if it doesn't happen to exit in time. Additionally, if a background thread is waiting on stimuli from outside the process to start working then an external process giving work could cause currently waiting threads to start working at any time. The `SHCORE!<lambda_9844335fc14345151eefcc3593dd6895>::<lambda_invoker_cdecl>` thread meets this criterion because it is listening to a window object in a [Windows message loop](https://en.wikipedia.org/wiki/Message_loop_in_Microsoft_Windows) (confirmed by decompling the code). The `combase!CRpcThreadCache::RpcWorkerThreadEntry` thread is waiting on a timer, which means it can also run at an arbitrary time past our initial `ShellExecute`. On Windows, waiting can be "alertable" thus allowing the kernel to run custom code (in the form of APCs) in a given process as it waits. The `SHCORE!<lambda_9844335fc14345151eefcc3593dd6895>::<lambda_invoker_cdecl>` thread's wait with [`MsgWaitForMultipleObjectsEx`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-msgwaitformultipleobjectsex) passes the alertable flag, which is another means through which this wait is not guaranteed. Generally, a thread waiting on an intra-process synchronization mechanism is safe because an inter-process synchronization mechanism like a Win32 event object could be set by another process without regard to the process lifecycle if the application nevers joins the thread back. Information on thread running states can be gathered in Process Explorer (although this information doesn't include whether the waiting thread is alertable since that requires decompilation).

One practice I've noticed in the Windows API is for it to be [holding a lock even while waiting](code/windows/ntterminateprocess-test-harness/ntterminateprocess-test-harness.c), presumably for a message. This means that even hypothetically if a programmer were to generously wait for some time to increase the "odds" that threads belonging to the Windows API are not working when `NtTerminateProcess` kills threads, locks will still become orphaned thereby always leaving the process in an inconsistent state. After sleeping for an extended amount of time—over a minute—some background threads will wind themselves down to save resoures (a stack memory allocation on Windows consumes at least 64 KiBs of physical memory). If this winding down happens to occur when process exit is happening, then these threads will be killed mid-operation. A significant time after the original call into a Windows API function, worker threads can also often be found lingering for some time in the process without regard to process lifetime. For instance the `RPCRT4!PerformGarbageCollection` thread, which is likely a mechanism for cleaning up idle asynchronous connections among other resources for the RPC subsystem.

## Process Meltdown

Process exit on Windows is broken. This fact comes as a symptom of Microsoft failing to come up with a correct model for thread lifetimes on the Windows platform, leading the NT designers to create an abrupt thread termination hack that occurs every time a process exits. In this section, we discuss Microsoft's best efforts to salvage process exit as it pertains to running the module destructors of a library and process cleanup.

As we covered, the inner workings of the Windows API can leave running threads in the process at the time of process exit. Process exit includes running modules destructors to perform cleanup work, so if threads are still running while the process is being destroyed then an unpredicatble crash by memory corruption would likely be the result.

Instead of addressing the root issue, Microsoft chose to mitigate the problem by forcefully terminating threads before winding down the process. However, abruptly terminating threads is impossible to do safely, and [Microsoft says as much in their own documentation](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-terminatethread#remarks). Terminating threads is unsafe because those threads could be modifying a shared resource, holding onto a lock, or doing some other important task at the time of termination, thereby leaving those resources in a corrupt state, orphaning locked synchronization mechanisms, or interrupting critical work. Thus, solving the first problem only transformed the issue and created a new problem.

Process exit working in this way is no way excusable and [cannot remotely be described as "design"](https://devblogs.microsoft.com/oldnewthing/20070503-00/?p=27003#:~:text=design), as one Microsoft engineer put it:

> Using the word design to describe this is like using the term swimming pool to refer to a puddle in your garden.

Even when Windows NT was in early development, it was of course [well-known by its designers that terminating threads for any reason was an egregiously wrong thing to do](data/windows/timeline-verification/timeline-checkups.md#terminatethread-introduction). But, that did not deter them.

What folllows describes how Microsoft implements their hack for process exit:

First, the `RtlExitUserProcess` function in Windows acquires a few locks. These locks, in order of acquisition, are the load/loader locks (i.e. `ntdll!LdrpLoadCompleteEvent` and `ntdll!LdrpLoaderLock`), the Process Environment Block (PEB) lock (`ntdll!FastPebLock`), and the process heap lock (calls `ntdll!RtlLockHeap`). These locks locks are acquired before thread termination as a quick fix to at least ensure consistency of these core components and data structures when module destructors run. The PEB and heap locks are both unlocked immediately following thread termination. The load/loader locks remain locked because locking them is necessary anyway to maintain consistency while the process runs its module destructors. These locks are all part of or managed by NTDLL, the Windows sysem DLL, and it sensibly places the loader's synchronization mechanisms at the top of its own lock hierarchy.

Now thread termination happens by the `RtlExitUserProcess` function making the `NtTerminateProcess` system call passing in a process handle of zero or no handle meaning operate on the current process. [`NtTerminateProcess` iterates through each thread in the process, abruptly terminating each one](https://github.com/reactos/reactos/blob/1b25fe161caff2806f47b31f4d467d16861ba648/ntoskrnl/ps/kill.c#L1214-L1235), [except for the thread requesting process termination](https://github.com/reactos/reactos/blob/1b25fe161caff2806f47b31f4d467d16861ba648/ntoskrnl/ps/kill.c#L1243-L1251). Thus, the process is forcibly reduced to a single thread.

Post `NtTerminateProcess`, trying to wait on one of these orphaned locks will cause the process to hang open, never exiting. Microsoft would like to avoid this, so their second mitigation is for Windows API locks to check if the process is shutting down before waiting on them, and if so, triggering a forceful termination of the processs without calling the remaining module destructors. Here is how Windows implements this mitigation for a few synchronization mechanisms:

The `LdrShutdownProcess` function, called by `RtlExitUserProcess`, sets `PEB_LDR_DATA.ShutdownInProgress` to true. Since `RtlExitUserProcess` acquires load/loader lock before `PEB_LDR_DATA.ShutdownInProgress` is set to `true`, code run past this point also runs under load/loader lock. `PEB_LDR_DATA.ShutdownInProgress` covers all module destructors at process exit, including FLS callbacks, TLS destructors, and the `DLL_PROCESS_DETACH` routine of `DllMain` functions (which includes the destructors of C++ objects that exist at the module scope, DLL `atexit` routines, and others).

For a critical section, upon calling `EnterCriticalSection` on a contended lock, the `ntdll!RtlpWaitOnCriticalSection` function checks if `PEB_LDR_DATA.ShutdownInProgress` is true. If so, the function jumps to calling `NtTerminateProcess` passing in a process handle of `-1` thereby forcefully terminating the process.

For a slim reader/writer (SRW) lock, upon calling `AcquireSRWLockExclusive` or `AcquireSRWLockShared` on a contended lock, the `ntdll!RtlpWaitCouldDeadlock` function checks if `PEB_LDR_DATA.ShutdownInProgress` is true. If so, that function returns true and `ntdll!RtlAcquireSRWLockExclusive` calls `NtTerminateProcess` passing in a process handle of `-1` to immediately kill the current process.

There are also other little things Windows will do following the initial `NtTerminateProcess` like [blocking thread creation with `CreateThread`](code/windows/ntterminateprocess-test-harness/ntterminateprocess-test-harness.c) at the kernel-level or failing the thread pool API functions by checking `PEB_LDR_DATA.ShutdownInProgress` in user-mode (since these functions are otherwise unaware that the threads in its pool have been killed).

With all mitigations applied, the fallout for library destructors and process cleanup in a Windows process are as follows:

### In-Process Inconsistencies

The consistency of all data structures, aside from the ones we specifically mentioned, in use by the Windows API or your program becomes a gamble as to whether they are left in a corrupt state after running `NtTerminateProcess`. Common data structures that could be left in a corrupt state include [private heaps](https://learn.microsoft.com/en-us/windows/win32/api/heapapi/nf-heapapi-heapcreate) or heaps using a custom allocator, CRT state (there are many locks here), internal `KERNEL32`/`KERNELBASE`/`NTDLL` state (e.g. various global list locks like the heaps list locks at `ntdll!RtlpProcessHeapsListLock` or the thread pools list lock at `ntdll!TppPoolpListLock`, [WIL](https://github.com/microsoft/wil) locks part of `KERNELBASE`), and generally tons of other obscure locks. Even FLS state could be corrupted due to the `ntdll!RtlpFlsDataCleanup` function at process exit (after the initial `NtTerminateProcess`) acquiring the [global FLS lock](code/windows/fls-experiment/fls-experiment.c) and per-FLS locks thereby forfeiting the process before any `DLL_PROCESS_DETACH` destructors get the chance to run. Even calling a function as simple as `printf` or `puts` from a module destructor is unsafe because the CRT stdio critical section lock could be orphaned.

### Process Hanging Open

Even with all mitigations applied, it's possible for a process to hang open due to deadlocking on an orhpaned synchronization mechanism in a module destructor. Specifically, [hanging open is possible with a Win32 event object](code/windows/dll-process-detach-test-harness/dll-process-detach-test-harness.c), which is a synchronization mechanism that the Windows API commonly utilizes throughout its operation. An event object has two unique properties that causes the anti-deadlock logic Microsoft employs for other synchronization mechanisms like critical sections and mutex objects to break down: No owning thread and inter-process synchronization support. An event object works in cases where the thread that reset the event has exited, by design. Combine this with event objects supporting inter-process synchronization (so, the entire process that reset the event can legally no longer exist) and [handle inheritance](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createeventa) causing events to easily be shared between processes makes implementing an anti-deadlock mitigation for event objects post-`NtTerminateProcess` infeasible (not that Microsoft likely wants to since [user-mode inherited event objects from the kernel](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/event-objects) and are supposed to work the same way). Microsoft makes no mention of this danger in their official documentation.

Likewise, other inter-process [synchronization objects](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject#remarks) without an owning thread are vulnerable to this deadlock scenario. Custom synchronization mechanisms without an owning thread, likely designed using the Windows [futex-like API](https://devblogs.microsoft.com/oldnewthing/20170601-00/?p=96265), are also at risk. For instance, asynchronous C++ semantics on MSVC internally use a <code>Sleep[ConditionVariable](https://en.wikipedia.org/wiki/Monitor_(synchronization)#Condition_variables)SRW</code> (which is implemented with the futex-like `ntdll!NtWaitForAlertByThreadId` API) when getting the result of a future object (`std::future`) returned by an asynchronous routine (`std::async`), thus hanging the process indefinitely (I have tested this).

A fate worse than deadlock, spinlocks are yet another another victim of `NtTerminateProcess`. A spinlock is useful for protecting hot data structures (often a flag, as Windows does in a few places) that observe short access times with the lowest overhead possible. An orphaned spinlock is a huge problem because attempting to acquire it will infinitely busy loop the CPU thereby degrading completely system performance on that CPU core and wasting power. Knowing about `NtTerminateProcess` does at least mean a programmer can mitigate deadlock concerns in their spinlock by checking `PEB_LDR_DATA.ShutdownInProgress` before spinning (although, frequently perfoming this check on a hot code path like a synchronization mechanism could impact performance, especially since the branch has an immediate data dependency).

Generally, there also exists other edge cases involving Windows APIs with the ability to wait (e.g. a blocking read waiting forever for I/O that will never happen after `NtTerminateProcess` unexpectedly killing relevent threads or a [corner cases with file locks](code/windows/dll-process-detach-test-harness/dll-process-detach-test-harness.c)), which can hang the process open.

### Crash

One of the [side effects of thread termination](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-terminatethread#remarks) is that it causes the underlying thread object in the kernel to become signaled:

> The state of the thread object becomes signaled, releasing any other threads that had been waiting for the thread to terminate. The thread's termination status changes from `STILL_ACTIVE` to the value of the `dwExitCode` parameter.

This side effect can result in incorrect behavior or crashes if the library destructor uses the synchronization provided by the thread object as a signal to operate on some state that the thread owned without further protection or a variable that it was only going to set before exiting. A common occurrence of this risk being realized is when getting the [return value of a thread](code/windows/dll-worker-thread-return-value/lib.cpp), here in cross-platform C++. The outcomes of this side effect could have security implications.

Further, subsystems that employ the efficient [thread-local storage strategy for thread synchronization](https://en.wikipedia.org/wiki/Thread-local_storage#Usage) typically work by waiting for threads to exit then checking an accumulator to get the final result. Thread termination breaks this synchronization approach by causing threads to die before they finished their part of the work thus leaving the accumulator in an incorrect or impartial state, which could result in memory corruption or incorrect behavior.

Generally, thread termination by `NtTerminateProcess` kills threads that some subsystems could still hold a reference to or believes to exist. Windows tries to compensate for this scenario in some cases by raising an exception when interacting with these subsystems. In particular, a predicatable crash like this can occur upon trying to use thread pool internals, which raises a `ntdll!TppRaiseInvalidParameter` exception because the relevant functions inspect `PEB_LDR_DATA.ShutdownInProgress` before proceeding with typical operation. Beyond synchronization mechanisms, there are lots of places where Windows checks `PEB_LDR_DATA.ShutdownInProgress` (setting a read watchpoint here gleans a lot of information), prior to proceeding with typical operation, which is presumably to prevent undefined behavior that could result in a crash or incorrect behavior.

A memory access violation crash can occur if one thread tries to access the stack memory allocation of another thread (I've confirmed this memory mapping is removed as soon as a thread exits in WinDbg and the [behavior for thread termination is documented to be the same](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-terminatethread#remarks)) that it assumes still exists. While not a typical action, walking *between* stacks (sometimes performed along with stack walking) is still commonly done by [garbage collectors](https://github.com/dotnet/coreclr/blob/master/Documentation/botr/stackwalking.md), anti-malware software, and in other specialized cases.

Beyond these scenarios, assuming applications respond correctly to Windows API failure statuses by failing closed, a crash should not occur. However, software vendors are not always known to robustly check for failure with each Windows API call. As a result, in practice, crashes can still occur in cases like ignoring the [`WAIT_ABANDONED` mutex error status](https://devblogs.microsoft.com/oldnewthing/20050912-14/?p=34253) (only mutex objects can return this error status) or ignoring [thread creation failure](code/windows/ntterminateprocess-test-harness/ntterminateprocess-test-harness.c).

### Out-of-Process Inconsistencies

Here is a short list containing some of the out-of-process affects Process Meltdown could have on module cleanup and graceful shutdown routines:

- Thread termination could interrupt the process' communication with another process or endpoint leading to an inconsistent data stream when destructors run
  - If a destructor reuses that connection to communicate again (e.g. to gracefully send a connection end message) then undefined behavior outside the process could result. Buffered I/O provides a great example: When communicating in a [TLV protocol](https://en.wikipedia.org/wiki/Type%E2%80%93length%E2%80%93value), for instance, if transmission is interrupted midway sending a packet then the server will never receive the full packet length of data causing it wait until timeout or forever, which could leak the connection. A destructor trying to send new data over that socket would break the application-layer message boundary thus forfeiting or corrupting the connection.
- Destructors that were supposed to clean up external resources such as stored data including temporary files or registry entries may never run due to the process forcefully exiting early (e.g. in the orphaned lock case)
will be permanently and persistently leaked to the system.
  - On Windows, [`atexit` routines are included in a library's module deinitialization code when registered from a DLL](code/windows/atexit/README.md) and the typical use case for an `atexit` routine to clean up a resource outside the process like a file. Strictly speaking, calls to `atexit` should not should not be made by a library since its lifetime is not necessarily tied to the process lifetime; however, the pervasive "DLL host" Windows architecture, where everything is a library and programs only exist to load libraries, significantly increases this risk and the chance that all Process Meltdown risks are realized across the board
- System resources like kernel objects can leak if inconistent process state following `NtTerminateProcess` leads to another process not closing handles because, for instance, the other process was waiting for some communication that it never got and it may never receive
- If an event object was in use between multiple processes and the thread in a process that last put it into a waiting state gets terminated before setting it again, then indefinite hangs or resource leakage can occur in other processes
- Two processes are communicating via shared memory and one process has its communication end abruptly due to arbitrary thread termination, the other process might never become aware that the process it was communicating with has been killed thus causing the other process to leak its handle to the memory mapping
  - The Windows API makes heavy use of shared memory using the `NtMapViewOfFile` API and [some Windows libraries also use shared memory in the scope of module lifetime](data/windows/LdrShutdownProcess-trace.log)

**Note:** Additions are pending further research.

### Kernel Object Cleanup Performance Degradation

When a process ends, the kernel will close any handles to kernel objects that the process is still holding onto. This action is a requirement for the memory integrity of a system because if the system did not reclaim resources then a bad application could cause permanently starve the system. For some system resources, the kernel checks if the resource it is trying to close was left in an impartial state. If the kernel finds the state of a system resource to be inconsistent, then it must do extra work to account for that fact. This work can be costly because it often includes I/O, acquiring big locks the aquisition of which would not usually be necessary, and a general increase in bookkeeping.

This issue, for instance, can be seen with [files](https://github.com/reactos/reactos/blob/2186ce3d580bf664491060ded28e697d1f8c17e8/ntoskrnl/io/iomgr/file.c#L2286-L2340) including all of the following I/O devices: "file, file stream, directory, physical disk, volume, console buffer, tape drive, communications resource, mailslot, and pipe" objects, especially when opening in the [default exclusive mode](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea?redirectedfrom=MSDN#remarks:~:text=Prevents%20other%20processes%20from%20opening%20a%20file%20or%20device%20if%20they%20request%20delete%2C%20read%2C%20or%20write%20access.). [Inter-process communication](https://github.com/reactos/reactos/blob/2186ce3d580bf664491060ded28e697d1f8c17e8/ntoskrnl/lpc/close.c#L148-L199) I/O is another case that shows how expensive leaving threads around can be because another process could be waiting on communications from our process that it won't get until the kernel gets around to reading its message then closing the connection. [File locks](https://github.com/reactos/reactos/blob/2186ce3d580bf664491060ded28e697d1f8c17e8/ntoskrnl/io/iomgr/file.c#L2198-L2266) are yet another great example. In the [`LockFile`/`LockFileEx` documentation](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-lockfile#remarks) it specifically calls out that "the time it takes for the operating system to unlock these locks depends upon available system resources". To avoid leaving files locked for an extended period of time, the documentation therefore recommends unlocking files before process exit; however, this may not be doable if the Windows API is not correctly managing its thread lifetimes within the scope of the process and [external Windows vendors](data/windows/load-many-modules-ldr-ddag-node-state-trace.log) have adopted the same poor practice as a result. An exclusive access or other access type is a property tied to the kernel object itself and the kernel [calls object-specific "Okay To Close" or `CloseProcedure` routines](https://github.com/reactos/reactos/blob/2186ce3d580bf664491060ded28e697d1f8c17e8/ntoskrnl/ob/obhandle.c#L716-L719) before cleaning up an object. Additionally, if an orphaned intra-process synchronization mechanism causes forceful process termination before libraries get the chance to run module destructors, then that will generally create more resources for the [kernel to clean up sequentially](https://github.com/reactos/reactos/blob/d72864de9558928ecaefc9acf4ea925c385e6836/ntoskrnl/ob/obhandle.c#L2054-L2057). If another process needs access to an exclusive resource while a thread in another process that was about to relinquish that resource but did not because process exit killed that thread then [priority inversion](https://en.wikipedia.org/wiki/Priority_inversion) occurs until the kernel gets around to reclaiming the exclusive resource. The thread was about to relinquish an exclusive resource before being killed may have also been given a higher base or boosted priority by the Windows scheduler, which would directly affect priority because the thread doing process exit would not share the priority of the thread that held access to the resource. The loss or inversion of priority when managing system resources can contribute to overall system resource thrashing and reduce responsiveness, especially when the system is under memory pressure.

Lastly, it is notable that the kernel [blocks kernel APCs from running on the current thread for the duration of process handle table cleanup](https://github.com/reactos/reactos/blob/d72864de9558928ecaefc9acf4ea925c385e6836/ntoskrnl/ob/obhandle.c#L2047-L2060). These APCs, a form of cooperative multitasking, could be important operations that the process started before termination like I/O completion by `NtWriteFile` or other system routines. [ReactOS warns](https://github.com/reactos/reactos/blob/d72864de9558928ecaefc9acf4ea925c385e6836/ntoskrnl/ke/apc.c#L564-L565): "Any caller of this routine should call `KeLeaveCriticalRegion` as quickly as possible". So, decreasing the cost of performing kernel object cleanup when process exit happens would be more than ideal in this scenario.

**Note:** Parts of this analysis covers the ReactOS implementation because Windows is closed source and reverse engineering kernel code is hard. Modern Windows could have changed some technical details but the overall message stays the same.

### Conclusion

In the end, Windows is stuck between implementing process exit deadlock and resource cleanup heuristics at the cost of performance on code hot paths, all the while it being impossible for the operating system to achieve correctness as long as it is killing threads. Over a long enough period, resource leaks in memory and on disk can accumulate until a system restart, reinstallation, or manual cleanup is the only solution. The complete lack of design here is hostile towards cross-platform software wishing to reasonably rely on destructors for their intended purpose without writing code natively for Windows, low-level programming languages that provide access to module destructors through its features, and correct operating system designs that coexist with Windows. In its current state, the phenomenon that occurs every time an application finishes running on Windows would most accurately be described as Process Meltdown.

## Further Research on Windows' Usage of Threads

### Securable Threads

In Windows, threads are a securable resource independent of the host process:

> A thread can assume a different security context than that of its process. This mechanism is called impersonation. When a thread is impersonating, security validation mechanisms use the thread’s security context instead of that of the thread’s process. When a thread isn’t impersonating, security validation falls back on using the security context of the thread’s owning process.
>
> *Windows Internals: System architecture, processes, threads, memory management, and more, Part 1 (7th edition)*

Where Windows often uses thread impersonation to execute code as another user, the equivalent functionality on a Unix-like system can be accomplished by creating a new minimal process and using `setuid` along with the `CAP_SETUID` privilege (this is the Linux privilege, it can vary on other Unix-like OSs) to permanently change the process' user ID (UID). An [OpenSSH server](https://isopenbsdsecu.re/mitigations/fork_exec/), for instance, works in this fashion with the project also striving towards splitting components to create [increasingly minimal processes](https://undeadly.org/cgi?action=article;sid=20241014070554). Unix is an operating system that values multiprocessing over multithreading (this is the only correct operating system design because processes contain threads). As a result, process creation is fast, making it practical to isolate each identity to its own process.

The [Principle of Least Privilege](https://en.wikipedia.org/wiki/Principle_of_least_privilege) states that user or entity should only have access to the specific data, resources, and privileges necessary to complete a required task. Securable threads violate the Principle of Least Privilege because threads with different identities have access to each other by residing in the same address space within a process. As per *Windows Internals*, this shared access includes handles to kernel objects:

> It’s important to keep in mind that all the threads in a process share the same handle table, so when a thread opens an object—even if it’s impersonating—all the threads of the process have access to the object.

Essentially, impersonation is the opposite of a proactive security design. Instead of limiting attack surface, securable threads often maximize it as much as possible.

Thread impersonation is also [highly complex](https://devblogs.microsoft.com/oldnewthing/20110928-00/?p=9533) and does not compose. For thread impersonation to work, every layer of a subsystem within the Windows API has to specially support its usage (e.g. [COM cloaking](https://learn.microsoft.com/en-us/windows/win32/com/cloaking)). And if there is even a single occurrence of a Windows API function being called that does not support impersonation or that you forget to pass the impersonation token to, then that creates a vulnerability. Failing to correctly use and control for the consequences of thread impersonation has long been a [source of security bugs in Windows](https://y3a.github.io/2023/08/24/cve-2023-35359/) (with Microsoft now implementing hacks in the kernel to workaround this delicate security model).

For all these reasons, I find that the Windows securable thread model is an insecure and fragile security model. Securable processes or per-process security is inherently more robust and secure, and this is the model that Unix-like systems are built on.

### Multithreading is Insecure

Concurrently operating threads introduce a significant source of non-determinism in computers. Multithreading allows the execution of separate threads to overlap at unspecified times while accessing shared/global state or data structures. These interactions are inherently complex.

Security is first and foremost about minimizing attack surface or the things that can go wrong.

Multithreading works counter to security by introducing an entire an entire new class of bugs—concurrency bugs, for attackers to find and exploit. The Microsoft Security Response Center (MSRC) is known to treat killing bug classes as a top priority  in their proactive approach to security.

Nowhere has this huge attack surface come to light more than in the 2022 paper [*COMRace: Detecting Data Race Vulnerabilities in COM Objects*](https://www.usenix.org/system/files/sec22-gu-fangming.pdf), that revealed **26 privilege escalation vulnerabilities** (with most of these also being sandbox escapes that are commonly used as part of web browser exploits). Windows uses COM everywhere and managing concurrent access, particularly for MTA apartments since requests to an STA server are serialized, is error-prone even for experienced developers. COM components that come with Windows are mostly free-threaded (supporting STA or MTA), with most uses being MTA to ensure high performance (this information is visible by looking at registered COM components in the registry and tracing Windows APIs). These vulnerabilities are only the tip of the iceberg, with [concurrency bugs](#thread-creation-is-not-thread-safe-on-windows-a-concurrency-bug-in-the-windows-loader) in complex, multithreaded software being an immense landscape for correctness and security issues to hide.

The non-deterministic nature of concurrency bugs makes them difficult to catch in code review or fuzzing. As a result, multithreading, especially when combined with any sizable amount of shared state, should be avoided in security-sensitive contexts.

In contrast to Windows, Unix systems tends to avoid this entire class of bugs through by emphasizing minimal processes that work with other minimal processes in a multiprocessing architecture, as opposed to multithreading.

### Expensive Threads

Anyone familiar with operating systems and their differences is aware that process creation tends to be slow on Windows. However, this fact is [commonly attributed to an architectural preference of Windows](https://stackoverflow.com/a/48244) favoring multithreading over multiprocessing (i.e. one process housing multiple threads over separate single-threaded processes). It follows then, that Windows would be better optimized for creating threads and multithreaded workloads than Unix-like operating systems.

Let's get some numbers on Windows vs. Linux thread creation and join times for 10,000 threads (benchmark source code for [Linux](code/glibc/thread-performance) and [Windows](code/windows/thread-performance)):

| System   | Native Create Thread (seconds) | Native Create Thread with 50 FLS Allocations (seconds) | Native Create Thread without Loader Initialization (seconds) |
| -------- | ------------------------------ | ------------------------------------------------------ | ------------------------------------------------------------ |
| Linux    | 0.45                           | N/A                                                    | N/A                                                          |
| Windows  | 1.43                           | 1.54                                                   | 1.33                                                         |

**Benchmark systems details:** Both Xen HVMs, Intel i5 4590, 4 vCPUs each, 8 GiBs of memory each, up-to-date Windows 10 22H2 and Fedora 39 on Linux 6.1. Tests performed while the host system and other virtual machines were suspended or turned off.

Linux native thread creation and join times comes out firmly ahead, averaging speeds 3.2x faster than Windows. However, outside of some server applications that may correspond each client connection to a new thread, quickly creating 10,000 threads is not a realistic workload. Upon booting Windows, Process Explorer shows that there are about 1,000 threads between all processes on the system. So, Windows thread creation time is unlikely to become a performance bottleneck in practice especially because Windows typically keeps threads alive and waiting as worker threads for some time instead of immediately deleting them in case new work comes along. Windows threads run their `DLL_THREAD_ATTACH` and `DLL_THREAD_DETACH` routines at thread startup and exit, which requires the same synchronization as `LoadLibrary` (including the `DLL_PROCESS_ATTACH` routine) and `FreeLibrary` (including the `DLL_PROCESS_DETACH` routine) operations. Therefore, significant variance or unexpected stutters could be present in thread startup and exit times if overlapping thread creation/exit or library load/free operations occur. Interestingly, by looking at these numbers we can see that Windows thread creation overhead primarily comes from the time it takes for the NT kernel to spawn the thread itself and not from any action in user-mode (in the future, we may run tests to see how performance changes with two threads simultaneously creating threads since the synchronization requirement of thread loader initialization could have a greater effect then). Another minor note is that not joining threads on Linux yields around a 25% performance improvement, although this didn't seem to have a noticeable effect on Windows (joining means running the thread until its end so any performance impact here would be due to the scheduler).

Next, we will review the difference in resource consumption between Windows and Unix threads. Each thread requires its own stack memory allocation. On Windows, the default reservation size of this memory mapping is [1 MiB](https://learn.microsoft.com/en-us/windows/win32/procthread/thread-stack-size). However, only [64 KiBs](https://devblogs.microsoft.com/oldnewthing/20031008-00/?p=42223) of that reservation is consumed from phyiscal memory. On Linux, these sizes are [8 MiB](https://unix.stackexchange.com/a/473445) and the [archiecture's page size](https://stackoverflow.com/a/24819521) (typically 4 KiBs on modern x86-based and ARM systems), respectively. Since Linux and other Unix-like systems follow the system's page size (the smallest possible memory mapping size as set by the MMU) when creating memory mappings, each thread's stack memory mapping consumes significantly less memory on Unix systems than on Windows, an attribute which is certainly desirable for a general-purpose computer. Specifically, with a page size of 4 KiB means Linux threads are 16x more lightweight in memory than Windows. These facts only account for user-mode threads because kernel-mode threads do not exist in virtual memory. Kernel-mode threads are fully committed into physical memory with a fixed size stack (typically [8 KiBs on Linux](https://docs.kernel.org/next/x86/kernel-stacks.html) or [16 KiBs on Windows](https://bsodtutorials.blogspot.com/2013/11/kernel-stacks-user-stacks-dpc-stacks.html)). Generally, less memory mapping granularity also makes guard pages less effective at catching memory overrun bugs.

Checking in Process Explorer, a freshly booted Windows system has around 1,000 threads between all processes. 1,000 × 64 = 64,000 KiB or 62.5 MiB of memory spent just on thread stacks. In contrast, `htop` (since `ps` and `top` default to including kernel threads) shows a typical freshly booted Linux system has around 100 threads (although this number increases to ~150 when starting an XFCE desktop). Let's compare apples to apples since Windows has its desktop open. 150 × 4 = 600 KiBs in thread stacks. Let's assume that thread stacks stay within 4 KiBs because stacks mostly consist of pointers or small constants and thus rarely grow to be very large. By our measurement, the end result is that threads on a typical Linux desktop system account for **over 100x fewer memory resources** than on Windows (107x to be exact).

With more threads, in particular running threads, comes greater context switching overhead. The cost for a kernel to swtich between threads is expensive and not a negligible factor in performance. This price includes saving and restoring CPU state, CPU cache (L1, L2, and L3) eviction leading to cache misses having the largest potential performance impact, TLB (translation lookaside buffer) flushes thus invalidating virtual-to-physical address translation caches, and general kernel bookkeeping.

Checking in Performance Monitor, a freshly booted Windows system does around 700 context switches per second while idling. In contrast, checking `vmstat` shows that an idling desktop Linux system sees around 150 context switches per second. Linux, probably due to less overall background work going on, has around 5 times less context switches while idling. These differences are significant and could lead to notable baseline performance and battery life differences (although, Windows may takes steps to reduce background work when running on a battery for devices like laptops).

A kernel's scheduler plays a large role in the performance of a multithreading program or threads within a system. A scheduler decides which thread the kernel should switch to when a context switch occurs. As of version 6.6 (2023), the Linux kernel uses a new scheduler called [earliest eligible virtual deadline first (EEVDF)](https://en.wikipedia.org/wiki/Earliest_eligible_virtual_deadline_first_scheduling) that employs an algorithm by the same name. This scheduler takes into account multiple paramters including virtual time, eligible time, virtual requests and virtual deadlines for determining scheduling priority. This scheduler replaces the Completely Fair Scheduler (CFS), likely because overly striving for equal run-time distribution over thread readiness factors can cause [lock convoys](https://learn.microsoft.com/en-us/archive/msdn-magazine/2008/october/concurrency-hazards-solving-problems-in-your-multithreaded-code#lock-convoys) in practice. According to *Windows Internals: System architecture, processes, threads, memory management, and more, Part 1 (7th edition)*, "Windows implements a priority-driven, preemptive scheduling system". Indeed, the Windows scheduler is dynamic, relying heavily on ["priority boosts"](https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling) to optimize for the foreground window, user interface interactiveness, multimedia applications, and lock ownership for locks that fully rely on the kernel (e.g. an event object allows execution to proceed). The *Windows Internals* book talks in-depth about the scheduler in its "Thread scheduling" subchapter. The scheduler on Windows and Linux is best optimized for the workloads common to each system (similar to a heap memory allocator, there are no settings that will be best optimized for every possible workload).

Departing from synthetic benchmarks, [Linux is also better equipped to take advantage of modern CPUs with high core counts in real-world applications](https://www.phoronix.com/review/3990x-windows-linux/6), with increasing margins for higher numbers of cores.

Based on our findings, we can conclude that Windows threads, like processes, are significantly more expensive and heavyweight than their typical Unix counterparts.

## DLL Thread Routines Anti-Feature

The Windows [loader](#defining-loader-and-linker-terminology) is tightly coupled with the [threading implementation](https://en.wikipedia.org/wiki/Thread_(computing)#1:1_(kernel-level_threading)) to provide a feature known as [DLL thread routines or notifications](#constructors-and-destructors-overview). The notifications run a callback in each DLL at thread startup and exit times. By default, all Windows DLLs are registered for this callback and can define custom actions by handling the `DLL_THREAD_ATTACH` or `DLL_THREAD_DETACH` call reasons in `DllMain`.

These notifications have existed in Windows NT, what we know today simply as Windows, [ever since its first release as Windows NT 3.1 in 1993](http://web.archive.org/web/20240308195249/http://bytepointer.com/resources/pietrek_peering_inside_pe.htm#:~:text=DllCharacteristics)—the same time threading was introduced to the operating system.

DLL thread routines are an anti-feature that should have never existed because their synchronization [breaks](code/windows/library-init-thread-join) the [library subsystem lifetime](#the-process-lifetime) for threads.

DLL thread notifiactions themselves are [are effectively useless](https://stackoverflow.com/a/30637968), and well-written libraries often disable them to improve performance by calling `DisableThreadLibraryCalls` (a dynamic operation that must be called in `DLL_PROCESS_ATTACH`). Dynamically allocated [thread-local data](#flimsy-thread-local-data) is already a fragile mechanism for managing state and integrating it with the loader causes breakage.

There is no good trade-off that justifies the existence of these notifications and they come with far more disadvantages than advantages. Additionally, other operating systems work just fine without requiring loaded libraries receive thread creation and exit notifications. I suspect this false feature for thread-local data integration was added in as an afterthought to intentionally cover up some implications the faulty architecture of Windows has on concurrency. Specifically, module finalization must occur in the opposite order of module initialization and if there those modules have dependency cycles between them then the thread belonging to a module could still be concurrently running and using a module that has now been deinitialized—likely resulting in a crash—before the module owning that thread is destructed at which time it would have been able to join back the thread. The DLL thread routines block joining back the thread thereby universally breaking the library subsystem lifetime for threads for acyclic and cyclic
dependencies alike.

Also notable in this context is that that calling `TerminateThread` from the module destructor of a module that has a dependency cycle was especially untenable even as a hack because the other library in the cycle could deinitialize before the library that owns a thread thereby leaving that thread to operate concurrently on a library dependency that has undergone finalization. This is the same reason joining a thread from

### Synchronization Requirements

DLL thread routines are for initializing per-thread data, so one might question the need for process-wide synchronization here. However, these routines run as part of the [loader's state machine](#concurrency-and-parallelism-in-the-loader) and a module is the fundamental building block that is accessible from the global scope, so synchronization is necessary for a few reasons:

1. Protect from concurrent library unload
    - Thread startup and exit acquiring the same locks needed for library load and free fully protects all DLLs from being unloaded while `DLL_THREAD_ATTACH` or `DLL_THREAD_DETACH` routines run
    - The loader could also ensure a module is not concurrently unloaded by incrementing the reference count on each module, running its DLL thread routine, then decrementing that module's reference count, and then repeat for all modules. But, that could be taxing on performance, and if a reference count concurrently drops to zero then actual library unload (acquiring the typical locks) must occur, anyway.
2. Module list protection
    - The loader thread initialization function walks a module list to initialize each module, this list is a global data structure that requires protection to walk between nodes ([Windows fails to protect this access](#thread-creation-is-not-thread-safe-on-windows-a-concurrency-bug-in-the-windows-loader))
      - There could be a lock specific to just protecting a modlue list's access; however, Windows groups the protection of module lists in with broader locks
    - If a consistent snapshot of the loaded modules at a given point in time is desired then the lock must remain held while all the callbacks are run, a trait which is desirable because the loader does not want to run the `DLL_THREAD_ATTACH` of a module while it is still in its [`LdrModulesInitializing`](#windows-loader-module-state-transitions-overview) state due to a concurrent library load but it likely still wants to run the `DLL_THREAD_ATTACH` of a `LdrModulesInitializing` library that is responsible for starting the new thread
3. Full load owner protection requirement
    - Technically, only acquiring `ntdll!LdrpLoaderLock` is necessary to protect from concurrent library unload because unloading libraries must first deinitialize by calling `DLL_PROCESS_DETACH` routines, which requires `ntdll!LdrpLoaderLock` protection, before any futher unloading steps can occur
    - Still, full loader owner protection by the `ntdll!LdrpLoadCompleteEvent` and `ntdll!LdrpLoaderLock` synchronization mechanisms is necessary to prevent lock hierarchy violation in the case that a `DLL_THREAD_ATTACH` or `DLL_THREAD_DETACH` routine loads a library perhaps accidentally due to Windows delay loading
    - Taking this extra lock makes no difference to DLL thread routines breaking the library subsystem lifetime but some extra performance in concurrent library load and thread startup scenarios could be eeked out if not for requiring full load owner protection

## Flimsy Thread-Local Data

Thread-local data is a fragile mechanism that introduces unnecessary failure points, is often a symptom of poor design, and is unfit for use in subsystems, particularly at the operating-system-level, for a variety of reasons:

1. Thread-affinity issues
    - A subsystem that uses thread-local data ties itself to the lifetime of the thread it set a thread-local value on, once that thread exits the thread-local data is invalid to use on any thread
    - Subsystems should avoid imposing strict threading requirements on other subsystems or the application
    - Keeping track of the caller is always best performed by explictly passing a context structure around, as is commonly done by C libraries like [SQLite with its `sqlite3` structure](https://www.sqlite.org/c3ref/sqlite3.html), rather than assuming the caller's identity is tied to a thread like [how COM keeps track of state using its `TEB.ReservedForOle` per-thread data structure](https://github.com/reactos/reactos/blob/947c03e9483ba5e20cb083de519c463adf37643b/dll/win32/ole32/compobj_private.h#L266-L282) (the `CoInitialize` and `CoInitializeEx` functions initialize this per-thread state)
    - Thread-affinity in combination with dynamic loading of a library on an unspecified thread makes using thread-local data a [source of module initialization routine issues](https://devblogs.microsoft.com/oldnewthing/20040127-00/?p=40873) (thread-local data is, [of course](https://learn.microsoft.com/en-us/windows/win32/dlls/using-thread-local-storage-in-a-dynamic-link-library), [safe to create](https://pubs.opengroup.org/onlinepubs/9799919799/functions/pthread_key_create.html#:~:text=The%20pthread_key_create()%20call%20could%20either%20be%20explicitly%20made%20in%20a%20module%20initialization%20routine) in a module initializaion routine, but setting it is unsafe)
2. Dynamically allocated thread-local storage can quickly run out of indexes
    - TLS has [1088 maximum slots per-process](https://devblogs.microsoft.com/oldnewthing/20170712-00/?p=96585) and FLS has [128 maximum slots per-process](https://ntdoc.m417z.com/fls_maximum_available)
    - glibc thread-specific data has [1024 maximum keys per-process](https://elixir.bootlin.com/glibc/glibc-2.38/source/sysdeps/unix/sysv/linux/bits/local_lim.h#L63-L64)
3. Thread-local storage has subpar performance
    - Thread-local storage slots may not be as close together in memory as they would be in a single contiguous allocation using [`alloca`](https://www.gnu.org/software/libc/manual/2.23/html_node/Advantages-of-Alloca.html), which could lessen the [locality of reference](https://en.wikipedia.org/wiki/Locality_of_reference) performance benefit for memory accesses
    - Thread-local storage retrival and storage can introduce unnecessary overhead in the form of lookup cost and extra function calls
      - On Windows, a dynamic thread-local storage allocation with `TlsAlloc` due to [`TlsAlloc` acquires the shared PEB lock on every allocation](https://github.com/reactos/reactos/blob/54433319af31c2b49737469d36072153de375f4d/dll/win32/kernel32/client/thread.c#L1109) because it works by modifying the process-wide `Peb->TlsBitmap` data structure
      - On Windows, a dynamic thread-local storage allocation with `FlsAlloc` returns an FLS index which is implemented as a key in a binary array, which means that retrieving thread-local data means you need to search a binary array each time (it's bloat on top of a pointer or index)
      - glibc simply implements thread-specific data key as an index [stored as an unsigned integer](https://elixir.bootlin.com/glibc/glibc-2.38/source/sysdeps/nptl/bits/pthreadtypes.h#L48-L49) and [uses a per-key atomic swap to create keys](https://elixir.bootlin.com/glibc/glibc-2.38/source/nptl/pthread_key_create.c#L32-L34)
4. Thread-local data can complicate library unload or even make correctly unloading a library impossible
    - Global thread-local/thread-specific data always makes a library unloadable (e.g. marking a variable with the standard C `thread_local`, Windows `__declspec(thread)`, or glibc `__thread` attrbiutes, as well as, use of Windows `DLL_THREAD_ATTACH`)
      - The library's lifetime may be shorter or longer than the thread's lifetime
    - Local thread-local/thread-specific data is safe to use from a library while not making it unloadable as long as the given library owns the thread it set the thread-local data on and will join that thread before unloading
      - Joining a thread from a module destructor in unsafe on Windows, which generally makes safe library unloading more difficult to achieve
    - For example: If a worker thread dynamically loads a library, uses the contained subsystem which calls `FlsAlloc` to create thread-local data with a callback into its library code, uses the thread-local data by calling `FlsSetValue`, then the library is unloaded, and later the thread exits then a crash will occur! Simply calling `FlsFree` from `DLL_PROCESS_DETACH` will not solve the problem because the thread could have exited before your library was unloaded.
    - Another example: If the `DLL_THREAD_ATTACH` of a dynamically loaded library sets some per-thread data, a new thread is created anywhere in the process, then the library is unloaded before that thread exits, the resources that the `DLL_THREAD_DETACH` of the now unloaded DLL would have cleaned up, will be leaked
    - Unfortunately, [MacOS has already been hit with this issue](https://github.com/rust-lang/rust/issues/28794#issuecomment-368693049)
5. It is easy to accidentally use a thread-local data slot that is not yours thus creating an instability or an [application compatibility issue](https://devblogs.microsoft.com/oldnewthing/20221128-00/?p=107456) where memory sanitizers would have been able to proactively catch an issue with traditional pointers
6. Thread-local data is easy to misuse outside of its one valid use case: Giving each thread gets its own isolated instance of some data
    - Valid use cases: Passing a key created by `pthread_key_create` between threads so each thread has their own instance of some data, `thread_local int thread_local_counter = 0;` in an application (because global thread-local data can make libraries unloadable), or [`errno`](https://en.wikipedia.org/wiki/Thread-local_storage#Usage) by the operating system
    - An application programmer may misuse thread-local data in a way that unnecessarily extends memory lifetime until the end of a thread, which could be a waste when typical stack memory provides fine-grained memory lifetime management and can also automatically clean up resources with a great pattern like [RAII](https://en.cppreference.com/w/cpp/language/raii)
    - Instead of creating a structured function-oriented program or subsystem that cleanly passes through values and keeps track of memory in block scope, a programmer can easily use thread-local storage to be lazy by allocating FLS slots with a custom cleanup FLS callback to run at thread exit when managing the data's lifetime in a smaller block scope would have created a cleaner and more coherent codebase

## The PEB Problem

In Windows, the Process Environment Block (PEB) stores data that is global to the process. As opposed to linking with [data symbols](code/windows/data-export/README.md) made available by libraries or passing a flag when the value is constant, there are several reasons why the PEB is a bad model for sharing process-wide data:

### Adds Process Startup Overhead

Every process must initialize its PEB, a large and complex data structure, when it starts up. This fixed cost is an inefficient burden that contributes to slow process creation times on Windows.

The PEB resides in its own memory mapping that the kernel allocates and performs some initialization on before the first line of user-mode code runs (e.g. the kernel initializes `PEB.BeingDebugged`). The address of this independent memory mapping must be randomized in every process for security purposes, which also increases process startup overhead.

### Promotes Centralization

By its very definition, the PEB centralizes state into one process-wide block. The contents of this block are dictated only by Microsoft. Clumping together unrelated data from separate subsystems makes the operating system more monolithic as opposed to an improved way of functioning whereby libraries, each of which is its own subsystem, simply choose data symbols to export.

Centralizing state is also bad because it encourages coarse-grained locking, thus increasing lock contention, as is true for the PEB with its single `PEB.FastPEBLock` for synchronizing access even to unrelated members in this data structure. Windows calls this critical section lock "fast" presumably because it has a spin count attached to it that optimizes for the typically short acquisition times of this lock. However, a heuristic that improves performance by busy waiting is not a better solution than implementing fine-grained locking, thus reducing waits to begin with. Common scenarios where acquiring the PEB lock is necessary includes [creating thread-local data](https://github.com/reactos/reactos/blob/54433319af31c2b49737469d36072153de375f4d/dll/win32/kernel32/client/thread.c#L1109) and accessing environment variables, whereas Unix systems typically use purpose-built locks or avoid unnecessary locking in single-threaded programs.

### Elevates Backward Compatibility Risk

The PEB's definition is well-known with [some of its members being made public by Microsoft](https://learn.microsoft.com/en-us/windows/win32/api/winternl/ns-winternl-peb). The overt process-wide nature of the PEB and its public members could lead applications to depend on its exact layout or the existence of certain members. In contrast, data symbols do not require positioning into any exact layout and can easily be versioned (e.g. glibc supports symbol versioning). When the data is constant (for example, in the case of the handle returned by `GetProcessHeap`, which internally goes through PEB), passing through a flag to an API does an even better job at reducing backward compatability risk by not exposing any data type to applications.

### Weakens Security

Through a thinly veiled segment register, the PEB exposes pointers to various valuable targets and data structures. Most prominently, using its contained `PEB_LDR_DATA` structure, the PEB makes it trivial for shellcode to extract the base address of any module in the process, serving as a universal technique for breaking the ASLR of all modules in the process even if an attacker starts out by only knowing where one module is located. [Accessing `PEB_LDR_DATA` is a part of virtually all Windows shellcodes.](https://www.exploit-db.com/shellcodes?platform=windows)

### Accessing the PEB is Slow

[Accessing the PEB requires going through the TEB to reach it](https://github.com/reactos/reactos/blob/56c8c8ab4f16633398356087ae9923d7b71bb467/sdk/include/ndk/rtlfuncs.h#L1111), which adds a layer of indirection. Alternatively, one could further increase indirection by calling the NTDLL exported function of [`RtlGetCurrentPeb`](https://github.com/reactos/reactos/blob/56c8c8ab4f16633398356087ae9923d7b71bb467/dll/ntdll/rtl/libsupp.c#L63-L68) to make PEB access slower by also introducing a function call plus the slight overhead of dynamic linking before going through the TEB.

Operating systems such as Windows use the the segment registers of an x86-64 CPU for accessing per-thread data. On the microarchitectural level, accessing addresses with a non-zero segment base is [documented to increase load latency](https://devblogs.microsoft.com/oldnewthing/20220919-00/?p=107195#:~:text=segment%20base). Registers outside of the general-purpose registers are also less likely to be cached in the CPU pipeline, leading to a slowdown.

After acquiring the PEB address, accessing the contained data is also slower because PEB members are typically references to where the data actually resides, usually within a module or on the heap. Thus, the PEB introduces another unnecessary layer of indirection.

### Conclusion

The PEB should never have existed. Microsoft [first added the PEB in Windows NT 3.1](https://www.geoffchappell.com/studies/windows/km/ntoskrnl/inc/api/pebteb/peb/index.htm#Layout:~:text=3.10) (the first Windows NT release), at the same time Windows gained the ability for dynamic linking. Despite this fact, how the PEB works is a callback to the time before dynamic linking, when data was centralized into one location and required manual symbol resolution to obtain. In modern Windows, the PEB continues to exist and Microsoft is not shy about expanding it, undermining the gift of dynamic linking with each new member it receives.

## When Initialization Fails

Like any code, code that runs during initialization can fail. Resource allocation failure or anything involving I/O is a common example of code that can unpredictably fail. For instance, a memory allocation operation (e.g. `malloc`) could fail due to being out of memory. A close operation can also fail, even on a regular file, but implementations commonly abstract this fact away to prevent resource starvation. For example, Windows tries to ensure a close operation on a regular file will not fail by [letting `CloseHandle` on a file object](https://github.com/reactos/reactos/blob/ec5b1422bc7c1a1ac6a1b8b5933acce25cd3efbd/ntoskrnl/io/iomgr/file.c#L2228-L2229) [continually reissue a failed I/O request packet (IRP) allocation](https://github.com/svn2github/reactos/blob/d461c3f6a3cb7ce06d1d63a3370556f71d305b21/ntoskrnl/io/iomgr/irp.c#L654-L667) (although, allocating the IRP upfront at object creation to ensure closing an object is always possible would be the more robust solution with a trade-off that the object has slightly greater overall memory usage). Since code may always fail, it is important to have robust handling for when, not if, that failure case arises. Let's see what happens when initialization code at the module scope fails on different platforms and throughout different languages.

### Unix

Unix module constructors cannot fail:

```c
__attribute__((constructor))
int my_init_1() {
    // Return type is ignored
    return 0;
}

__attribute__((constructor))
int my_init_2() {
    // Return type is ignored
    return 1;
}
```

This fact is a shortcoming of module constructors on the Unix platform. Alternatively, a programmer could do something like this to fail if an initialization operation fails:

```c
void* alloc;

__attribute__((constructor))
int init_my_var() {
    size_t size = 1024;
    alloc = malloc(size);

    if (alloc == NULL) {
        // Allocation failed: handle it
        fprintf(stderr, "malloc(%zu) failed\n", size);
        // Exit abruptly
        // Note: Do not attempt gracefully exiting the process with exit(EXIT_FAILURE) here because that leads to undefined behavior on whether module destructors will be called (see: code/glibc/library-init-exits-fini)
        abort();
    }
}

// Individual constructors should be kept small and granular
// Initializing every independent variable at the module scope should have its own constructor
```

However, the problem with taking this approach is that it ends the entire process when a library is not supposed to affect the state of the broader application. Abruptly exiting could be acceptable if the library is a dynamically linked dependency of the given application, in which case library initialization failure translates into the process ending, anyway. But, if the process dynamically loads a library during process run-time and that library fails its initialization, then abruptly exiting is a bad solution.

This issue is forgivable because it is coherent with the minimal process architecture of Unix systems that favors creating new processes over dynamically loading libraries into an existing process, anyway (there are lots of other benefits generally associated with the minimal process architecture of Unix). However, the absence of a failure mechanism for module constructors on Unix leaves the system unequipped to fully support dynamic library loading where there is a valid use case for the functionality.

To fix this problem, I advise implementing a solution that pairs constructors and destructors in their respective `.init_array` and `.fini_array` lists. The `__attribute__((constructor))` macro could be extended to support matching a constructor function with its respective destructor function using a pair name property. If a constructor returns `false` or `0` for failure, navigate to the `.fini_array` at the same position minus one from the current position in the `.init_array` and run those destructors in reverse of the constructors. Keeping the `.init_array` and `.fini_array` lists separate is good for maintaining performance by locality of reference while running the entries of these lists. Additionally, we perform better by only keeping track of where we are in the constructor list and matching that to the destructor list instead of recording some every constructor we run, which would be suboptimal. `dlopen` can then unload the library and return a `NULL` pointer to indicate that the library failed to load and we can either reuse or create new `errno` error type for indicating that library initialization failed (there is already a family of `ELIB` error types so maybe create `ELIBINIT` or probably better would be reusing `ELIBEXEC`). For source code compatability, it could reasonably be assumed that all currently existing `void` constructors intend to return a truthy value that indicates a successful initialization (i.e. even though `void` would usually imply zero, the compiler can assume success by defaulting to returning one for constructor functions that do not have a return type). Unix module initialization functions currently do not benefit from having a return type other than void and are only called once by design, so breakage resulting from other functions calling an initialization function and expecting a certain result should not occur. Binary compatability could also be obtained through versioning of the ELF format.

### Windows

On Windows a module-scope constructor or `DllMain` can return `FALSE` to indicate failure or `TRUE` to indicate success:

```c
LPVOID alloc;

BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        alloc = HeapAlloc(GetProcessHeap(), 0, 1024);
        // alloc != NULL would also work and the compiler will create the same code
        return (BOOL)alloc;
        break;
    }

    return TRUE;
}
```

As you can see, `FALSE` is naturally the correct error value for module initialization because APIs typically return a `NULL` pointer if a resource allocation fails, which both cast to zero as an integer.

But, there is a problem. On Windows, the [`DLL_PROCESS_DETACH` of a library runs even if the `DLL_PROCESS_ATTACH` of a library fails](https://learn.microsoft.com/en-us/windows/win32/dlls/dllmain#:~:text=The%20DLL%20is%20being%20unloaded%20from%20the%20virtual%20address%20space%20of%20the%20calling%20process%20because%20it%20was%20loaded%20unsuccessfully):

```c
LPVOID alloc;

BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        alloc = HeapAlloc(GetProcessHeap(), 0, 1024);
        return (BOOL)alloc;
        break;
    case DLL_PROCESS_DETACH:
        // !!! If HeapAlloc fails causing the module constructor to also fail, the
        // module destructor will still run causing HeapFree on a NULL pointer !!!
        HeapFree(GetProcessHeap(), 0, alloc);
        break;
    }

    return TRUE;
}
```

[The Windows loader will run `DLL_PROCESS_DETACH` immediately after `DLL_PROCESS_ATTACH` if the latter fails.](code/windows/library-init-fail-detach-routine/README.md) This behavior is flawed and [inconsistent with other paradigms that implement failable constructors such as C++](code/windows/cpp-object-constructor-fail-constructor-test-destruction/README.md), Rust, and all other programming languages. Instead, a destructor should only run on fully contructed objects and constructors should verify that each step of initialization suceeded before proceeding, and if not [gracefully failing early by backing out](code/windows/library-init-fail-detach-routine/README.md#hypothetical-the-opposite-result). It does not make sense to send a detach/unload message to a DLL that was never fully attached/loaded in the same way that it does not make to destroy an object was never created. In addition, [fail-fast](https://en.wikipedia.org/wiki/Fail-fast_system) operation is a strong design principle for creating robust software. If an error occurs then fail as early as possible, always. The earlier failure happens, the better the chance an error can be recovered from and that it does not cause unwanted side effects.

This issue can easily be worked around in the module scope with little to no side effects because the code is still running at an expected time in the module destructor (the MSVC compiler implements this work around when calling C++ constructors and destructors using glue code compiled into the given DLL in a function named `dllmain_dispatch`). However, it remains unfortunate that Windows did not implement failable constructors correctly.

### C++

In C++, constructors are not regular functions in that they cannot return a value to indicate success or failure. Instead, constructors in C++ can throw an exception to indicate failure. For instance, if allocating memory with the `new` keyword fails, it will throw a `std::bad_alloc` exception that can be caught.

Before C++ was standardized in 1988 with ISO/IEC 14882:1998, throwing from a constructor was considered poor practice because inconsistent or buggy implementations of stack unwinding made the outcome unpredictable. This is obviously no longer the case today; however, throwing an exception from a constructor can still be not ideal when that exception is being thrown from the constructor of an object that is in the module scope due to the question of where that exception will be caught being undefined or platform-dependent.

On Windows, throwing an exception from a module constructor will be caught by the Windows library loader and [translated to mean the library failed module initialization](code/windows/library-init-fail-exception/README.md). On the GNU loader, [throwing an exception from a module constructor will cause it to go uncaught by default, resulting in glibc calling `std::terminate` which in turn aborts the entire application by sending a POSIX abort signal.](code/glibc/library-init-fail-exception/README.md) In the case of a dynamic library load, an exception handler could be set up around a `dlopen` to avoid ending the process.

The problem with catching the exceptions in both these places though is that the exception is being caught too late. By the time the exception is caught, there is no opportunity to clean up the resources of the partially constructed module that were constructed before the failing constructor ran. Thus, resource leakage occurs.

Hypothetically, the glue code MSVC puts in `dllmain_dispatch` for running C++ module constructors and destructors could catch the exception earlier and do destruction of all the objects that constructed before the failing constructor. However, MSVC currently does not implement this functionality. A similar glue code workaround could be improvised for running C++ constructors and destructors on Unix systems, although it would be preferable for the functionality to come from the system-level as discussed earlier. Implementing one of these solutions on the affected platforms would allow for C++ constructors to fail without negatively impacting the rest of the process.

#### Replacing Exceptions with Returning the Error

The stack unwinding ability of exceptions promotes catching and handling exceptions late, when it may be difficult to correctly handle the error. Handling errors early is especially important in the module scope because each constructor is an independent routine that has no knowledge of the constructors that ran before it or the consturctors that will run after it. For this reason, it would be desirable if C++ constructors had the ability to return a `nullptr` on failure instead of throwing an exception (with all the error handling being done early as soon as the error occurs).

In addition to the promotion of late error handling, there are generally other strong arguments against exceptions such as poor embedded systems support, their timing signature being a poor fit for real-time applications, and how they complicate some exploit mitigations. For these reasons, C++ supports specifying `nothrow` on the `new` of dynamically allocated types so they return a `nullptr` on memory allocation failure instead of throwing a `std::bad_alloc` exception:

```cpp
// Dynamically allocate a primitve type or perhaps some RAII type
char* buffer = new (std::nothrow) char[1024];
```

As an extension to the `nothrow` capabilities of C++, I recommend that C++ adds the equivalent error handling functionality for C++ objects. When `nothrow` is specified during instance creation of an object, any throws that the object would have generated during its construction should instead result in returning a `nullptr` which the code calling the constructor would then take to mean that the constructor failed and act accordingly. Information about the type of error could still be stored in a per-thread variable similar to how `errno` works if necessary, although I maintain that error handling is always best done as early as possibly.

[At CppCon 2018, Andreas Weis did a good talk](https://www.youtube.com/watch?v=S7I66lZX_zM) about better mitigating the usability difficulties that C++ constructors have in relation to their reliance on exceptions for indicating failure (the workaround solution still requires avoiding the language-provided constructor, but doing so in a more tactful way).

### Rust

In Rust, constructors are regular functions and can return a value, such as the standard result type (`Result<T, E>`) to indicate success or failure. [Rust does not have exceptions.](https://doc.rust-lang.org/book/ch09-00-error-handling.html)

Due to cross-platform issues with operating system support for module construction (*cough* Windows *cough*), the Rust language does not allow objects that exist in the module scope to access the operating-system-provided module constructors and destructors. Although, access can still be achieved through the use of a crate that provides the functionality. Rust also does not provide builtin access to the operating system's dynamic library loading function (e.g. `dlopen` or `LoadLibrary`). A third-party package called `libloading` exists to serve this functionality, but it does require wrapping the code in `unsafe`.

## Historical Windows Library Loader Issues with Module Initialization

### Improper State Management of Library Dependencies

The legacy library loader incorrectly managed its state of library dependencies because it could depart from adhering to the dependency graph during its accounting or when deciding on the order of operations.

#### Problems in Reentrancy During Module Initialization

- Microsoft mostly built the legacy loader to be reentrant; however, how it performed module initialization was subject to crashes or correctness issues due to the loader's poor ability to enforce the correct order of operations when initializing modules upon being reentered
- Starting with Windows 8, the loader correctly stores and maintains the dependency graph as a [directed acyclic graph (DAG) data structure](https://www.geoffchappell.com/studies/windows/km/ntoskrnl/inc/api/ntldr/ldr_ddag_node.htm) thus resolving out-of-order module initialization problems that can occur when loading a library from `DllMain`
- The legacy loader [kept track of dependencies in a linked list](https://learn.microsoft.com/en-us/archive/blogs/mgrier/the-nt-dll-loader-dll_process_attach-reentrancy-step-1-loadlibrary) that it would form by [walking the import address tables (IATs)](https://github.com/reactos/reactos/blob/053939e27cbf4d6475fb33b6fc16199bd944880d/dll/ntdll/ldr/ldrpe.c#L307-L309) at library load thus giving the module initialization order, which had the effect of collapsing the data thereby making the loader [unable to adjust the initialization order](https://github.com/reactos/reactos/blob/053939e27cbf4d6475fb33b6fc16199bd944880d/dll/ntdll/ldr/ldrinit.c#L698-L740) in the reentrant case of calling `LoadLibrary` from `DllMain`
- Therefore, lossy storage of the module dependency graph meant that legacy loaders, even with the [initialization workaround added to `GetProcAddress`](https://learn.microsoft.com/en-us/archive/blogs/mgrier/the-nt-dll-loader-dll_process_attach-reentrancy-step-2-getprocaddress), could initialize a valid acyclic dependency graph formation in the incorrect order (**Todo:** Create and run a code test that demonstrates this difference across legacy and modern loader versions)
- Unix systems got initialization ordering right ever since the ELF format was originally added to Unix by [System V ABI Release 4](https://www.sco.com/developers/devspecs/gabi40.pdf) (released in 1988, five years before the first Windows NT release), which formally specified that initialization order must adhere to the dependency graph and that "initialization code for an object is invoked after the needed entries for that object have been processed", where the needed entries for an object can change due to a `dlopen` operation reentering the loader

#### Difficulty Processing Library Unloads

- For library unloads, the legacy loader [re-walked the IATs](https://github.com/reactos/reactos/blob/053939e27cbf4d6475fb33b6fc16199bd944880d/dll/ntdll/ldr/ldrutils.c#L109-L114) (necessitating inefficient translation back to the in-memory libraries) to [determine which modules required deinitializing and unloading](https://github.com/reactos/reactos/blob/053939e27cbf4d6475fb33b6fc16199bd944880d/dll/ntdll/ldr/ldrapi.c#L1338-L1339), which was an error-prone process that went through multiple iterations to get right (and even then the implementation may have always been a bit broken)

### The Windows `GetModuleHandle` function is broken

- [`GetModuleHandle` from `DllMain`](#assessing-the-brokenness-of-getmodulehandle-and-getmodulehandleex-functions) is problematic because it assumes a DLL is already loaded when it may not be yet or has only partially loaded
- With the release of an `Ex` function and patchwork to `GetProcAddress` (commonly but not necessarily used after `GetModuleHandle`), Microsoft has mostly fixed this issue

## COM is Bloatware

**Note:** WORK IN PROGRESS! Nothing here is done or fully fleshed out. I still have plans on what I want to do here that I need to implement.

At its core, [COM is a binary standard for software module interaction in the object-oriented paradigm](#what-is-com). Here, we will explain why this concept of the original motivation behind COM was fundamentally flawed. Then, we will cover some of the bloated abstractions COM makes that only serve to confuse through complexity and obfuscate the real problems and primitives.

### In-Process COM Reinvents C++ and Dynamic Linking

C++ is an object-oriented programming language for creating a module based on the object-oriented paradigm. C++ modules (e.g. EXEs or DLLs) can interact by utilizing dynamic linking to [import and export C++ interfaces](code/windows/cpp-object-oriented-linkage/README.md).

These two elements of object-oriented programming and dynamic linking make up what COM is. So, what is the point of COM? Don Box, creator of COM, realizes this and so he made the entire first chapter of his book titled *Essential COM* on *COM as a Better C++*, where the core issue he keeps running into with C++ when it comes to its use between modules is that the language does not have binary encapsulation. Then, he goes over the run-time discoverability and extensibility benefits that COM integrates, and COM's use of reference-counting for lifetime management:

> <b>Chapter 1        COM as a Better C++</b><br>
> ................... Software Distribution and C++<br>
> ................... Dynamic Linking and C++<br>
> ................... C++ and Portability<br>
> ................... Encapsulation and C++<br>
> ................... Separating Interface from Implementation<br>
> ................... Abstract Bases as Binary Interfaces<br>
> ................... Runtime Polymorphism<br>
> ................... Object Extensibility<br>
> ................... Resource Management<br>
> ................... Where Are We?

The *COM as a Better C++* chapter states that the lack of binary encapsulation in C++ is a consequence of each vendor's compilers and linkers (e.g. Microsoft MSVC, GNU GCC, Watcom, Borland, more recently Clang, etc.) being incompatible with each other due to them all representing C++ objects and constructs with different internal data structures and naming schemes (i.e. name mangling). Therefore, a library exposing C++ interfaces built with Microsoft MSVC cannot be used by a C++ module built with GNU GCC, for instance.

However, introducing COM does not solve this inherit problem, [it just introduces another implementation](https://xkcd.com/927/) (like adding another compiler/linker into the mix). From its inception, COM has been a technology that tightly integrates with and was built for one platform: Microsoft Windows. COM is merely a repaint over Microsoft's [Object Linking and Embedding (OLE)](https://en.wikipedia.org/wiki/Object_Linking_and_Embedding) technology used in Microsoft office products and Internet Explorer. COM changed all the interfaces of OLE such that they do not accept any Win32 types, instead using some abstract COM-specific; however, the architectural inner-workings of COM remain closely tied to how Windows works like with how every call to `CoCreateInstance` does dynamic library loading (`LoadLibrary`) which while always common on Windows, remains uncommon on Unix platforms due to its architectural preference for minimal processes. Unix platforms historically and currently being the leading competitor to Windows NT, this fact alone made it obvious that COM was never going to be something that other vendors would use to develop in-process components for systems outside of Windows. COM also depended and still depends on other concepts like a central IPC/RPC server for object activation and a central in-memory registry database that has always mapped naturally to Windows internals but not to other platforms. So, implementing COM well on other platforms would have required implementing large chunks of Windows. In addition, while the concept of COM from a binary standard perspective is ["platform-independent"](#what-is-com) in theory, in practice COM components typically call Windows-specific APIs due to COM's heritages as a Microsoft technology that was integrated into Windows at no cost starting from Windows 95 (on DOS) and Windows NT 4.0 (released 1996) with only few Unix ports. Further, even with the few Unix ports there were, they were [primarily only for the purpose](https://news.microsoft.com/source/1997/05/05/digital-and-hewlett-packard-to-support-microsofts-com-on-unix-platforms/) of [getting Microsoft Windows technology to work on other platforms](https://news.microsoft.com/source/1998/09/10/microsoft-and-compaq-agree-to-implement-select-windows-nt-components-in-compaqs-digital-unix/), not because the system developers wanted to start developing their own native operating system components in COM. The only company involved in developing in-process COM applications where the ABI compatibility factor came into play was Microsoft since most others only used COM for interoperability with Windows through distributed objects (using the DCOM network protocol) and in-process to work with other Microsoft-developed applications like Microsoft Office or Internet Explorer for Unix. In which case, Microsoft could have just used their own compiler to work with modules in the object-oriented paradigm while also steering clear of ABI compatibility issues between C++ compiler vendors. Since COM has always been tidally locked to the Windows platform anyway, a developer could just suffice to use the Microsoft C++ compiler and linker toolchain when developing object-oriented Windows modules thereby solving the binary encapsulation problem (requiring use of the Windows C runtime or CRT when building a module is actually already common on Windows projects to ensure per-runtime types such as a `FILE` structure can be passed between modules). Of course, Microsoft would have to keep the Microsoft C++ compiler object binary interfaces stable then, but the same is true for COM. And so the question reemerges: What is and ever was the point of COM?

Finally, every current compiler and linker toolchain since around the early 2000s [*except* the modern Microsoft Visual C++ compiler](https://news.ycombinator.com/item?id=30399707) uses the [Itanium C++ standard ABI](https://itanium-cxx-abi.github.io/cxx-abi/abi.html) thereby solving cross-vendor C++ ABI differences. And so another question emerges: What is Microsoft doing still creating components for this arcane framework that should have provably never been created in the first place?

The run-time discoverability and extensibility properties of COM just refers to the `CoCreateInstance` and `QueryInterface` COM which are analogous to `LoadLibrary` and `GetProcAddress` in dynamic linking. Dynamic linking is an invention from the most influential operating system there ever written: [Multics](https://en.wikipedia.org/wiki/Multics)—the precursor to Unix.

Therefore, COM reinvents C++ and dynamic linking by repeating them then putting them into one architectural layer.

Other Notes:

Interface discovery is a special case

COM has versioning but glibc dynamic linker has that simply as an extension with a bit of metadata so it never justified the creation of COM

OO perspective but C++ is OO and it uses dynamic linking because you can export C++ symbols at the module scope and is language neutral because any language can make C/C++ FFI calls

In-Process COM is by far the most commonly used.

### COM Does Nothing to Help with Versioning

COM bikesheds the problem

From *Essential COM*:
One common solution to the versioning problem is to rename the DLL each
time a new version is produced. This is the strategy taken by the Microsoft
Foundation Classes (MFC). When the version number is encoded into the
DLL’s file name (e.g., FastString10.DLL, FastString20.DLL), clients always
load the version of the DLL that they were built against, irrespective of what
10 C H A P T E R 1 : C O M A S A B E T T E R C + +
Figure 1.3 C++ and Encapsulation
s i z e o f ( F a s t S t r i n g ) = = 4
s i z e o f ( F a s t S t r i n g ) = = 8
s i z e o f ( F a s t S t r i n g ) = = 8
s i z e o f ( F a s t S t r i n g ) = = 4
Application A
Version 2.0
FastString.dll
Version 2.0
Application B
Version 1.0
Application C
Version 1.0
other versions may be present on the system. Unfortunately, over time, the
number of versioned DLLs present on the end-user’s system could conceivably
exceed the number of actual client applications due to poor software configu-
ration practices. Simply examining the system directory of any computer that
has been in use for more than six months would reinforce this.

### COM is Unintuitive

Microsoft makes very complex and unintuitive APIs -> APIs are misused leading to bugs that application compatibility issues -> blames developers for not using it correctly.

Multithreading is not just some magical good thing because factoring work is a non-trivial problem

lifetimes exist

concurrency exists

From *Essential COM*: It is worth noting that there is an inherent race condition in the
CoFreeUnusedLibraries/DllCanUnloadNow protocol. It is possible that oneCLASSES AND IDL
thread may be executing the final release on the last instance exported from
a DLL while a second thread is simultaneously executing the CoFreeUnused-
Libraries routine. COM takes every possible precaution to avoid this situa-
tion. In particular, the Windows NT 4.0 Service Pack 2 implementation of
COM added a special facility to address this potential race condition. The
Service Pack 2 version of the COM library detects that a server DLL has been
accessed from multiple threads and, instead of unloading the DLL immedi-
ately from within CoFreeUnusedLibraries, COM enqueues the DLL onto a list
of DLLs that need to be freed. COM will then wait an unspecified period of
time before it will free these idle server DLLs to ensure that no residual
Release calls are still being executed.16 This means that in multithreaded en-
vironments, it may take considerably longer for a DLL to unload from its client
than is expected.

https://github.com/reactos/reactos/blob/cfcc8d85b2b3065d895925eb32837a83019eed98/dll/win32/ole32/compobj.c#L1148-L1151 (ReactOS incorrectly classifies this as a strategy "to cope for programs that have races between last object destruction and threads in the DLLs", when in fact this a model defect of COM)

Lastly, a gentle reminder of the [KISS design principle](https://en.wikipedia.org/wiki/KISS_principle) that is true of all engineering: The genius sees elegance in simplicity; the fool is captivated by convolution.

### Callback Bloat

COM supports a feature known as ["connectable objects"](https://learn.microsoft.com/en-us/windows/win32/com/events-in-com-and-connectable-objects) which [is just a callback](https://learn.microsoft.com/en-us/windows/win32/api/ocidl/nf-ocidl-iconnectionpoint-advise#parameters) with run-time discoverability tacked onto it. This run-time enumeration ability is similar to the discoverable nature of COM interfaces by calling `QueryInterface` on a COM object. Further, `QueryInterface` itself is matched by `GetProcAddress` in the dynamic linking API of Win32.

Although a callback precisely describes the communication model a connectable object and its corresponding "connection point" offers, there is not a single occurrence of using this straightforward terminology to describe the "technology" in any Microsoft documentation or in Microsoft Developer Blogs, instead opting for its description as a unique ["architecture"](https://learn.microsoft.com/en-us/windows/win32/com/architecture-of-connectable-objects) and even its own ["model"](https://devblogs.microsoft.com/oldnewthing/20130611-00/?p=4113#:~:text=Today%2C%20the%20connection%20point%20model.).

This obfuscation of such a simple primitive exemplifies the overengineering that went into making COM.

### Single-Threaded COM Servers are Expensive

COM servers in single-threaded apartment (STA) mode are implemented as Windows objects, and [Window objects are expensive](https://devblogs.microsoft.com/oldnewthing/20050315-00/?p=36183). Using the STA mode is typical in GUI components and in cases where, according to Microsoft, [the need for performance does not outweigh the complexity introduced by using COM in multi-threaded apartment (MTA) mode](https://learn.microsoft.com/en-us/windows/win32/com/processes--threads--and-apartments#the-apartment-and-the-com-threading-architecture).

It is still the case in modern Windows that an STA COM servers is internally just a Window object that are used in a [Windows message queue](https://en.wikipedia.org/wiki/Message_loop_in_Microsoft_Windows):

```
0:000> k
 # Child-SP          RetAddr               Call Site
00 00000000`09d7f018 00007ffb`eeef7039     USER32!CreateWindowExW
01 00000000`09d7f020 00007ffb`eeef6f32     combase!GetOrCreateSTAWindow+0x91 [onecore\com\combase\dcomrem\chancont.cxx @ 563]
02 00000000`09d7f090 00007ffb`eee7d466     combase!OXIDEntry::StartServer+0x22 [onecore\com\combase\dcomrem\ipidtbl.cxx @ 1415]
03 (Inline Function) --------`--------     combase!CComApartment::StartServer+0x89 [onecore\com\combase\dcomrem\aprtmnt.cxx @ 1225]
04 00000000`09d7f0c0 00007ffb`eee7d5c9     combase!InitChannelIfNecessary+0xf6 [onecore\com\combase\dcomrem\channelb.cxx @ 1028]
```

### COM is a Security Liability

The COM framework contained in `combase.dll` is full of layered indirection (e.g. class factories, proxies/stubs, marshaling code, dispatcher code, callbacks, and glue code) that make it the perfect farm for [weird machines](https://en.wikipedia.org/wiki/Weird_machine) that bypass modern exploit mitigations.

In a [2024 case study](https://media.defcon.org/DEF%20CON%2032/DEF%20CON%2032%20presentations/DEF%20CON%2032%20-%20WangJunJie%20Zhang%20YiSheng%20He%20-%20Defeating%20magic%20by%20magic%20Using%20ALPC%20security%20features%20to%20compromise%20RPC%20services.pdf) (see: slide 61), a gadget in COM was used to bypass Microsoft's leading eXtended Flow Guard (XFG) mitigation. The attacker found a function (`combase!ImmediateCallback<lambda_...>::CallbackFunction` which in turn called `LoadLibraryWithLogging`) in `combase.dll` with a [matching signature](https://blog.quarkslab.com/how-the-msvc-compiler-generates-xfg-function-prototype-hashes.html#a%20hands-on%20hash%20calculation%20exercise) for an indirect call (`_guard_xfg_dispatch_icall_fptr`) to the function being called by the `RPCRT4.dll`. The intended destination for this indirect call was a function residing in `RPCRT4.dll`, but the matching signature of a function in `combase.dll` allowed for control flow redirection by the attacker. This lookalike function loaded the library specified by an argument thus allowing the attacker to achieve arbitrary code execution (ACE) at the System level because the vulnerability he found in ALPC gave him sufficient control of the registers leading up to this indirect call. Microsoft has since patched this exact instance of XFG bypass but there are undoubtedly tons more still here.

While it is true that any sufficiently complex system will have weird machines that may be susceptible to exploitation, COM stands out as a bottomless pit of novel gadgets that are routinely mapped into process memory thanks to its ubiquity throughout the Windows API.

### Distributed Objects are Bad

COM supports [location transparency](https://en.wikipedia.org/wiki/Location_transparency) through the DCOM network protocol, meaning that a program can operate on a COM object that exists on a remote host. Thus, the [fallacies of distributed computing](https://en.wikipedia.org/wiki/Fallacies_of_distributed_computing) apply to COM, making its remote use a highly [leaky abstraction](https://en.wikipedia.org/wiki/Leaky_abstraction). If system-level code relies on this abstraction without regard to its many potential pitfalls then [confidentiality, integrity, or availability](https://en.wikipedia.org/wiki/Information_security#CIA_triad) of the system or its communications could be compromised.

The fallacies of distributed computing are:

> 1. The network is reliable;
> 2. Latency is zero;
> 3. Bandwidth is infinite;
> 4. The network is secure;
> 5. Topology doesn't change;
> 6. There is one administrator;
> 7. Transport cost is zero;
> 8. The network is homogeneous;

Distributed objects were once the hot new thing but now they are mostly a thing of the past. Simpler ways of communicating between machines have won out.

### Conclusion

...

COM was Microsoft's attempt at vendor lock in of all system APIs that luckily failed.

CLR is COM internally but lives on because it is a high-level language so no manual reference counting makes the overly abstract nature of COM tolerable in some scenarios

## Symbol Lookup Operating System Comparison

Let's review how looking up library symbols works on Windows with the `GetProcAddress` function compared to on POSIX-compliant Unix systems with the `dlsym` function, specifically its implementation by the GNU loader. Windows `GetProcAddress`, where `Proc` is short for procedure, is inaccurately named because this function can resolve the linkage of any symbol, [data](code/windows/data-export/README.md) or code (the address returned by `GetProcAddress` can arbitrarily be casted to a data type or a function prototype). For this reason, we use the symbol terminology even in our Windows review of `GetProcAddress`.

The Windows `GetProcAddress` and POSIX `dlsym` functions are platform equivalents because they resolve a symbol name to its address. They differ because `GetProcAddress` can only resolve function export symbols, whereas `dlsym` can resolve any global (`RTLD_GLOBAL`) or local (`RTLD_LOCAL`) symbol. There's also this difference: The [first argument of `GetProcAddress`](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getprocaddress#parameters) requires passing in a module handle. In contrast, the first argument of `dlsym` can take a module handle, but it also accepts one of the [`RTLD_DEFAULT` or `RTLD_NEXT` flags](https://pubs.opengroup.org/onlinepubs/9699919799/functions/dlsym.html#tag_16_96_07) (or "pseudo handles" in more Windows terminology). Let's discuss how `GetProcAddress` functions first.

`GetProcAddress` receives an `HMODULE` (a module's base address) as its first argument. The loader maintains a red-black tree sorted by each module's base address called `ntdll!LdrpModuleBaseAddressIndex`. `GetProcAddress` ➜ `LdrGetProcedureAddressForCaller` ➜ `LdrpFindLoadedDllByAddress` (this is a call chain) searches this red-black tree for the matching module base address to ensure a valid DLL handle. Searching the `LdrpModuleBaseAddressIndex` red-black tree mandates acquiring the `ntdll!LdrpModuleDataTableLock` lock. If locating the module fails, `GetProcAddress` sets the thread error code (retrieved with the `GetLastError` function) to `ERROR_MOD_NOT_FOUND` and returns early. `GetProcAddress` receives a symbol name as a string for its second argument. `GetProcAddress` ➜ `LdrGetProcedureAddressForCaller` ➜ `LdrpResolveProcedureAddress` ➜ `LdrpGetProcedureAddress` calls `RtlImageNtHeaderEx` to get the NT header (`IMAGE_NT_HEADERS`) of the PE image. `IMAGE_NT_HEADERS` contains optional headers (`IMAGE_OPTIONAL_HEADER`), including the image data directory (`IMAGE_DATA_DIRECTORY`, this is in the `.rdata` section). The data directory (`IMAGE_DATA_DIRECTORY`) includes multiple directory entries, including `IMAGE_DIRECTORY_ENTRY_EXPORT`, `IMAGE_DIRECTORY_ENTRY_IMPORT`, `IMAGE_DIRECTORY_ENTRY_RESOURCE`, [and more](https://learn.microsoft.com/en-us/windows/win32/api/dbghelp/nf-dbghelp-imagedirectoryentrytodata#parameters). `LdrpGetProcedureAddress` gets the PE's export directory entry. The compiler sorted the PE export directory entries alphabetically by symbol name ahead of time. `LdrpGetProcedureAddress` performs a [binary search](https://en.wikipedia.org/wiki/Binary_search_algorithm) over the sorted symbol names, looking for a name matching the symbol name passed into `GetProcAddress`. If locating the symbol fails, `GetProcAddress` sets the thread error code to `ERROR_PROC_NOT_FOUND`. Locking isn't required while searching for an export because PE image exports are resolved once during library load and remain unchanged (this doesn't cover delay loading). In classic Windows monolithic fashion, `GetProcAddress` may do [much more than find a symbol](#getprocaddress-module-initialization-mitigation-implementations) on edge cases. Still, for the sake of our comparison, we only need to know how `GetProcAddress` works at its core.

GNU's POSIX-compliant `dlsym` implementation firstly differs from `GetProcAddress` because the former will not validate a correct module handle before searching for a symbol. Pass in an invalid module handle, and the program will crash; to be fair, you deserve to crash if you do that. Also, not validating the module handle provides a great performance boost. Depending on the `flags` passed to `dlopen` and the `handle` passed to `dlsym`, the GNU loader searches for symbols in a few ways. Most commonly, a symbol lookup occurs in the global scope (`RTLD_DEFAULT` handle to `dlsym`), which requires iterating the [`searchlist` in the main link map](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/dl-open.c#L106) then matching on the first symbol found within a library. Here, we will cover the most straightforward case when [calling `dlsym` with a handle to a library](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/dl-sym.c#L151) (i.e. `dlsym(myLibraryHandle, "myfunc")`). The ELF standard specifies the use of a hash table for searching symbols. `do_sym` (called by `_dl_sym`) calls `_dl_lookup_symbol_x` to find the symbol in our specified library (also referred to as an object). `_dl_lookup_symbol_x` calls [`_dl_new_hash`](https://elixir.bootlin.com/glibc/glibc-2.36/source/sysdeps/generic/dl-new-hash.h#L67) to hash our symbol name with the djb2 hash function ([look for the magic numbers](https://stackoverflow.com/a/13809282)). Recent versions of the GNU loader use this djb2-based hash function, which differs from the [old](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/dl-lookup.c#L449) [standard ELF hash function](https://elixir.bootlin.com/glibc/glibc-2.38/source/sysdeps/generic/dl-hash.h#L28) based on the [PJW hash function](https://en.wikipedia.org/wiki/PJW_hash_function#Other_versions). At its introduction, this new hash function [improved dynamic linking time by 50%](https://libc-alpha.sourceware.narkive.com/33Q6yg6i/patch-dt-gnu-hash-50-dynamic-linking-improvement). This improved hash function is a [de facto standard](https://en.wikipedia.org/wiki/De_facto_standard), it began as a GNU extension and was never part of the formal System V ABI specification, but its utility has made it commonplace across Unix-like systems. It is also worth noting that, in 2023, someone [caught an overflow bug](https://en.wikipedia.org/wiki/PJW_hash_function#Implementation) in the original hash function described by the System V ABI. `_dl_lookup_symbol_x` calls `do_lookup_x`, where the real searching begins. `do_lookup_x` filters on our library's `link_map` to see if it should disregard searching it for any reason. Passing that check, [`do_lookup_x` gets pointers](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/dl-lookup.c#L404) into our library's [`DT_SYMTAB` and `DT_STRTAB`](https://man7.org/linux/man-pages/man5/elf.5.html) ELF tables (the latter for use later while searching for matching symbol names). Based on our symbol's hash (calculated in `_dl_new_hash`), [`do_lookup_x` selects a bucket](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/dl-lookup.c#L423) from the hash table to search for symbols from. `l_gnu_buckets` is an array of buckets in our hash table to choose from. At build time during the linking phase, the linker builds each ELF image's hash table with the number of buckets, which adjusts [depending on how many symbols are in the binary](https://sourceware.org/git/?p=binutils-gdb.git;a=blob;f=bfd/elflink.c;h=c2494b3e12ef5cd765a56c997020c94bd49534b0;hb=aae436c54a514d43ae66389f2ddbfed16ffdb725#l6397). With a bucket selected, `do_lookup_x` [fetches the `l_gnu_chain_zero` chain for the given bucket and puts a reference to the chain in the `hasharr` pointer variable](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/dl-lookup.c#L427) for easy access. `l_gnu_chain_zero` is an array of [GNU hash entries containing standard ELF symbol indexes](https://elixir.bootlin.com/glibc/glibc-2.38/source/sysdeps/generic/ldsodefs.h#L56). The ELF symbol indexes inside are what's relevant to us right now. Each of these indexes points to a [symbol table entry](https://docs.oracle.com/cd/E19504-01/802-6319/chapter6-79797/index.html) in the `DT_SYMTAB` table. [`do_lookup_x` iterates through the symbol table entries in the chain until the selected symbol table entry holds the desired name or hits `STN_UNDEF` (i.e. `0`), marking the end of the array.](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/dl-lookup.c#L440) `l_gnu_buckets` and `l_gnu_chain_zero` inherit similarities in structure from the original ELF standardized [`l_buckets` and `l_chain`](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/dl-lookup.c#L451) arrays which the GNU loader still implements for backward compatibility. For the memory layout of the symbol hash table, see [this diagram](https://docs.oracle.com/cd/E23824_01/html/819-0690/chapter6-48031.html). There appears to be a [fast path](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/dl-lookup.c#L430) to quickly eliminate any entries that we can know early on won't match—passing the fast path check, `do_lookup_x`  calls [`ELF_MACHINE_HASH_SYMIDX`](https://elixir.bootlin.com/glibc/glibc-2.38/source/sysdeps/generic/ldsodefs.h#L56) to extract the offset to the standard ELF symbol table index from within the GNU hash entry. The GNU hash entry is a layer on top of the standard ELF symbol table entry; [in the old but standard hash table implementation, you can see that the chain is directly an array of symbol table indexes](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/dl-lookup.c#L454). Having obtained the symbol table index, `dl_lookup_x` [passes the address to `DT_SYMTAB` at the offset of our symbol table index](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/dl-lookup.c#L434) to the `check_match` function. [The `check_match` function then examines the symbol name cross-referencing with `DT_STRTAB` where the ELF binary stores strings to see if we've found a matching symbol.](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/dl-lookup.c#L90) Upon finding a symbol, `check_match` looks if the [symbol requires a certain version](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/dl-lookup.c#L95) (i.e. [`dlvsym` GNU extension](https://www.man7.org/linux/man-pages/man3/dlvsym.3.html)). In the [unversioned case](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/dl-lookup.c#L127) with `dlsym`, `check_match` [determines if this is a hidden symbol](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/dl-lookup.c#L150) (e.g. `-fvisibility=hidden` compiler option in GCC/Clang), and if so not returning the symbol. This loop restarts at the fast path check in `dl_lookup_x` until `check_match` finds a matching symbol or there are no more chain entries to search. Having found a matching symbol, `do_lookup_symbol_x` determines the symbol type; in this case, it's [`STB_GLOBAL`](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/dl-lookup.c#L499) and returns the successfully found symbol address. Finally, the loader internals pass this value back through the call chain until `dlsym` returns the symbol address to the user!

Note that GNU prefixes all [members of the `link_map` structure](https://elixir.bootlin.com/glibc/glibc-2.38/source/include/link.h#L95) with `l_` to clearly denote them as such across the glibc codebase. Similarly, GNU prefixes internal structure defintions with `r_`. These are custom [Hungarian notations](https://en.wikipedia.org/wiki/Hungarian_notation) that the code follows.

Also, note that, unlike the Windows `ntdll!LdrpHashTable` (*which serves an entirely different purpose*), the hash table in each ELF `DT_SYMTAB` is made up of arrays instead of linked lists for each chain (each bucket has a chain). Using arrays (size determined during binary compilation) is possible because the ELF images are not dynamically allocated structures like the `LDR_DATA_TABLE_ENTRY` structures `ntdll!LdrpHashTable` keeps track of. Arrays are significantly faster than linked lists because following links is a relatively expensive operation (e.g. you lose [locality of reference](https://en.wikipedia.org/wiki/Locality_of_reference)). In general, the hash table is a commonly used data structure because they typically outperform other means of storing and retrieving data.

Due to the increased locality of reference and a hash table being O(1) average and amortized time complexity vs a binary search being O(log n) time complexity, I believe that searching a hash table (bucket count optimized at compile time) and then iterating through the also optimally sized array as done by the GNU loader's `dlsym` is faster than the binary search approach employed by `GetProcAddress` in Windows for locating symbol addresses.

## How Does `GetProcAddress`/`dlsym` Handle Concurrent Library Unload?

The `dlsym` function [can locate](https://pubs.opengroup.org/onlinepubs/9699919799/functions/dlsym.html#tag_16_96_06) [`RTLD_LOCAL` symbols](https://pubs.opengroup.org/onlinepubs/9699919799/functions/dlopen.html) (i.e. libraries that have been `dlopen`ed with the `RTLD_LOCAL` flag). Internally, the [`dlsym_implementation` function acquires `dl_load_lock` at its entry](https://elixir.bootlin.com/glibc/glibc-2.38/source/dlfcn/dlsym.c#L52). Since `dlclose` must also acquire `dl_load_lock` to unload a library, this prevents a library from unloading while another thread searches for a symbol in that same (or any) library. `dlsym` eventually calls into `_dl_lookup_symbol_x` to perform the symbol lookup.

Answering this question on Windows requires a more in-depth two-part investigation of `GetProcAddress` and `FreeLibrary` internals:

For `GetProcAddress`, `LdrGetProcedureAddressForCaller` (this is the NTDLL function `GetProcAddress` calls through to) acquires the `LdrpModuleDatatableLock` lock, searches for our module `LDR_DATA_TABLE_ENTRY` structure in the `ntdll!LdrpModuleBaseAddressIndex` red-black tree, checks if our DLL was dynamically loaded, and if so atomically incrementing `LDR_DATA_TABLE_ENTRY.ReferenceCount`. `LdrGetProcedureAddressForCaller` releases the `LdrpModuleDatatableLock` lock. Before acquiring the `LdrpModuleDatatableLock` lock, note there's a special path for NTDLL whereby `LdrGetProcedureAddressForCaller` checks if the passed base address matches `ntdll!LdrpSystemDllBase` (this holds NTDLL's base address) and lets it skip close to the meat of the function where **`LdrpFindLoadedDllByAddress`** and **`LdrpResolveProcedureAddress`** occur. Towards the end of `LdrGetProcedureAddressForCaller`, it calls `LdrpDereferenceModule`, passing the `LDR_DATA_TABLE_ENTRY` of the module it was searching for a symbol in. `LdrpDereferenceModule`, assuming the module isn't pinned (`LDR_ADDREF_DLL_PIN`) or a static import (`ProcessStaticImport` in `LDR_DATA_TABLE_ENTRY.Flags`), atomically decrements the same `LDR_DATA_TABLE_ENTRY.ReferenceCount` of the searched module. If `LdrpDerferenceModule` senses the `LDR_DATA_TABLE_ENTRY.ReferenceCount` reference counter has dropped to zero (this could only occur due to a concurrent thread decrementing the reference count), it will delete the necessary module information data structures and unmap the module. By reference counting, `GetProcAddress` ensures a module isn't unmapped midway through its search.

The Windows loader maintains `LDR_DATA_TABLE_ENTRY.ReferenceCount` and `LDR_DDAG_NODE.LoadCount` reference counters for each module. The loader ensures there are no references to a module's `LDR_DDAG_NODE` (`LDR_DDAG_NODE.LoadCount` = 0) before there are no references to the same module's `LDR_DATA_TABLE_ENTRY` (`LDR_DATA_TABLE_ENTRY.ReferenceCount` = 0). This sequence is the correct order of operations for decrementing these reference counts. The `LdrUnloadDll` NTDLL function (public `FreeLibrary` calls this) calls `LdrpDecrementModuleLoadCountEx` which typically decrements a module's `LDR_DDAG_NODE.LoadCount` then, if it hits zero, runs `DLL_PROCESS_DETACH`. Lastly, `LdrUnloadDll` calls `LdrpDereferenceModule` which decrements the module's `LDR_DATA_TABLE_ENTRY.ReferenceCount`. Unloading a library (when `LoadCount` decrements for the last time from `1` to `0`) requires becoming the load owner (`LdrUnloadDll` calls `LdrpDrainWorkQueue`). Once the thread is appointed as the load owner (only one thread can be a load owner at a time), `LdrUnloadDll` calls `LdrpDecrementModuleLoadCountEx` again with the [`DontCompleteUnload` argument](#ldrpdecrementmoduleloadcountex) set to `FALSE` thus allowing actual module unload to occur instead of just decrementing the `LDR_DDAG_NODE.LoadCount` reference counter. With `LDR_DDAG_NODE.LoadCount` now at zero but the thread still being the load owner, `LdrpDecrementModuleLoadCountEx` calls `LdrpUnloadNode` to run the module's `DLL_PROCESS_DETACH` routine (`LdrpUnloadNode` can also walk the dependency graph to unload other now unused libraries). `LdrUnloadDll` then calls `LdrpDropLastInProgressCount` to decommission the current thread as the load owner followed by calling `LdrpDereferenceModule` to remove the remaining module information data structures and unmap the module. **The `LdrpDereferenceModule` function acquires the `LdrpModuleDatatableLock` lock while removing the module from the global module information data structures. Since `GetProcAddress` also appropriately acquires the `LdrpModuleDatatableLock` when searching global module information data structures and correctly utilizes module reference counts, this prevents a library from unloading while another thread searches for a symbol in that same library.** Note that many functions all throughout the loader call `LdrpDereferenceModule` (including `GetProcAddress` internally) so there can't be a race condition that causes a module now with a reference count of zero to remain loaded.

**The coarse-grained locking approach of GNU `dlsym` is the only place the Windows loader approach is superior for maximizing concurrency and preventing deadlocks.** I recommend that glibc switches use a more fine-grained locking approach like they already do with `RTLD_GLOBAL` symbols and their [global scope locking system (GSCOPE)](#gnu-loader-global-scope-symbol-protection). Note that acquiring `dl_load_lock` also allows `dlsym` to safely search link maps in the `RTLD_DEFAULT` and `RTLD_NEXT` pseudo-handle scenarios [without acquiring `dl_load_write_lock`](#gnu-loader-lock-hierarchy). However, that goal could also be accomplished by shortly acquiring the `dl_load_write_lock` lock in the `_dl_find_dso_for_object` function, so this is only a helpful side effect. Simply protecting symbol resolution through per-module reference counting is likely not viable due to global symbols. Although, might easier protection be doable by combining global scope locking (for global symbols) with module reference counting (for local symbols)?

Keep in mind, that if your program frees libraries whose exports/symbols are still in use (*after* locating them with `GetProcAddress` or `dlsym`), then you can expect your application to crash due to your own negligence. In other words, `GetProcAddress`/`dlsym` only protects from internal data races (within the loader). However, you the programmer are responsible for guarding against external data races. Said in another way again: The loader doesn't know what your program might do. So, the loader can only maintain consistency with itself.

## ELF Flat Symbol Namespace (Global Symbols, GNU Namespaces, and Unique Symbols)

Windows PE (EXE) and [MacOS Mach-O starting with OS X v10.1](https://developer.apple.com/library/archive/documentation/DeveloperTools/Conceptual/MachOTopics/1-Articles/executing_files.html) executable formats store library symbols in a two-dimensional namespace; thus effectively making every library its own namespace.

On the other hand, the Linux ELF executable format specifies a flat namespace such that two libraries sharing the same symbol name can collide. For example, if two libraries expose a `malloc` function, then the dynamic linker will not be able to differentiate between them. As a result, the dynamic linker recognizes the first `malloc` symbol definition it sees as *the* `malloc`, ignoring any `malloc` definitions that come later (assuming both libraries are loaded in the default `RTLD_GLOBAL` loading context and the symbols themselves are marked as global).

These namespace collisions have been the source of some bugs, and as a result, there have been workarounds to fix them. The most straightforward being: `dlsym(mySpecificLibraryHandle, "malloc")`. However, this fix does not cut it for implicitly linked symbols, so GNU had to devise some other solutions.

[Since 2004](https://sourceware.org/git/?p=glibc.git;a=commit;h=c0f62c56788c48b9fb36dc609c0a9f9db3667306), the glibc loader has supported a feature known as loader namespaces for separating symbols contained within a loaded library into a separate namespace. Creating a new namespace for a loading library requires calling [`dlmopen`](https://man7.org/linux/man-pages/man3/dlmopen.3.html) (this is a GNU extension). Loader namespaces allow a programmer to isolate the symbols of a module to its own namespace. There are [various reasons](https://man7.org/linux/man-pages/man3/dlmopen.3.html#NOTES) GNU gives for why a developer might want to open a library in a separate namespace and why `RTLD_LOCAL` is not a substitute for this functionality, such as in cases where an `RTLD_LOCAL` library can be promoted to `RTLD_GLOBAL` because another library takes a dependency on it (especially if the promoted library pollutes the namespace with many generic symbol names). The GNU loader places a hard limit of 16 maximum loader namespaces in a process. Some might say GNU loader namespaces are just a bandage patch over the core issue of flat ELF symbol namespaces, but it does not hurt to exist as an option.

[In 2009](https://sourceware.org/git/?p=glibc.git;a=commit;h=415ac3df9b10ae426d4f71f9d48003f6a3c7bd8d), the GNU loader received a new symbol type called `STB_GNU_UNIQUE`. In the `dl_lookup_x` function, determining the symbol type is the final step after successfully locating a symbol. [`STB_GNU_UNIQUE`](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/dl-lookup.c#L504) is one of these symbol types, and as the name implies, it is a GNU extension. The purpose of this new symbol type was to fix [a symbol collision problem in C++](https://gcc.gnu.org/onlinedocs/gcc/Code-Gen-Options.html#index-fno-gnu-unique) by giving precedence to `STB_GNU_UNIQUE` symbols over global symbols. During compilation, the `g++` compiler marks C++ symbols as being `STB_GNU_UNIQUE` to use this symbol type. In theory, the C++ [one definition rule (ODR)](https://en.wikipedia.org/wiki/One_Definition_Rule) works perfectly with global symbols because it enforces a single defintion for each symbol name in a process ([Windows DLLs break standards compliance with ODR](https://stackoverflow.com/questions/79270318/why-does-odr-not-apply-to-dll-when-dynamically-link-it-in-a-program)). In practice though, the real world can be more messy with different versions or implementations of the same C++ symbol name requring per-library linkage because one linkage cannot be agreed upon process-wide, even though this disagreement counts as an ODR violation in principle. While, `STB_GNU_UNIQUE` solves the symbol collision problem, it may not have been the best or most holistic solution to the problem. Due to the global nature of `STB_GNU_UNIQUE` symbols and their lack of reference counting (per-symbol reference counting could impact performance), their usage in a library makes [unloading](https://sourceware.org/git/?p=glibc.git;a=commit;h=802fe9a1ca0577e8eac28c31a8c20497b15e7e69) [impossible](https://sourceware.org/git/?p=glibc.git;a=commit;h=077e7700b30df967d9000ebe692894fc5d66df80) by design. On top of this, `STB_GNU_UNIQUE` introduced significant bloat to the GNU loader by adding a special, dynamically allocated hash table known as the `_ns_unique_sym_table` just for handling this one new symbol type, which, along with a lock (`_ns_unique_sym_table.lock`) for controlling access to this new data structure, is [included in the `_rtld_global`](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/rtld.c#L337) structure (the run-time loader structure, `rtld_global`, is an internal GNU loader structure containing member variables that are global to the loader). Searching for unique symbols also incurs a performance hit because it [additionally searches the unique symbol table](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/dl-lookup.c#L475-L507) if the located symbol, found in a [per-module symbol table](#symbol-lookup-operating-system-comparison) turns out to be of type `STB_GNU_UNIQUE`.

The ELF standard introducing a per-process symbol namespace was not an unthinkably bad decision because it is coherently rooted in the minimal process philosophy of Unix systems (i.e. conflicting symbols indicates splitting tasks into separate processes may be warranted). However, a per-library symbol namespaces, where each library cannot taint the namespace of a process, has consistently shown itself to be more flexible and robust in practice because each library file must already specify the library files it depends on for dynamic linking to take place, so it does not make much sense to then group all symbols in a process together by removing library-symbol association during symbol lookup. Acting like this association does not exist during symbol lookup when it really does is exhibting too much "magic" for a system-level component such as the dynamic linker.

The workarounds and gotchas that have accumulated due to the existence of a per-process symbol namespace indicates that symbol namespaces should have been two-dimensional long ago. Standards organizations could still update the ELF standard to support two-dimensional namespaces (although it may require a new version or a slight [ABI compatibility hack](https://github.com/mic101/windows/blob/6c4cf038dbb2969b1851511271e2c9d137f211a9/WRK-v1.2/base/ntos/rtl/rtlnthdr.c#L48-L52)). But, what exactly would be necssary to make 2D namespaces a reality on GNU/Linux? Well, let's take a page from someone who's done it ([in 2001 no less](https://en.wikipedia.org/wiki/Mac_OS_X_10.1)). According to Apple's documentation, their dymanic linker simply "adds the module name as part of the symbol name of the symbols defined within it" to accomplish 2D namespaces. As a result, this could be a very easy feature to add even on a module opt-in basis. Plus, a significant perk of [2D namespaces are that they allow for an optimzation that improves symbol resolution performance](https://developer.apple.com/library/archive/documentation/DeveloperTools/Conceptual/MachOTopics/1-Articles/executing_files.html#//apple_ref/doc/uid/TP40001829-97182-TPXREF112) in the `RTLD_DEFAULT` case. When mixing 1D and 2D namespace symbols (if that is desirable), a per-symbol flag (like `STB_GNU_UNIQUE`) could be added to differentiate the two symbol types.

**Side note:** We also need to [kill ELF interposition](https://www.facebook.com/dan.colascione/posts/10107358290728348) ASAP, then we can do away with the temporary compilation flag hacks currently working around it.

**Suggestion:** Since going 2D will require we have some way to denote module + symbol pairs in GDB (like Windows does in WinDbg, for instance, `ntdll!NtOpenFile`), I suggest using the `/` character as the separator. The only two characters that are illegal to have in a file name on Unix platforms is `/` and the null byte, so choosing slash for this purpose (e.g. `libc.so/printf`) will not limit any file nor symbol names (just split on the first instance of a `/` character to separate module + symbol name pairs) and is the most natural (where symbols in a module are sort of like a virtual file system), in my opinion. LLDB on MacOS uses a <code>`</code> (backtick) for the same purpose, but I do not like that because it conflicts with markdown.

## GNU Loader Global Scope Symbol Protection

The GNU loader uses GSCOPE, the global scope system, to ensure consistent access to `STB_GLOBAL` symbols (as it's known in the ELF standard; this maps to the [`RTLD_GLOBAL` flag of POSIX `dlopen`](https://pubs.opengroup.org/onlinepubs/009696799/functions/dlopen.html#tag_03_111_03)). [The global scope is the `searchlist` in the main link map.](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/dl-open.c#L106) The main link map refers to the program's link map structure (not one of the libraries). In the [TCB](https://en.wikipedia.org/wiki/Thread_control_block) (this is the generic term for the Windows TEB) of each thread is a piece of atomic state flag (this is *not* a reference count), which can hold [one of three states](https://elixir.bootlin.com/glibc/glibc-2.38/source/sysdeps/x86_64/nptl/tls.h#L211) known as the [`gscope_flag`](https://elixir.bootlin.com/glibc/glibc-2.38/source/sysdeps/x86_64/nptl/tls.h#L49) that keeps track of which threads are currently depending on the global scope for their operations. A thread uses the [`THREAD_GSCOPE_SET_FLAG` macro](https://elixir.bootlin.com/glibc/glibc-2.38/source/sysdeps/x86_64/nptl/tls.h#L225) (internally calls [`THREAD_SETMEM`](https://elixir.bootlin.com/glibc/glibc-2.38/source/sysdeps/x86_64/nptl/tcb-access.h#L92)) to atomically set this flag and the [`THREAD_GSCOPE_RESET_FLAG`](https://elixir.bootlin.com/glibc/glibc-2.38/source/sysdeps/x86_64/nptl/tls.h#L214) macro to atomically unset this flag. When the GNU loader requires synchronization of the global scope, it uses the [`THREAD_GSCOPE_WAIT` macro](https://elixir.bootlin.com/glibc/glibc-2.38/source/sysdeps/generic/ldsodefs.h#L1410) to call [`__thread_gscope_wait`](https://elixir.bootlin.com/glibc/glibc-2.38/source/sysdeps/nptl/dl-thread_gscope_wait.c#L26). Note there are two implementations of `__thread_gscope_wait`, one for the [Native POSIX Threads Library (NPTL)](https://en.wikipedia.org/wiki/Native_POSIX_Thread_Library) used on Linux systems and the other for the [Hurd Threads Library (HTL)](https://elixir.bootlin.com/glibc/glibc-2.38/source/htl/libc_pthread_init.c#L1) which was previously used on [GNU Hurd](https://en.wikipedia.org/wiki/GNU_Hurd) systems (with the [GNU Mach](https://www.gnu.org/software/hurd/microkernel/mach/gnumach.html) microkernel). GNU Hurd has [since](https://www.gnu.org/software/hurd/doc/hurd_4.html#SEC18) [switched to using NPTL](https://www.gnu.org/software/hurd/hurd/libthreads.html). For our purposes, only NPTL is relevant. The `__thread_gscope_wait` function [iterates through the `gscope_flag` of all (user and system) threads](https://elixir.bootlin.com/glibc/glibc-2.38/source/sysdeps/nptl/dl-thread_gscope_wait.c#L57), [signalling to them that it's waiting (`THREAD_GSCOPE_FLAG_WAIT`) to synchronize](https://elixir.bootlin.com/glibc/glibc-2.38/source/sysdeps/nptl/dl-thread_gscope_wait.c#L65). GSCOPE is a custom approach to locking that works by the GNU loader creating its own synchronization mechanisms based on [low-level locking primitives](https://elixir.bootlin.com/glibc/glibc-2.38/source/sysdeps/x86_64/nptl/tls.h#L222). Creating your own synchronization mechanisms can similarly be done on Windows using the [`WaitOnAddress` and `WakeByAddressSingle`/`WakeByAddressAll` functions](https://devblogs.microsoft.com/oldnewthing/20160823-00/?p=94145). Note that the [`THREAD_SETMEM`](https://elixir.bootlin.com/glibc/glibc-2.38/source/sysdeps/x86_64/nptl/tcb-access.h#L92) and [`THREAD_GSCOPE_RESET_FLAG`](https://elixir.bootlin.com/glibc/glibc-2.38/source/sysdeps/x86_64/nptl/tls.h#L217) macros don't prepend a `lock` prefix to the assembly instruction when atomically modifying a thread's `gscope_flag`. These modifications are still atomic because [`xchg` is atomic by default on x86](https://stackoverflow.com/a/3144453), and a [single aligned load or store (e.g. `mov`) operation is also atomic by default on x86 up to 64 bits](https://stackoverflow.com/a/36685056). If `gscope_flag` were a reference count, the assembly instruction would require a `lock` prefix (e.g. `lock inc`) because incrementing internally requires two memory operations, one load and one store. The GNU loader must still use a locking assembly instruction/prefix on architectures where memory consistency doesn't automatically guarantee this (e.g. AArch64/ARM64). Also, note that all assembly in the glibc source code is in AT&T syntax, not Intel syntax.

Understanding the fundamentals of how the GNU loader uses low-level locking to perform global scope synchronization requires knowing how a modern [futex](https://en.wikipedia.org/wiki/Futex) works. A modern POSIX mutex or Windows critical section are implemented using a futex or [futex-like mechanism](https://devblogs.microsoft.com/oldnewthing/20170601-00/?p=96265) under the hood, respectively. Additionally, one must grasp the [thread-local storage strategy for thread synchronization](https://en.wikipedia.org/wiki/Thread-local_storage#Usage), although how the GNU loader employs this strategy comes with a twist because it uses a predefined field in the TCB as the per-thread flag that it makes atomic modifications to and acquires a stack lock (`dl_stack_cache_lock`) when accumulating values versus typical thread-local synchronization using, for example, a `thread_local` (C++) variable which non-atmoic modifications are made to and joining relevant threads before reading the accumulated value.

## Assessing the Brokenness of `GetModuleHandle` and `GetModuleHandleEx` Functions

In the modern Windows loader, `GetModuleHandle` and `GetModuleHandleEx` are both broken functions. We will discuss why these functions are broken, the outcomes of their brokenness, how this functionality could be better implemented, draw perspective from the equivalent functionality as its implemented on the GNU loader, and recommendations for how to proceed.

Firstly, the `GetModuleHandle` function is broken because it [does not increment the reference count of a library](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulehandlea#remarks) before returning its module handle. This incorrect behavior could leaves callers of `GetModuleHandle` vulnerable to using a library that no longer exists if a concurrent `FreeLibrary`, run by an independent thread, removes that (dynamically loaded) library from the process. Microsoft released the `GetModuleHandleEx` extension function to patch over this issue of `GetModuleHandle`. For some poor reason, the refreshed `GetModuleHandleEx` function comes with a `GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT` flag which is also [never correct](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulehandleexa#remarks) to use because the Windows API does not practically support single-threaded applications.

Secondly, both the `GetModuleHandle` and `GetModuleHandleEx` functions are vulnerable to getting a handle on a module while it is still in the process of loading. These functions simply look up the requested module in a module informaton data structure (using the `ntdll!LdrpModuleBaseAddressIndex` red-black tree in a modern loader or the [`PEB_LDR_DATA.InMemoryOrderModuleList` linked list in the legacy loader](https://github.com/reactos/reactos/blob/053939e27cbf4d6475fb33b6fc16199bd944880d/dll/ntdll/ldr/ldrutils.c#L2066-L2100)), they pay no attention to if the module is partially loaded such as whether the library has had its imports resolved or if the module has undergone initialization yet.

Beyond these implementation errors, the `GetModuleHandle` family of functions are also easy to misuse when the `LoadLibrary` function ([which should actually be called the `LibraryOpen` or `OpenLibrary` function](#loadlibrary-vs-dlopen-return-type)) is typically the functionality developers are actually desiring. This fact comes because it is a rare requirement that a developer wants to test out-of-band of their module's functionality whether a given library is loaded in the process without loading that lirary if it is not already loaded.

The `GetModuleHandle` family of functions are also easy to misuse by assuming a library is loaded in the process when it may not be. Especially with delay loading, Windows changing whether a library is delay loaded or not between Windows versions could cause an application to break thus creating an application compatability problem if the `GetModuleHandle` functions are misused in this way.

For these reasons, Microsoft should deprecate the the `GetModuleHandle` functions because even with a correct implementation, with their functionality would be better served by passing a flag to `LoadLibraryEx`, as opposed to separate functions.

The GNU loader implements a form of `GetModuleHandle` within `dlopen` as the [`RTLD_NOLOAD` flag](https://man7.org/linux/man-pages/man3/dlmopen.3.html#DESCRIPTION) (this is a GNU extension, it is not defined by POSIX). However, `dlopen` with this flag [does not suffer](code/glibc/dlopen-noload/reference-count) from [either of the implementation flaws](code/glibc/dlopen-noload/initialization) that its Windows counterpart does.

[The `GetProcAddress` comes with mitigations to work around the brokenness of `GetModuleHandle`](#getprocaddress-workarounds-for-getmodulehandle-and-getmodulehandleex-functions); however, these patches do not resolve the issue if `GetProcAddress` is bypassed.

### Recommendations for Developers

Always prefer the `LoadLibrary` family of functions over the `GetModuleHandle` functions.

Never call the `GetModuleHandle` function nor `GetModuleHandleEx` with the `GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT` flag to stay safe from race conditions. Call `GetModuleHandleEx` without the `GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT` flag for its rare use case of testing whether a module exists in the process out-of-bound then using it or doing some operations while the given module is known to be loaded.

When done using a library, do not forget to balance out calls that increment a library's reference count such as `LoadLibrary` or `GetModuleHandleEx` without the `GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT` flag by calling `FreeLibrary` to decrement a library's reference count.

## `GetProcAddress` Workarounds for `GetModuleHandle` and `GetModuleHandleEx` Functions

### `GetProcAddress` with Uninitialized Module Mitigation

On the legacy and modern Windows loaders, [`GetProcAddress` holds the ability to initialize a module if it is uninitialized](code/windows/getmodulehandleex-getprocaddress-library-init). In this section, we learn why `GetProcAddress` can perform module initialization and compare how `GetProcAddress` determines if module initialization should occur across loader versions.

[The original reason `GetProcAddress` gained the ability for module initialization](https://learn.microsoft.com/en-us/archive/blogs/mgrier/the-nt-dll-loader-dll_process_attach-reentrancy-step-2-getprocaddress) was as a quick fix to workaround initialization order issues stemming from the library loader not tracking module dependencies correctly, which happened because the loader was collapsing the dependency graph into a linear linked list. However, this change also did well to fix the implementation error in `GetModuleHandle` and `GetModuleHandleEx` that allows getting a handle to a library while it has yet to undergo initialization.

#### Modern and Legacy Loader Implementations Analysis

On with the `GetProcAddress` analysis, the public `GetProcAddress` function internally  calls through to the `LdrGetProcedureAddressForCaller` (in NTDLL) on the modern loader or the [`LdrpGetProcedureAddress`](https://github.com/reactos/reactos/blob/053939e27cbf4d6475fb33b6fc16199bd944880d/dll/ntdll/ldr/ldrutils.c#L2224-L2226) function on the legacy loader.

In the legacy loader, when `GetProcAddress` internally resolves an exported symbol to an address, it checks if the loader has performed initialization on the containing module. If the module requires initialization, then `GetProcAddress` initializes the module before returning a symbol address to the caller.

In the ReactOS code for `LdrpGetProcedureAddress`, we see this happen:

```c
    /* Acquire lock unless we are initing */
    /* MY COMMENT: This refers to the loader initing not a module initing; this can be ignored */
    if (!LdrpInLdrInit) RtlEnterCriticalSection(&LdrpLoaderLock);

    ...

        /* Finally, see if we're supposed to run the init routines */
        /* MY COMMENT: ExecuteInit is a function argument that LdrGetProcedureAddress passes to LdrpGetProcedureAddress, always setting it to TRUE (the legacy loader does not expose this parameter from the exported LdrGetProcedureAddress function) */
        if ((NT_SUCCESS(Status)) && (ExecuteInit))
        {
            /*
            * It's possible a forwarded entry had us load the DLL. In that case,
            * then we will call its DllMain. Use the last loaded DLL for this.
            */
            Entry = NtCurrentPeb()->Ldr->InInitializationOrderModuleList.Blink;
            LdrEntry = CONTAINING_RECORD(Entry,
                                         LDR_DATA_TABLE_ENTRY,
                                         InInitializationOrderLinks);

            /* Make sure we didn't process it yet*/
            /* MY COMMENT:
               Legacy Loader Per-Module Flag Information
               LDRP_ENTRY_PROCESSED = Loader has added the given module to the initialization plan (InInitializationOrderModuleList list). Module initialization may have started but it is not complete.
               LDRP_PROCESS_ATTACH_CALLED = The loader has called the given module's initialization routine and it has returned.
               Evidence 1: https://github.com/reactos/reactos/blob/053939e27cbf4d6475fb33b6fc16199bd944880d/dll/ntdll/ldr/ldrinit.c#L698-L740
               Evidence 2: https://github.com/reactos/reactos/blob/053939e27cbf4d6475fb33b6fc16199bd944880d/dll/ntdll/ldr/ldrinit.c#L846-L866
               The names for these flags in the code do not correspond well with their meanings.
            */
            if (!(LdrEntry->Flags & LDRP_ENTRY_PROCESSED))
            {
                /* Call the init routine */
                _SEH2_TRY
                {
                    Status = LdrpRunInitializeRoutines(NULL);
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    /* Get the exception code */
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
            }
        }

...
```

Now, let's see how the modern Windows loader handles performing module initialization. In `LdrGetProcedureAddressForCaller`, there's an instance where module initialization may occur without the `LdrGetProcedureAddressForCaller` function itself acquiring loader lock (what follows is a cleaned up IDA decompilation):

```c
...
        ProcedureAddress = LdrpResolveProcedureAddress(
                            (unsigned int)v24,
                            (unsigned int)Current_Module_LDR_DATA_TABLE_ENTRY,
                            (unsigned int)Heap,
                            v38,
                            v30,
                            (char **)&v34
                          );
...
        // If LdrpResolveProcedureAddress succeeds
        if ( ProcedureAddress != NULL )
        {
            // Test for the searched module having a LDR_DDAG_STATE of LdrModulesReadyToInit (7)
            if ( Current_Module_LDR_DDAG_STATE == LdrModulesReadyToInit
                // Test if LdrGetProcedureAddressForCaller was called without setting the SkipInit flag
                // In the legacy/ReactOS loader, the LdrpGetProcedureAddress function has an ExecuteInit argument (fifth argument) that it sets to TRUE by default
                // In the modern loader, the LdrpGetProcedureAddress functions changes this fifth argument to a Flags parameter
                // The LdrpGetProcedureAddress function calls LdrpGetProcedureAddressForCaller while passing in zero for the Flags parameter, which indicates the default behavior of performing module initialization if necessary
                // KERNEL32!GetProcAddressStub (linked as GetProcAddress export) -> KERNELBASE!GetProcAddressForCaller -> ntdll!LdrGetProcedureAddressForCaller
                // SkipInit = 0x1
                && (Flags & SkipInit) == 0
                // Test for the current thread having the LoadOwner flag set in TEB.SameTebFlags
                // LoadOwner = 0x1000
                && NtCurrentTeb()->SameTebFlags & LoadOwner
                // Test for the current thread not holding the LdrpDllNotificationLock lock (i.e. we're not executing a DLL notification callback)
                && !RtlIsCriticalSectionLockedByThread(&LdrpDllNotificationLock) )
            {
                // Perform module initialization
                // Status is an NTSTATUS like STATUS_SUCCESS or STATUS_DLL_INIT_FAILED
                Status = LdrpInitializeGraphRecurse(Current_Module_LDR_DATA_TABLE_ENTRY->DdagNode, 0, &ReturnCode);
            }
...
        }
```

Huh? `LdrGetProcedureAddressForCaller` didn't acquire loader lock, yet it is performing module initialization! How could that be safe?

Checking for the `LoadOwner` flag in `TEB.SameTebFlags` ensures that a given thread has the necessary protection to safely perform module initialization because the loader only sets this flag on the calling thread of a `LoadLibrary` operation and unsets it once library loading is complete. The level of protection necessary for module initialization is typically of that imposed by the `ntdll!LdrpLoadCompleteEvent` + `ntdll!LdrpLoaderLock` synchronization mechanisms. However, during [loader initialization](#windows-loader-initialization-locking-requirements), Windows restricts the process to only having one load owner thread (by making new threads block on `ntdll!LdrpInitCompleteEvent`). So, the `LoadOwner` state check ensures this code behaves correctly when locking may not be done in the loader initialization edge case. As you can see, these state checks also examines other information regarding the execution context and module information whereby it could be undesirable or unsafe to proceed with module initialization.

How the modern `GetProcAddress` implements its module initialization logic only accounts for the sequential case of initializing a module when the thread calling `GetProcAddress` is also the load owner thread. The `ntdll!LdrpLoadCompleteEvent` loader [event](code/windows/event-experiment/event-experiment.c) is not a synchronization mechanism that can be recursively acquired, so it would have necessitated a different implementation. Also, the modern loader efficiently uses the time that the legacy loader would have just spent waiting for the other thread to finish loading libraries to help in that thread's mapping and snapping work if any is available (this is part of the functionality of the `ntdll!LdrpDrainWorkQueue` function). As a result, modern `GetProcAddress` implements a separate mitigation for the concurrent case.

### Concurrent `GetProcAddress` with Partially Loaded Module Mitigation

The `GetModuleHandle` family of functions can obtain a module handle to a library that the loader is still setting up on another thread in the process. To mitigate this concern, [`GetProcAddress` on the modern loader internally checks if a module is not done loading then yields to the load owner thread before proceeding](code/windows/getmodulehandleex-getprocaddress-concurrent-partial-load).

The code in `ntdll!LdrGetProcedureAddressForCaller` goes a little like this:

```c
            // If this thread owns the load and the state of the module passed into GetProcAddress indicates it is done loading then we break, skipping where we temporarily become the load owner
            if ( (NtCurrentTeb()->SameTebFlags & LoadOwner) || DdagState == LdrModulesReadyToRun )
                break;

            ...

            // Temporarily become the load owner to ensure any ongoing loader work setting up the given library on another thread is done before this thread proceeds
            LdrpDrainWorkQueue(0);
            LdrpDropLastInProgressCount();
```

### Holes in the Workarounds

Bypassing `GetProcAddress` allows for the incorrect behavior of the `GetModuleHandle` family of functions to show through. This can happen in at least two ways:

1. [Offsetting into a module](https://stackoverflow.com/a/22457769) to [use its contents directly](https://stackoverflow.com/a/3030948) instead of calling `GetProcAddress` to retrieve symbol addresses
2. Digging into the implementation such as by manually parsing the export table of a library then using it

So, if the `GetModuleHandle` functions are to be used, ensure that your code sticks to locating symbols using the `GetProcAddress` function.

## Windows Loader Initialization Locking Requirements

On Windows, loader initialization includes process initialization (e.g. critical data structures like the PEB), as well as fully loading the library dependencies of the application. Once loader initialization is complete, the loader can proceed with running the application or program.

Reading the ReactOS code for `LdrpLoadDll` (the internal NTDLL function called by `LoadLibrary`), we see this code:

```c
NTSTATUS NTAPI LdrpLoadDll(...)
{
    // MY COMMENT: Get the value of global variable LdrpInLdrInit into a local variable
    BOOLEAN InInit = LdrpInLdrInit;
...
    /* Check for init flag and acquire lock */
    /* MY COMMENT: This refers to the loader initing */
    if (!InInit) RtlEnterCriticalSection(&LdrpLoaderLock);
...
}
```

The loader won't acquire loader lock during library loading (including during module initialization by the `LdrpRunInitializeRoutines` function) when the loader is initializing (at process startup). What's up with that? It's a startup performance optimization to forgo locking during process startup.

Not acquiring loader lock here is safe because the legacy loader, like the modern loader, includes a mechanism for blocking new threads spawned into the process until loader initialization is complete. The legacy loader waits in the `LdrpInit` function using a [`ntdll!LdrpProcessInitialized` spinlock and sleeping with `ZwDelayExecution`](https://github.com/reactos/reactos/blob/053939e27cbf4d6475fb33b6fc16199bd944880d/dll/ntdll/ldr/ldrinit.c#L2603-L2617). While the loader is initializing, the `LdrpInit` function [sets `LdrpInLdrInit` to `TRUE`, initializes the process by calling `LdrpInitializeProcess`, then upon returning, `LdrpInit` sets `LdrpInLdrInit` to `FALSE`.](https://github.com/reactos/reactos/blob/053939e27cbf4d6475fb33b6fc16199bd944880d/dll/ntdll/ldr/ldrinit.c#L2625-L2651) After, the loader, will [unlock the `ntdll!LdrpProcessInitializing` spinlock](https://github.com/reactos/reactos/blob/053939e27cbf4d6475fb33b6fc16199bd944880d/dll/ntdll/ldr/ldrinit.c#L2653-L2658). Hence, during loader initialization, one can safely forgo acquiring loader lock.

The modern loader optimizes waiting by using the `ntdll!LdrpInitCompleteEvent` event object instead of sleeping for a set time. The modern loader also includes a `ntdll!LdrpProcessInitialized` spinlock. However, the loader may (in the unlikely occurrence of a remote thread spawning in early) only spin on it until event creation (`NtCreateEvent`), at which point the loader waits solely using that synchronization object. `ZwDelayExecution` is still called to slow the spin. While `ntdll!LdrInitState` is `0`, it's safe not to acquire any locks. This includes accessing shared module information data structures without acquiring `ntdll!LdrpModuleDataTableLock` lock and performing module initialization/deinitialization. `ntdll!LdrInitState` changes to `1` immediately after `LdrpInitializeProcess` calls `LdrpEnableParallelLoading` which creates the loader worker threads (`LoaderWorker` flag in `TEB.SameTebFlags`). However, these loader worker threads won't have any work yet, so it should still be safe not to acquire locks during this time. Once these loader worker threads receive work, though, the loader would have to start acquiring the `ntdll!LdrpModuleDataTableLock` lock to ensure thread safety when accessing module information data structures. Additionally, since loader worker threads are naturally limited to only performing mapping and snapping, the loader can currently still forgo acquiring any locks associated with being a load owner (`LoadOwner` flag in `TEB.SameTebFlags`) such as performing module initialization.

The `LdrpInitShimEngine` function is a good example of the loader performing a bunch of loader operations without locking. The loader may call `LdrpInitShimEngine` shortly before it calls `LdrpEnableParallelLoading` to start spawning loader worker threads (not always; it happens when running Google Chrome under WinDbg). The `LdrpInitShimEngine` function calls `LdrpLoadShimEngine`, which does a whole bunch of typically unsafe actions like module initialization (calls `LdrpInitalizeNode` directly and calls `LdrpInitializeShimDllDependencies`, which in turn calls `LdrpInitializeGraphRecurse`) without loader lock and walking the `PEB_LDR_DATA.InLoadOrderModuleList` without acquiring `ntdll!LdrpModuleDataTableLock`. Of course, all these actions are safe due to the unique circumstances of loader initialization. Note that the shim engine initialization function may still acquire the `ntdll!LdrpDllNotificationLock` lock not for thread safety but because the loader branches on its state using the `RtlIsCriticalSectionLockedByThread` function.

The modern loader explicitly checks `ntdll!LdrInitState` to optionally perform locking as an optimization in a few places. Notably, the `ntdll!RtlpxLookupFunctionTable` function opts to skip locking the `ntdll!LdrpInvertedFunctionTableSRWLock` lock before accessing the `ntdll!LdrpInvertedFunctionTable` shared data structure if `ntdll!LdrInitState` equals `3` (i.e. just before "loader initialization is done"). Similarly, the `ntdll!LdrLockLoaderLock` function only acquires loader lock if loader initialization is done.

Be aware that for both the legacy and modern loaders, this improvement in startup performance comes with a trade-off in run-time performance. Since, after loader initialization is complete those branches on `ntdll!LdrpInLdrInit` or `ntdll!LdrInitState` become nothing but dead weight.

Beyond a slight startup performance improvement through reduced synchronization overhead, there is another reason Microsoft may want to disallow new (potential load owner) threads from running during process statup, particularly in regard to module initializers: Windows places loader lock (or the modern equivalents) at the bottom of any lock hierarchy that is external to the loader. Thus, restricting additional threads from running in this stage works as a quick fix to mitigate ABBA deadlock risk from module initializers at process startup. Of course, this risk reduction does not extend to libraries that are dynamically loaded at process run-time.

The GNU loader doesn't implement any such startup performance hack to forgo locking on process startup. The absence of any such mechanism by the GNU loader enables threads to start and exit at process startup or within a module initializer. The same is true for process exit. Therefore, the GNU loader is more flexible in this respect.

## Loader Enclaves

An enclave is a security feature that isolates a region of data or code within an application's address space. Enclaves utilize one of three backing technologies to provide this security feature: Intel Software Guard Extensions (SGX), AMD Secure Encrypted Virtualization (SEV), or Virtualization-based Security (VBS). The Intel and AMD solutions are memory encryptors; they safeguard sensitive memory by encrypting it at the hardware level. VBS securely isolates sensitive memory by containing it in [virtual secure mode](https://techcommunity.microsoft.com/t5/virtualization/virtualization-based-security-enabled-by-default/ba-p/890167) where even the NT kernel cannot access it.

Within SGX, there is [SGX1 and SGX2](https://caslab.csl.yale.edu/workshops/hasp2016/HASP16-16_slides.pdf). SGX1 only allows using statically allocated memory with a set size before enclave initialization. SGX2 adds support for an enclave to allocate memory dynamically. Due to this limitation, putting a library into an enclave on [SGX1 requires that the library be statically linked](https://download.01.org/intel-sgx/latest/linux-latest/docs/Intel_SGX_Developer_Guide.pdf). On the other hand, [SGX2 supports dynamically loading/linking libraries](https://www.intel.com/content/dam/develop/public/us/en/documents/Dynamic-Loading-to-Build-Intel-SGX-Applications-in-Linux.docx).

A Windows VBS-based enclave requires Microsoft's signature as the root in the chain of trust or as a countersignature on a third party's certificate of an Authenticode-signed DLL. The signature must contain a specific extended key usage (EKU) value that permits running as an enclave (thanks to the *Windows Internals: System architecture, processes, threads, memory management, and more, Part 1 (7th edition)* book for this tidbit on VBS signing requirements). Enabling test signing on your system can get an unsigned enclave running. VBS enclaves may call [enclave-compatible versions of Windows API functions](https://learn.microsoft.com/en-us/windows/win32/trusted-execution/available-in-enclaves) from the enclave version of the same library.

Both Windows and Linux support loading libraries into enclaves. An enclave library requires special preparation; a programmer cannot load any generic library into an enclave.

Windows integrates enclaves as part of the native loader. One may [`CreateEnclave`](https://learn.microsoft.com/en-us/windows/win32/api/enclaveapi/nf-enclaveapi-createenclave) to make an enclave and then call [LoadEnclaveImage](https://learn.microsoft.com/en-us/windows/win32/api/enclaveapi/nf-enclaveapi-loadenclaveimagew) to load a library into an enclave. Internally, `CreateEnclave` (public API) calls `ntdll!LdrCreateEnclave`, which then calls `ntdll!NtCreateEnclave`; this function only does the system call to create an enclave, and then calls `LdrpCreateSoftwareEnclave` to initialize and link the new enclave entry into the `ntdll!LdrpEnclaveList` list. `ntdll!LdrpEnclaveList` is the list head, its list entries of type [`LDR_SOFTWARE_ENCLAVE`](https://github.com/winsiderss/systeminformer/blob/fcbd70d5bb9908ebf50eb4de7cca620549e532c4/phnt/include/ntldr.h#L1109) are allocated onto the heap and linked into the list. Compiling an enclave library requires special compilation steps (e.g. [compiling for Intel SGX](https://www.intel.com/content/www/us/en/developer/articles/guide/getting-started-with-sgx-sdk-for-windows.html#inpage-nav-3-undefined)). The Windows loader only supports Intel SGX enclaves (for Intel CPUs), it does not support AMD SEGV enclaves (for AMD CPUs).

The GNU loader has no knowledge of enclaves. Intel provides the [Intel SGX SDK and the Intel SGX Platform Software (PSW)](https://github.com/intel/linux-sgx) necessary for using SGX on Linux. The [SGX driver](https://github.com/intel/linux-sgx-driver) awaits upstreaming into the Linux source tree. Linux currently has no equivalent to Windows Virtualization Based Security. Here is some [sample code for loading an SGX module](https://github.com/intel/linux-sgx/tree/master/SampleCode/SampleCommonLoader). However, [Hypervisor-Enforced Kernel Integrity (Heki)](https://github.com/heki-linux) is on its way, including patches to the kernel. Developers at Microsoft are introducing this new feature into Linux.

If you are interested in the CPU-based enclave technologies themselves, here is a [comparison of Intel SGX and AMD SEV](https://caslab.csl.yale.edu/workshops/hasp2018/HASP18_a9-mofrad_slides.pdf).

[Open Enclave](https://github.com/openenclave/openenclave) is a cross-platform and hardware-agnostic open source library for utilizing enclaves. Here is some [sample code](https://github.com/openenclave/openenclave/tree/master/samples/helloworld) calling into an enclave library.

**Side Note Regarding the Windows `LdrpObtainLockedEnclave` Function:**

The Windows loader uses the `ntdll!LdrpObtainLockedEnclave` function to obtain the enclave lock for a module, doing per-node locking. The `LdrpObtainLockedEnclave` function acquires the `ntdll!LdrpEnclaveListLock` lock and then searches from the `ntdll!LdrpEnclaveList` list head to find an enclave for the DLL image base address this function receives as its first argument. If `LdrpObtainLockedEnclave` finds a matching enclave, it atomically increments a reference count and enters a critical section stored in the enclave structure before returning that enclave's address to the caller. Typically (unless your process uses enclaves), the list at `ntdll!LdrpEnclaveList` will be empty, effectively making `LdrpObtainLockedEnclave` a no-op.

The `LdrpObtainLockedEnclave` function is called every time `GetProcAddress` ➜ `LdrGetProcedureAddressForCaller` runs (bloat alert), so it was worth giving a look.

## Investigating the COM Server Deadlock from `DllMain`

Trying to connect to a COM server under loader lock fails deterministically. For instance, running this code from `DllMain` on `DLL_PROCESS_ATTACH` will deadlock:

```cpp
// Ensure valid LNK file with this CMD command:
// explorer "C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Accessories\Notepad.lnk"
LPCSTR linkFilePath = "C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\Accessories\\Notepad.lnk";
WCHAR resolvedPath[MAX_PATH];

HRESULT hres;
HWND hwnd = GetDesktopWindow();

hres = CoInitializeEx(NULL, 0);

if (SUCCEEDED(hres)) {
    // Resolve LNK file to its target file
    // Implementation: https://learn.microsoft.com/en-us/windows/win32/shell/links#resolving-a-shortcut
    ResolveIt(hwnd, linkFilePath, resolvedPath, MAX_PATH);
}

CoUninitialize();

// Output: C:\Windows\system32\notepad.exe
wprintf(L"%ls\r\n", resolvedPath);
```

Here we see the deadlocked call stack, which shows that <code>NtAlpcSend<strong>Wait</strong>ReceivePort</code> is waiting for something (this function only exists to make the `NtAlpcSendWaitReceivePort` [system call](https://j00ru.vexillium.org/syscalls/nt/64/)):

```
0:000> k
 # Child-SP          RetAddr               Call Site
00 00000080`f96fcde8 00007ffd`26c93f8f     ntdll!NtAlpcSendWaitReceivePort+0x14
01 00000080`f96fcdf0 00007ffd`26ca94d7     RPCRT4!LRPC_BASE_CCALL::SendReceive+0x12f
02 00000080`f96fcec0 00007ffd`26c517c0     RPCRT4!NdrpSendReceive+0x97
03 00000080`f96fcef0 00007ffd`26c524bf     RPCRT4!NdrpClientCall2+0x5d0
04 00000080`f96fd510 00007ffd`28491ce5     RPCRT4!NdrClientCall2+0x1f
05 (Inline Function) --------`--------     combase!ServerAllocateOXIDAndOIDs+0x73 [onecore\com\combase\idl\internal\daytona\objfre\amd64\lclor_c.c @ 313]
06 00000080`f96fd540 00007ffd`28491acd     combase!CRpcResolver::ServerRegisterOXID+0xd5 [onecore\com\combase\dcomrem\resolver.cxx @ 1056]
07 00000080`f96fd600 00007ffd`28494531     combase!OXIDEntry::RegisterOXIDAndOIDs+0x71 [onecore\com\combase\dcomrem\ipidtbl.cxx @ 1642]
08 (Inline Function) --------`--------     combase!OXIDEntry::AllocOIDs+0xc2 [onecore\com\combase\dcomrem\ipidtbl.cxx @ 1696]
09 00000080`f96fd710 00007ffd`2849438f     combase!CComApartment::CallTheResolver+0x14d [onecore\com\combase\dcomrem\aprtmnt.cxx @ 693]
0a 00000080`f96fd8c0 00007ffd`284abc2f     combase!CComApartment::InitRemoting+0x25b [onecore\com\combase\dcomrem\aprtmnt.cxx @ 991]
0b (Inline Function) --------`--------     combase!CComApartment::StartServer+0x52 [onecore\com\combase\dcomrem\aprtmnt.cxx @ 1214]
0c 00000080`f96fd930 00007ffd`2849c285     combase!InitChannelIfNecessary+0xbf [onecore\com\combase\dcomrem\channelb.cxx @ 1028]
0d 00000080`f96fd960 00007ffd`2849a644     combase!CGIPTable::RegisterInterfaceInGlobalHlp+0x61 [onecore\com\combase\dcomrem\giptbl.cxx @ 815]
0e 00000080`f96fda10 00007ffd`21b86399     combase!CGIPTable::RegisterInterfaceInGlobal+0x14 [onecore\com\combase\dcomrem\giptbl.cxx @ 776]
0f 00000080`f96fda50 00007ffd`21b5adb3     PROPSYS!CApartmentLocalObject::_RegisterInterfaceInGIT+0x81
10 00000080`f96fda90 00007ffd`21b842e6     PROPSYS!CApartmentLocalObject::_SetApartmentObject+0x7b
11 00000080`f96fdac0 00007ffd`21b5c1fc     PROPSYS!CApartmentLocalObject::TrySetApartmentObject+0x4e
12 00000080`f96fdaf0 00007ffd`21b5bde6     PROPSYS!CreateObjectWithCachedFactory+0x2bc
13 00000080`f96fdbd0 00007ffd`21b5d16c     PROPSYS!CreateMultiplexPropertyStore+0x46
14 00000080`f96fdc30 00007ffd`241d3235     PROPSYS!PSCreateItemStoresFromDelegate+0xbfc
15 00000080`f96fde90 00007ffd`2422892f     windows_storage!CShellItem::_GetPropertyStoreWorker+0x2d5
16 00000080`f96fe3d0 00007ffd`2422b7e7     windows_storage!CShellItem::GetPropertyStoreForKeys+0x14f
17 00000080`f96fe6a0 00007ffd`2415f2b6     windows_storage!CShellItem::GetCLSID+0x67
18 00000080`f96fe760 00007ffd`2415eb0b     windows_storage!GetParentNamespaceCLSID+0xde
19 00000080`f96fe7c0 00007ffd`241772fb     windows_storage!CShellLink::_LoadFromStream+0x2d3
1a 00000080`f96feaf0 00007ffd`2417709c     windows_storage!CShellLink::LoadFromPathHelper+0x97
1b 00000080`f96feb40 00007ffd`24177039     windows_storage!CShellLink::_LoadFromFile+0x48
1c 00000080`f96febd0 00007ffd`21aa10e2     windows_storage!CShellLink::Load+0x29
1d (Inline Function) --------`--------     TestDLL!ResolveIt+0x8c [C:\Users\user\source\repos\TestDLL\TestDLL\dllmain.cpp @ 110]
1e 00000080`f96fec00 00007ffd`21aa143b     TestDLL!DllMain+0xd2 [C:\Users\user\source\repos\TestDLL\TestDLL\dllmain.cpp @ 170]
1f 00000080`f96ff4f0 00007ffd`28929a1d     TestDLL!dllmain_dispatch+0x8f [d:\a01\_work\20\s\src\vctools\crt\vcstartup\src\startup\dll_dllmain.cpp @ 281]
20 00000080`f96ff550 00007ffd`2897c2c7     ntdll!LdrpCallInitRoutine+0x61
21 00000080`f96ff5c0 00007ffd`2897c05a     ntdll!LdrpInitializeNode+0x1d3
22 00000080`f96ff710 00007ffd`2894d947     ntdll!LdrpInitializeGraphRecurse+0x42
23 00000080`f96ff750 00007ffd`2892fbae     ntdll!LdrpPrepareModuleForExecution+0xbf
24 00000080`f96ff790 00007ffd`289273e4     ntdll!LdrpLoadDllInternal+0x19a
25 00000080`f96ff810 00007ffd`28926af4     ntdll!LdrpLoadDll+0xa8
26 00000080`f96ff9c0 00007ffd`260156b2     ntdll!LdrLoadDll+0xe4
27 00000080`f96ffab0 00007ff7`8fda1022     KERNELBASE!LoadLibraryExW+0x162
28 00000080`f96ffb20 00007ff7`8fda1260     TestProject!main+0x12 [C:\Users\user\source\repos\TestProject\TestProject\source.c @ 82]
29 (Inline Function) --------`--------     TestProject!invoke_main+0x22 [d:\a01\_work\20\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl @ 78]
2a 00000080`f96ffb50 00007ffd`26f37344     TestProject!__scrt_common_main_seh+0x10c [d:\a01\_work\20\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl @ 288]
2b 00000080`f96ffb90 00007ffd`289626b1     KERNEL32!BaseThreadInitThunk+0x14
2c 00000080`f96ffbc0 00000000`00000000     ntdll!RtlUserThreadStart+0x21
```

Running the same code but swapping the `CoCreateInstance` execution context from `CLSCTX_INPROC_SERVER` (an in-process DLL server, the most common execution context) to any other execution context such as [`CLSCTX_LOCAL_SERVER`](https://learn.microsoft.com/en-us/windows/win32/api/wtypesbase/ne-wtypesbase-clsctx#constants) (an out-of-process EXE server on the same machine) yields a similar deadlock (the `CShellLink` COM component corresponding to the `CLSID_ShellLink` identifier is not set up to support the latter execution context but that is besides the point):

```
0:000> k
 # Child-SP          RetAddr               Call Site
00 000000ea`aed1c908 00007ff8`d3b41b4f     ntdll!NtAlpcSendWaitReceivePort+0x14
01 000000ea`aed1c910 00007ff8`d3b5c357     RPCRT4!LRPC_BASE_CCALL::SendReceive+0x12f
02 000000ea`aed1c9e0 00007ff8`d3b01610     RPCRT4!NdrpSendReceive+0x97
03 000000ea`aed1ca10 00007ff8`d3b0102f     RPCRT4!NdrpClientCall2+0x5d0
04 000000ea`aed1d030 00007ff8`d379d801     RPCRT4!NdrClientCall2+0x1f
05 (Inline Function) --------`--------     combase!ServerAllocateOXIDAndOIDs+0x73 [onecore\com\combase\idl\internal\daytona\objfre\amd64\lclor_c.c @ 313]
06 000000ea`aed1d060 00007ff8`d379d67d     combase!CRpcResolver::ServerRegisterOXID+0xd5 [onecore\com\combase\dcomrem\resolver.cxx @ 1056]
07 000000ea`aed1d120 00007ff8`d379ded1     combase!OXIDEntry::RegisterOXIDAndOIDs+0x71 [onecore\com\combase\dcomrem\ipidtbl.cxx @ 1642]
08 (Inline Function) --------`--------     combase!OXIDEntry::AllocOIDs+0xc2 [onecore\com\combase\dcomrem\ipidtbl.cxx @ 1696]
09 000000ea`aed1d230 00007ff8`d374a103     combase!CComApartment::CallTheResolver+0x14d [onecore\com\combase\dcomrem\aprtmnt.cxx @ 693]
0a 000000ea`aed1d3e0 00007ff8`d37476fe     combase!CComApartment::InitRemoting+0x25b [onecore\com\combase\dcomrem\aprtmnt.cxx @ 991]
0b 000000ea`aed1d450 00007ff8`d3717d87     combase!CComApartment::StartServer+0x2a [onecore\com\combase\dcomrem\aprtmnt.cxx @ 1214]
0c (Inline Function) --------`--------     combase!InitChannelIfNecessary+0x1f [onecore\com\combase\dcomrem\channelb.cxx @ 1028]
0d 000000ea`aed1d480 00007ff8`d3717708     combase!CRpcResolver::BindToSCMProxy+0x2b [onecore\com\combase\dcomrem\resolver.cxx @ 1733]
0e 000000ea`aed1d4c0 00007ff8`d37c6d66     combase!CRpcResolver::DelegateActivationToSCM+0x12c [onecore\com\combase\dcomrem\resolver.cxx @ 2243]
0f 000000ea`aed1d690 00007ff8`d3717315     combase!CRpcResolver::CreateInstance+0x1a [onecore\com\combase\dcomrem\resolver.cxx @ 2507]
10 000000ea`aed1d6c0 00007ff8`d372cb30     combase!CClientContextActivator::CreateInstance+0x135 [onecore\com\combase\objact\actvator.cxx @ 616]
11 000000ea`aed1d970 00007ff8`d372581a     combase!ActivationPropertiesIn::DelegateCreateInstance+0x90 [onecore\com\combase\actprops\actprops.cxx @ 1983]
12 000000ea`aed1da00 00007ff8`d37242c0     combase!ICoCreateInstanceEx+0x90a [onecore\com\combase\objact\objact.cxx @ 2032]
13 000000ea`aed1e8d0 00007ff8`d372401c     combase!CComActivator::DoCreateInstance+0x240 [onecore\com\combase\objact\immact.hxx @ 392]
14 (Inline Function) --------`--------     combase!CoCreateInstanceEx+0xd1 [onecore\com\combase\objact\actapi.cxx @ 177]
15 000000ea`aed1ea30 00007ff8`c41210a7     combase!CoCreateInstance+0x10c [onecore\com\combase\objact\actapi.cxx @ 121]
16 (Inline Function) --------`--------     TestDLL!ResolveIt+0x24 [C:\Users\user\source\repos\TestDLL\TestDLL\dllmain.cpp @ 42]
17 000000ea`aed1ead0 00007ff8`c412145b     TestDLL!DllMain+0x77 [C:\Users\user\source\repos\TestDLL\TestDLL\dllmain.cpp @ 304]
18 000000ea`aed1f3c0 00007ff8`d4209a1d     TestDLL!dllmain_dispatch+0x8f [d:\a01\_work\20\s\src\vctools\crt\vcstartup\src\startup\dll_dllmain.cpp @ 281]
19 000000ea`aed1f420 00007ff8`d425d307     ntdll!LdrpCallInitRoutine+0x61
1a 000000ea`aed1f490 00007ff8`d425d09a     ntdll!LdrpInitializeNode+0x1d3
1b 000000ea`aed1f5e0 00007ff8`d422d947     ntdll!LdrpInitializeGraphRecurse+0x42
1c 000000ea`aed1f620 00007ff8`d420fbae     ntdll!LdrpPrepareModuleForExecution+0xbf
1d 000000ea`aed1f660 00007ff8`d42073e4     ntdll!LdrpLoadDllInternal+0x19a
1e 000000ea`aed1f6e0 00007ff8`d4206af4     ntdll!LdrpLoadDll+0xa8
1f 000000ea`aed1f890 00007ff8`d1b32612     ntdll!LdrLoadDll+0xe4
20 000000ea`aed1f980 00007ff6`ff831012     KERNELBASE!LoadLibraryExW+0x162
21 000000ea`aed1f9f0 00007ff6`ff831240     TestProject!main+0x12 [C:\Users\user\source\repos\TestProject\TestProject\source.c @ 175]
22 (Inline Function) --------`--------     TestProject!invoke_main+0x22 [d:\a01\_work\20\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl @ 78]
23 000000ea`aed1fa20 00007ff8`d2ab7374     TestProject!__scrt_common_main_seh+0x10c [d:\a01\_work\20\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl @ 288]
24 000000ea`aed1fa60 00007ff8`d423cc91     KERNEL32!BaseThreadInitThunk+0x14
25 000000ea`aed1fa90 00000000`00000000     ntdll!RtlUserThreadStart+0x21
```

Once again, the `NtAlpcSendWaitReceivePort` function is indeed waiting for something, thus the reason for our deadlock in both execution contexts.

In the in-process server COM object creation call stack, the deadlock occurs on the first call to a method on the COM object (`hres = ppf->Load(wsz, STGM_READ)`). In the local server COM object creation call stack, the deadlock occurs within the `CoCreateInstance` function. The deadlock occurs later for the in-process execution context because an in-process COM server's startup is lazy.

Walking through this call stack a bit, starting a COM server (`combase!CComApartment::StartServer`) involves calling [`NdrClientCall2`](https://learn.microsoft.com/en-us/windows/win32/api/rpcndr/nf-rpcndr-ndrclientcall2) to perform a local procedure call. Here, we see the `combase!ServerAllocateOXIDAndOID` function is making a local procedure call according to the [`lclor` interface](https://github.com/ionescu007/hazmat5/blob/main/lclor.idl). The Local OXID Resolver (LCLOR) makes calls to the Remote Procedure Call Service (RPCSS), which Windows describes in its service description field:

> The RPCSS service is the Service Control Manager for COM and DCOM servers. It performs object activations requests, object exporter resolutions and distributed garbage collection for COM and DCOM servers. If this service is stopped or disabled, programs using COM or DCOM will not function properly. It is strongly recommended that you have the RPCSS service running.

Even though we are not using DCOM (our COM object exists locally on the same machine), DCOM comes into play because Microsoft has integrated DCOM into COM. This integration is part of what allows COM to support [location transparency](https://en.wikipedia.org/wiki/Location_transparency) across machines. The specific verbiage of "remoting" (`combase!CComApartment::InitRemoting` in the call stack) likely comes from [.NET Remoting](https://en.wikipedia.org/wiki/.NET_Remoting), .NET being the spiritual successor to COM.

Regarding the deadlock, the call stack looks similar to what happens when running [`ShellExecute` under `DllMain`](https://elliotonsecurity.com/perfect-dll-hijacking/shellexecute-initial-deadlock-point-stack-trace.png). As we know from "Perfect DLL Hijacking", loader lock (`ntdll!LdrpLoaderLock`) is the root cause of this deadlock and releasing this lock allows execution to continue (also, see the `DEBUG NOTICE` in the LdrLockLiberator project for further potential blockers). However, setting a read watchpoint on loader lock reveals that the user-mode code within our process never checks the state of the loader lock. This finding leads me to believe that the local procedure call by `ntdll!NtAlpcSendWaitReceivePort` causes a remote process or the kernel to introspect on the state of loader lock within our process, thus explaining why WinDbg never hits a user-mode watchpoint placed on loader lock yet appears to be affected by its locked or unlocked state. This introspection is likely done using [shared memory](https://en.wikipedia.org/wiki/Shared_memory) (i.e. `NtMapViewOfSection` targeting a mapping in a different process on Windows or `shm_open` on POSIX-compliant systems).

[Microsoft explains this deadlock](https://learn.microsoft.com/en-us/cpp/dotnet/initialization-of-mixed-assemblies) (emphasis mine):

> To do it, the Windows loader uses a process-global critical section (often called the "loader lock") that prevents unsafe access during module initialization.

> The CLR will attempt to automatically load that assembly, which may require the Windows loader to **block** on the loader lock. A deadlock occurs since the loader lock is already held by code earlier in the call sequence.

Note that Microsoft wrote this documentation to resolve Windows issues surrounding loader lock that high-level developers may come across with the CLR (the .NET runtime environment); however, it also [roughly appears to be applicable to COM](https://stackoverflow.com/a/35517052). As we know, Microsoft uniquely places the loader at the bottom any lock hierarchy in the system that lives outside of NTDLL, which is already reason enough Microsoft would choose to turn a probabilistic ABBA deadlock into a deterministic blocker. Particularly the sentence "A deadlock occurs since the loader lock is already held by code earlier in the call sequence." refers to the lock hierarchy nesting that can lead to ABBA deadlock when combined with how Windows defines lock ordering. Additionally, I reason that initializing a COM object, like a CLR assembly, may take "actions that are invalid under loader lock" such as spawning and waiting on a thread, which causes a deadlock on Windows. The `combase!CRpcThreadCache::RpcWorkerThreadEntry` thread that spawns in the case of both execution contexts stands out. So, instead of allowing for non-determinism that could cause an unlikely or hard to diagnose deadlock (or possibly crash due to an immediate circular dependency with an uninitialized libary) at an arbitrary point in time, Microsoft took steps to make connecting to a COM server from `DllMain`, which is [in effect using COM](#component-model-technology-overview), deadlock deterministically.

Emphasis on the word "block" because it indicates that, in this case, Windows treats loader lock as a [readers-writer lock](https://en.wikipedia.org/wiki/Readers%E2%80%93writer_lock) (SRW lock in Windows) that's been acquired in exclusive/write mode instead of a critical section (a thread synchronization mechanism), the latter of which allows recursive acquisition. Reacquiring a lock on the same thread requires the surrounding code to have a [reentrant design](https://en.wikipedia.org/wiki/Reentrancy_(computing)). Nesting the acquisition of different locks, from nested subsystems in this case, requires that they agree on lock hierachy. These facts align with what we see when starting a COM server from the `DLL_PROCESS_ATTACH` of `DllMain` and our diagnosis.

## GNU Loader Lock Hierarchy and Synchronization Strategy

`dl_load_lock` is the general lock that protects the loader from concurrent access.

`dl_load_write_lock` is, as the name implies, a *write* lock that protects against writing to the list of link maps (i.e. adding/removing a node to/from the list).

`dl_load_lock` is the highest lock in the loader's lock hierarchy, with its position being above both `dl_load_write_lock` and [GSCOPE locking](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/dl-lookup.c#L582).

There are occurrences where the GNU loader walks the list of link maps (a *read* operation) while only holding the `dl_load_lock` lock. For instance, walking the link map list while only holding `dl_load_lock` but not `dl_load_write_lock` occurs in [`_dl_sym_find_caller_link_map`](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/dl-sym-post.h#L22) ➜ `_dl_find_dso_for_object`. This action is safe because the [only occurrence where the loader modifies the list of link maps](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/dl-load.c#L1030) (following early loader startup) is when it loads/unloads a library, which means acquiring `dl_load_lock`. Linking a new link map into the list of link maps (a *write* operation) requires acquiring `dl_load_lock` then `dl_load_write_lock` (e.g. [see the GDB log from the `load-library` experiment](code/glibc/dlopen)). It's unsafe to modify the link map list without also acquiring `dl_load_write_lock` because, for instance, the [`dl_iterate_phdr`](https://man7.org/linux/man-pages/man3/dl_iterate_phdr.3.html) function [acquires `dl_load_write_lock` *without* first acquiring `dl_load_lock`](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/dl-iteratephdr.c#L39) to ensure the list of link maps remains consistent while the function walks between nodes (a *read* operation).

Note that the `dl_iterate_phdr` function is a GNU/BSD extension, it's not POSIX. Also, since `dl_iterate_phdr` calls your callback while holding `dl_load_write_lock`, lock hierarchy layout makes it unsafe to load a library from within this callback. It makes sense to keep `dl_load_write_lock` held while running these callbacks so they can obtain a consistent snapshot of information on the load libaries at the given point in time. There is a trade-off here between maximizing concurrency from not acquiring the broad `dl_load_lock` while calling these callbacks and the library loading/unloading limitation within these callbacks. Although, I deem this trade-off acceptable since a program should only want to use these callbacks to search the libraries and find information about a given loaded library. A program could call `dlopen` with the `RTLD_NOLOAD` flag on the library name it wants to find loaded before iterating libraries so as to increase that library's reference count thus allowing its safe use outside of the callback after finding it in memory. Alternatively, if one wants to use `dl_iterate_phdr` to gather information from the [ELF program headers](https://refspecs.linuxbase.org/elf/gabi4+/ch5.pheader.html) (`phdr`, which is this function's intended use) of all loaded libraries then that can be done in its entirety within the callback safely without loading a library or having to worry about library reference counts due to holding `dl_load_write_lock` thus making sure the library is not unloaded by a concurrent thread.

Interestingly, `dl_load_write_lock` is a standard POSIX mutex instead of a POSIX [readers-writer](https://en.wikipedia.org/wiki/Readers%E2%80%93writer_lock) or `rwlock` (acquired with the `pthread_rwlock_rdlock` and `pthread_rwlock_wrlock` locking functions). While a readers-writer lock would extend concurrency to multiple threads iterating the link map list with `dl_iterate_phdr` at once (these being *read* operations), that ability may be unwanted if its expected that libraries should be able to safely modify the iterated ELF program headers (a *write* operation) in their callbacks (this action would require changing memory protection).

## Thread Creation is Not Thread-Safe on Windows: A Concurrency Bug in the Windows Loader

In the `ntdll!LdrpInitializeThread` function, the loader acquires the load/loader lock (`ntdll!LdrpLoadCompleteEvent` + `ntdll!LdrpLoaderLock`) then iterates over modules in the `PEB_LDR_DATA.InLoadOrderModuleList` list to run `DLL_THREAD_ATTACH` routines. This function does not acquire `ntdll!LdrpModuleDataTableLock` to walk the load order list and somewhat peculiarly decides to walk it over the seemingly more fitting `PEB_LDR_DATA.InInitializationOrderModuleList` (i.e. DLLs that have had their `DLL_PROCESS_ATTACH` routine called).

By taking these actions, the loader's approach to thread safety assumes that load/loader lock protects against *write* operations to `PEB_LDR_DATA.InLoadOrderModuleList`. However, this assumption is wrong. Setting a breakpoint on `ntdll!LdrpInsertDataTableEntry` reveals that there are instances where a data table entry will be inserted, thereby writing a new entry into the `PEB_LDR_DATA.InLoadOrderModuleList`, *without* acquiring load/loader lock. During `ntdll!LdrpInsertDataTableEntry`, the `ntdll!LdrpModuleDataTableLock` lock will be exclusively acquired. However, this does not matter because `ntdll!LdrpInitializeThread` does not also acquire `ntdll!LdrpModuleDataTableLock` on its side.

**That's right, we have a thread-safety bug in the Windows loader!**

I was able to confirm the lack of safety through test runs in WinDbg and after searching around for instances where thread-unsafe use of `PEB_LDR_DATA.InLoadOrderModuleList` was causing a crash in the wild—[here it is](https://stackoverflow.com/a/63353814)!

## Library Lazy Loading and Lazy Linking Overview

In a [loader or dynamic linker](#defining-loader-and-linker-terminology), loading refers to the entire process of setting up a module from mapping it into memory, to linking, to initialization. Making up one part of the loading process, linking, also referred to as binding or snapping, resolves symbols names to addresses in memory.  Doing either of these operations lazily means that it is done on an as-needed basis instead of all at process startup or library load-time.

Windows collectively refers to both lazy loading and lazy linking as "delay loading". However, we use the distinguished terms throughout this section.

### Lazy Loading

Lazy loading is when a dynamically linked library loads the first time an importing module calls into it as opposed to when the importing module loads. This behavior is achieved using some glue code inserted in the importing module's API dependency table where each instance into a lazy library will, on the first call, dynamically load the necessary library before calling an API from it.

Windows is the only platform to natively support lazy loading. On Windows, lazy loading is a highly effective optimization because the operating system's library infrastructure is disorganized thus making it common to use a library in the Windows API without ever requiring all of that library's dependencies. Windows architecture also favors larger processes with more threads leading to, on average, more libraries per-process thus increasing the optimization's utility.

Regaring Unix-like systems, MacOS previously had native lazy loading support in Xcode until Apple removed this feature (see the [`ld-classic`](https://keith.github.io/xcode-man-pages/ld-classic.1.html#:~:text=%2Dlazy_library) and [`ld`](https://keith.github.io/xcode-man-pages/ld.1.html#:~:text=%2Dlazy_library) manuals). For all other Unix systems, lazy loading can effectively be achieved by manually calling `dlopen` and then `dlsym` at run-time. These operations could be abstracted away using a [proxy design pattern](https://stackoverflow.com/a/23405008) so manually calling dynamic linker functions is not necessary. A project called [Implib.so](https://github.com/yugr/Implib.so) includes seamless library lazy loading as part of its feature set, with notable caveats in terms of what it supports.

### Lazy Linking

Lazy linking is when the API linkage that connects an importing module to its dependencies is resolved on the first call to that API as opposed to when the importing module loads. This behavior is achieved using some glue code inserted in the importing module's API dependency table where each instance into another library will, on the first call, resolve the API name to its address in memory before calling the API.

Unix and Windows platforms both natively support lazy linking, with POSIX standardizing this feature to exist in the former case. Unix systems can achieve lazy linking simply by calling [`dlopen` with the `RTLD_LAZY` flag](https://pubs.opengroup.org/onlinepubs/009696799/functions/dlopen.html) or providing the [`-z lazy` flag](https://man7.org/linux/man-pages/man1/ld.1.html#:~:text=lazy) to the linker. On Windows, lazy linking is only supported as part of lazy loading (hence Microsoft collectively refers to it as "delay loading"). There is no way to make imported functions from a DLL link lazily without having the entire DLL load lazily. Windows also does not readily expose a lazy linking option through `LoadLibrary` like POSIX-compliant systems do with `dlopen`.

While [GNU enables glibc lazy linking by default](https://man7.org/linux/man-pages/man1/ld.1.html#:~:text=Lazy%20binding%20is%20the%0A%20%20%20%20%20%20%20%20%20%20%20%20%20%20%20default.), downstream [GNU/Linux distributions commonly default to using an exploit mitigation known as full RELRO](https://www.redhat.com/en/blog/hardening-elf-binaries-using-relocation-read-only-relro), which has the effect of disabling lazy linking to guard against the exploitation of a memory corruption vulnerability. In my testing, GCC on these distributions still typically compiles with lazy linking by default. Lazy linking on Windows poses the same potential security risk. Although, the risk is unavoidable on Windows since it does not have an option to entirely disable lazy linking. Unlike Unix lazy linking (when it is enabled), Microsoft opts to take the performance hit of changing memory protection to writable then back (two system calls) upon resolving each lazy linked function or delay loaded API, which [leaves only a small time window](data/windows/delay-load-trace.log#L1999-L2002) for an attacker to hypothetically modify a writable code pointer to their own liking (all confirmed by my own analysis). Although lazy linking is often disabled on modern Unix-like operating systems for security, its minimal process architecture means that there will be less symbols for a dynamic linker to resolve at process startup, anyway. In cases where a given process uses the majority of symbols it imports, immediate linking will also lead to faster overall execution time than lazy linking because the former approach makes more efficient use of CPU caches.

### Windows Delay Loading

Delay loading, including lazy loading and lazy linking, [was added to Visual C++ 6.0](https://web.archive.org/web/20080914202003/http://msdn.microsoft.com/en-us/library/aa729389(VS.71).aspx) ([released 1998](https://winworldpc.com/product/visual-c/6x)) as the public `DELAYIMP.H` header and the compiled `DELAYIMP.LIB` file for linking against. Delayed DLL loading was and still is done by the [`/DELAYLOAD` linker option](https://github.com/reactos/reactos/blob/1ea3af8959da6fcf34d3eb92885fe01ce18de83c/sdk/cmake/msvc.cmake#L302-L317), which under the hood uses `LoadLibrary`/`GetProcAddress` to implement the functionality. From the start with Visual C++ 6.0,  delay loading supported caching of library handle and symbol addresses (for evidence of this fact, see `DELAYHLP.CPP`, where the `// Store the library handle.` comment and `*ppfnIATEntry = pfnRet;` lines of code are).

Windows 2000 was the first of Microsoft's operating systems to be aware of delay loading. [This system newly added the definition for `IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT` to the `winnt.h` file of its corresponding software development kit (SDK) and Windows driver kit (WDK)](data/windows/legacy-software-analysis.md#operating-system-sdks) (delay loading is a feature of the linker, but its implementation added a new PE section type and the Windows header files record these identifiers). [Windows 2000 was also the first operating system to use delay loading](data/windows/legacy-software-analysis.md#operating-systems) in its [system components](data/windows/dll-deps-research/windows-2000-system32-modules-with-delay-load.txt).

[In Windows 8](https://learn.microsoft.com/en-us/windows/win32/devnotes/resolvedelayloadedapi#requirements), delay loading was integrated into the Windows loader or NTDLL shared library, presumably to save resources so libraries can reuse the same delay loading code across libraries, improve perfomance by directly calling some lower-level functions in NTDLL, and support additional features such as [`QueryOptionalDelayLoadedAPI`](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi2/nf-libloaderapi2-queryoptionaldelayloadedapi).

## Module Information Data Structures

A Windows module list is a circular doubly linked list of type [`LDR_DATA_TABLE_ENTRY`](https://www.geoffchappell.com/studies/windows/km/ntoskrnl/inc/api/ntldr/ldr_data_table_entry.htm). The loader maintains multiple `LDR_DATA_TABLE_ENTRY` lists containing the same entries but in different link orders. These lists include `InLoadOrderModuleList`, `InMemoryOrderModuleList`, and `InInitializationOrderModuleList`, which are the list heads that can be found in `ntdll!PEB_LDR_DATA`. Each `LDR_DATA_TABLE_ENTRY` structure houses `InLoadOrderLinks`, `InMemoryOrderLinks`, and `InInitializationOrderLinks`, which are `LIST_ENTRY` structures (containing both `Flink` and `Blink` pointers), thus building the module lists between `LDR_DATA_TABLE_ENTRY` nodes.

The glibc (on Linux) module list is a linear (i.e. non-circular) doubly linked list of type [`link_map`](https://elixir.bootlin.com/glibc/glibc-2.38/source/include/link.h#L86). `link_map` contains both the `next` and `prev` pointers used to link the module information structures together into a list. glibc makes the list of loaded modules accessible for debugging purposes through the [`r_debug->r_map`](code/glibc/scripts/debugging-commands.md#link_map-analysis) symbol it exposes, which is a list head for the list of `link_map` structures.

This section only covers the central module information data structure to each loader.

## Loader Components

### Locks

Operating-system-level synchronization mechanisms of all kinds, including thread synchronization locks (i.e. Windows critical section or POSIX mutex), [readers-writer locks](https://en.wikipedia.org/wiki/Readers%E2%80%93writer_lock) (i.e. [Windows slim reader/writer locks](https://learn.microsoft.com/en-us/windows/win32/sync/slim-reader-writer--srw--locks) or [POSIX rwlock locks](https://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_rwlock_unlock.html), which can be acquired in exclusive/write or shared/read mode), and [inter-process synchronization locks](https://learn.microsoft.com/en-us/windows/win32/sync/interprocess-synchronization). Operating-system-level, meaning these locks may make a system call (a `syscall` instruction on x86-64) to perform a non-busy wait if the synchronization mechanism is owned/contended/waiting.

An intra-process OS lock uses an atomic flag as its locking primitive when there is no contention (e.g. implemented with the `lock cmpxchg` instruction on x86). Inter-process locks such as Win32 event synchronization objects must rely entirely on system calls to provide synchronization (e.g. the Windows event object `NtSetEvent` and `NtResetEvent` functions are just stubs containing a `syscall` instruction).

Some lock varieties are a mix of an OS lock and a spinlock (i.e. [busy loop](https://en.wikipedia.org/wiki/Busy_waiting)). For example, both a Windows critical section and a [GNU mutex](https://www.gnu.org/software/libc/manual/html_node/POSIX-Thread-Tunables.html) (*not* POSIX; this is a GNU extension) support specifying a spin count. When there is contention on a lock, its spin count is a potential performance optimization for avoiding the expensive context switch between user mode and kernel mode that occurs when performing a system call.

- Windows `LdrpModuleDatatableLock`
  - Performs full blocking access to its respective module information data structures
    - This includes two linked lists (`InLoadOrderModuleList` and `InMemoryOrderModuleList`), a hash table, two red-black trees (`ntdll!LdrpModuleBaseAddressIndex` and `ntdll!LdrpMappingInfoIndex`), and more
    - The lock also helps to protect the loader directed acyclic graph (DAG) data structures (`LDR_DDAG_NODE.Dependencies` and `LDR_DDAG_NODE.IncomingDependencies`); however, protecting this data can also require another lock
      - Read operations (e.g. walking between nodes) on the DAG data structures are safe when a given thread: has the `LdrpLoadCompleteEvent` lock **OR** has the `LdrpModuleDatatableLock` lock
      - Write operations (e.g. adding/deleting nodes) on the DAG data structures are safe when a given thread: has the `LdrpLoadCompleteEvent` lock **AND** has the `LdrpModuleDatatableLock` lock
      - Additionally acquiring the `LdrpModuleDatatableLock` lock for write operations is necessary to ensure the DAGs remain consistent with other module information data structures that the loader maintains, for which the the loader does not acquire the `LdrpLoadCompleteEvent` lock before modifying
    - This lock also protects some structure members contained within these data structures (e.g. the `LDR_DDAG_NODE.LoadCount` reference counter)
  - Windows shortly acquires `LdrpModuleDatatableLock` many times every `LoadLibrary` operation (17 times exactly, tested while loading an empty sample DLL)
    - Monitor changes to `LdrpModuleDatatableLock` by setting a watchpoint: `ba w8 ntdll!LdrpModuleDatatableLock`
       - **Note:** There are a few occurrences of this lock's data being modified directly when unlocking instead of calling `RtlReleaseSRWLockExclusive` (this is likely done as a performance optimization on hot paths)
  - Implemented as a slim read/write (SRW) lock, although the Windows loader only ever acquires this lock in the exclusive/write locking mode

- Linux (GNU loader) `dl_load_write_lock`
  - Performs full blocking (exclusive/write) access to its respective module data structures
  - On Linux, this is only a linked list (the`link_map` list)
  - Linux shortly acquires `dl_load_write_lock` once on every `dlopen` from the `_dl_add_to_namespace_list` internal function (see the [GDB log](code/glibc/dlopen/gdb-log.html) for evidence)
    - Other functions that acquire `dl_load_write_lock` (not called during `dlopen`) include the [`dl_iterate_phdr`](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/dl-iteratephdr.c#L39) function, which is for [iterating over the module list](https://linux.die.net/man/3/dl_iterate_phdr)
      - [According to glibc source code](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/dl-support.c#L216-L219), this lock is acquired to: "keep `__dl_iterate_phdr` from inspecting the list of loaded objects while an object is added to or removed from that list."
      - On Windows, acquiring the equivalent `LdrpModuleDatatableLock` is required to iterate the module list safely (e.g. when calling the `LdrpFindLoadedDllByNameLockHeld` function)

- Windows `LdrpLoaderLock`
  - Blocks concurrent module initialization/deinitialization and protects the `PEB_LDR_DATA.InInitializationOrderModuleList` linked list
    - Safely running a module initialization/finalization routines (`DLL_PROCESS_ATTACH`/`DLL_PROCESS_DETACH`) or DLL thread initialization/finalization rotuines (`DLL_THREAD_ATTACH`/`DLL_THREAD_DETACH`) requires holding loader lock
  - On the modern Windows loader at `DLL_PROCESS_ATTACH`, the loader lock remains locked as each full dependency chain of a loading DLL, including the loading DLL itself, initializes (i.e. the loader locks loader lock once before starting `LdrpInitializeGraphRecurse` and unlocks after returning from that function)
  - Implemented as a critical section

- Linux (GNU loader) `dl_load_lock`
  - This lock is acquired right at the [start of a `_dl_open`](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/dl-open.c#L824), `dlclose`, and other loader functions
    - `dlopen` eventually calls `_dlopen` after some preparation work (which shows in the call stack) like setting up an exception handler, at which point the loader is committed to doing some loader work
  - According to glibc source code, this lock's purpose is to: "Protect against concurrent loads and unloads."
    - This protection includes concurrent module initialization similar to how a modern Windows `ntdll!LdrpLoaderLock` does
    - For example, `dl_load_lock` protects a concurrent `dlclose` from running a library's module destructors before that library's module initializer have finished running
  - `dl_load_lock` is at the top of the [loader's lock hierarchy](#gnu-loader-lock-hierarchy)
  - Since `dl_load_lock` protects the entire library loading/unloading process from beginning to end, the closest modern Windows loader equivalent synchronization mechanism would be the `LdrpLoadCompleteEvent` loader event

- Linux (GNU loader) [`_ns_unique_sym_table.lock`](https://elixir.bootlin.com/glibc/glibc-2.38/source/elf/rtld.c#L337)
  - This is a per-namespace lock for protecting that namespace's **unique** (`STB_GNU_UNIQUE`) symbol hash table
  - `STB_GNU_UNIQUE` symbols are a type of symbol that a module can expose; they are considered a misfeature of the GNU loader
  - As standardized by the ELF executable format, the GNU loader uses a per-module statically allocated (at compile time) symbol table for locating symbols within a module (`.so` shared object file); however, the `_ns_unique_sym_table.lock` lock protects a separate dynamically allocated hash table specially for `STB_GNU_UNIQUE` symbols
    - See the [Symbol Lookup Operating System Comparison](#symbol-lookup-operating-system-comparison) section for more information on how the GNU loader typically locates symbols
    - In Windows terminology, the closest approximation to a symbol would be a DLL's function exports (there's no mention of the word "export" in the `objdump` manual)
    - Use `readelf` to dump all the symbols, including unique symbols, of an ELF file: `readelf --symbols --file <FILE>` (**Note:** [The `readelf` tool is preferable over `objdump`](https://stackoverflow.com/a/8979687))
  - Internally, the call chain for looking up a `STB_GNU_UNIQUE` symbol starting with `dlsym` goes `dlsym` ➜ `dl_lookup_symbol_x` ➜ `do_lookup_x` ➜ `do_lookup_unique` where finally, `_ns_unique_sym_table.lock` is acquired
  - For more information on `STB_GNU_UNIQUE` symbols, see the [section covering `STB_GNU_UNIQUE`](#elf-flat-symbol-namespace-global-symbols-gnu-namespaces-and-unique-symbols)

- `ntdll!LdrpInitCompleteEvent`
  - This event ([Win32 event object](code/windows/event-experiment/event-experiment.c)) being set indicates loader initialization is complete
  - Loader initialization includes process initialization
  - This event is *only* set (`NtSetEvent`) by the `LdrpProcessInitializationComplete` function soon after `LdrpInitializeProcess` returns, at which point it's never set/unset again
  - Thread startup waits on this
  - This event is **not** an auto-reset event and is created in the nonsignaled (i.e. waiting) state
  - The `LdrpInitialize` (`_LdrpInitialize`) function creates this event
  - This event is created before loader initialization begins (early at process creation)

- `ntdll!LdrpLoadCompleteEvent`
  - The loader uses this event to signal when a library load/unload has completed or to determine if a concurrent loader operation should wait for a concurrent library load/unload operation to complete before proceeding
  - This event is set (`NtSetEvent`) in the `LdrpDropLastInProgressCount` function before relinquishing control as the load owner (`LoadOwner` flag in `TEB.SameTebFlags`)
  - Thread startup waits on this
  - This event is an auto-reset event and is created in the nonsignaled (i.e. waiting) state
  - Created by the `LdrpInitParallelLoadingSupport` function calling the  `LdrpCreateLoaderEvents` function at process startup

- `LdrpWorkCompleteEvent`
  - The loader users this event to determine when all loader worker threads have collectively finished processing (i.e. mapping and snapping) all items work queue
  - This event is set (`NtSetEvent`) immediately before the `LdrpProcessWork` function returns *if* the work queue is now empty or all current loader worker threads are finished processing work
  - This event is an auto-reset event and is created in the nonsignaled (i.e. waiting) state
  - Created by the `LdrpInitParallelLoadingSupport` function calling the  `LdrpCreateLoaderEvents` function at process startup
  - For in-depth information on the latter two events, see the [High-Level Loader Synchronization](#high-level-loader-synchronization) section

- `ntdll!LdrpWorkQueueLock`
  - Used in the `LdrpDrainWorkQueue` function to ensure that only one thread can access the `LdrpWorkQueue` work queue, and other related state, at a time
  - Implemented as a critical section

- `ntdll!LdrpDllNotificationLock`
  - This lock protects the `LdrpDllNotificationList` list and remains held during callback execution to ensure the execution of these callbacks cannot overlap
  - The loader runs these callbacks in a few places, such as at module load time using the `LdrpSendPostSnapNotifications` function after completing snapping work but before running module initialization routines
  - By default, the `LdrpDllNotificationList` list is empty, so the `LdrpSendDllNotifications` function does not send any callbacks
    - Notifications callbacks are registered with `LdrRegisterDllNotification` and are then sent with `LdrpSendDllNotifications` (it runs the callback function)
    - By putting Google Chrome under WinDbg, I found an instance where the loader ran a post-snap DLL notification callback. The callback ran the `apphelp!SE_DllLoaded` function.
  - Functions that may call `LdrpSendDllNotifications` to run notification callbacks include: `LdrpSendPostSnapNotifications`, `LdrpUnloadNode`, and `LdrpCorProcessImports` (the latter be called by `LdrpMapDllWithSectionHandle`)
  - Reading loader disassembly, you may see quite a few places where loader functions check the `LdrpDllNotificationLock` lock like so: `RtlIsCriticalSectionLockedByThread(&LdrpDllNotificationLock)`
    - For instance, in the `ntdll!LdrpAllocateModuleEntry`, `ntdll!LdrGetProcedureAddressForCaller`, `ntdll!LdrpPrepareModuleForExecution`, and `ntdll!LdrpMapDllWithSectionHandle` functions
    - These checks detect if the current thread is executing a DLL notification callback and then implement special logic for that edge case (for this reason, they can generally be ignored)
  - The actual call to a notification callback in WinDbg disassmebly looks like this because of CFG protecting the indirect call: `call    qword ptr [ntdll!__guard_dispatch_icall_fptr (<ADDRESS>)]`
  - [ReactOS added support for DLL notification callbacks in 2025](https://github.com/reactos/reactos/pull/6795)

- `LDR_DATA_TABLE_ENTRY.Lock`
  - Starting with Windows 10, each `LDR_DATA_TABLE_ENTRY` has a [`Lock` member](https://www.geoffchappell.com/studies/windows/km/ntoskrnl/inc/api/ntldr/ldr_data_table_entry.htm) (it replaced a `Spare` slot), which points to a SRW lock
  - The `LdrpWriteBackProtectedDelayLoad` function uses this per-node lock to implement concurrency protection while temporarily modifying the memory protection state of a module's Import Address Table (IAT) during the lazy linking part of Windows delay loading

- `PEB.TppWorkerpListLock`
  - This SRW lock (typically acquired exclusively) exists in the PEB to control access to the member immediately below it, which is the `TppWorkerpList` doubly linked list
  - This list keeps track of all the threads belonging to any thread pool in the process
    - These threads show up as `ntdll!TppWorkerThread` threads in WinDbg
    - There's a list head, after which each `LIST_ENTRY` points into the stack memory of a thread owned by a thread pool
      - The `TpInitializePackage` function (called by `LdrpInitializeProcess`) initializes the list head, then the main `ntdll!TppWorkerThread` function of each new thread belonging to a thread pool adds itself to the list
    - The threads in this list include threads belonging to the loader worker thread pool (`LoaderWorker` in `TEB.SameTebFlags`)
  - This is a bit out of scope since it relates to thread pool internals, not loader internals (however, the loader relies on thread pool internals to implement parallelism for loader workers)

- Searching symbols reveals more of the loader's locks: `x ntdll!Ldr*Lock`
  - `LdrpDllDirectoryLock` (SRW lock, sometimes acquired in shared mode), `LdrpTlsLock` (SRW lock, sometimes acquired in shared mode), `LdrpEnclaveListLock` (critical section lock), `LdrpPathLock` (SRW lock, only acquired exclusive mode), `LdrpInvertedFunctionTableSRWLock` (SRW lock, sometimes acquired in shared mode, high contention, locking and unlocking functions are inlined), `LdrpVehLock` (SRW lock, only acquired exclusive mode), `LdrpForkActiveLock` (SRW lock, sometimes acquired in shared mode), `LdrpCODScenarioLock` (SRW lock, only acquired exclusive mode, COD stands for component on demand and it is an application compatibility mechanism that integrates with the Program Compatibility Assistant service), `LdrpMrdataLock` (SRW lock, only acquired exclusive mode), and `LdrpVchLock` (SRW lock, only acquired exclusive mode)
  - The Windows loader may also dynamically create and destroy temporary synchronization objects in some cases (e.g. see the Windows loader's calls to `ntdll!ZwCreateEvent`)

### State

A state value may either be shared state (also known as global state) or local state (i.e. whether separate threads may access the state). Shared state has access to it protected by one of the aforementioned locks. Local state does not require protection. Only a few key pieces of Windows loader state I came across are listed here.

- `LDR_DDAG_NODE.State`
  - Each module has a `LDR_DDAG_NODE` structure with a `State` member containing **15 possible states** -5 through 9
  - `LDR_DDAG_NODE.State` tracks a module's **entire lifetime** from allocating module information data structures (`LdrpAllocatePlaceHolder`) and loading to unload and subsequent deallocation of its module information structures
    - In my opinion, this makes the combined `LDR_DDAG_NODE.State` values of all modules to be the **most important** piece of loader state**
  - Performing each state change *may* necessitate acquiring the `LdrpModuleDatatableLock` lock to ensure consistency between module information data structures
    - The specific state changes requiring `LdrpModuleDatatableLock` protection (i.e. consistency between all module information data structures) are documented in the link below
  - Please see the [Windows Loader Module State Transitions Overview](#windows-loader-module-state-transitions-overview) for more information

- `ntdll!LdrpWorkInProgress`
  - This [reference counter](https://en.wikipedia.org/wiki/Reference_counting) is a key piece of loader state (zero meaning work is *not* in progress and up from that meaning work *is* in progress)
    - It's not modified atomically with `lock` prefixed instructions
  - Acquiring the `LdrpWorkQueueLock` lock is a **requirement** for safely modifying the `LdrpWorkInProgress` state and `LdrpWorkQueue` linked list
    - I verified this by setting a watchpoint on `LdrpWorkInProgress` and noticing that `LdrpWorkQueueLock` is always locked while checking/modifying the `LdrpWorkInProgress` state (also, I searched disassembly code)
      - The `LdrpDropLastInProgressCount` function makes this clear because it briefly acquires `LdrpWorkQueueLock` around *only* the single assembly instruction that sets `LdrpWorkInProgress` to zero
  - Please see the [Windows Loader Module State Transitions Overview](#windows-loader-module-state-transitions-overview) for more information

- `ntdll!LdrInitState`
  - This value is *not* modified atomically with `lock` prefixed instructions
    - Loader initialization is a procedural process only occurring once and on one thread, so this value doesn't require protection
  - In ReactOS code, the equivalent value is `LdrpInLdrInit`, which the code declares as a `BOOLEAN` value
  - In a modern Windows loader, this a 32-bit integer (likely an `enum`) ranging from zero to three; here are the state transitions:
    - `LdrpInitialize` initializes `LdrInitState` to zero (**loader is uninitialized**)
    - `LdrpInitializeProcess` calls `LdrpEnableParallelLoading` and immediately after sets `LdrInitState` to one (**mapping and snapping dependency graph**)
    - `LdrpInitializeProcess` sets `LdrInitState` to two (**initializing dependency graph**)
      - The `DLL_PROCESS_ATTACH` routines of `DllMain` at process startup runs with this state active
    - `LdrpInitialize` (`LdrpInitializeProcess` returned), shortly before calling `LdrpProcessInitializationComplete`, sets `LdrInitState` to three (**loader initialization is done**)

- `LDR_DDAG_NODE.LoadCount`
  - This is the reference counter for a `LDR_DDAG_NODE` structure; safely modifying it requires acquiring the `LdrpModuleDataTableLock` lock

- `TEB.WaitingOnLoaderLock` is thread-specific data set when a thread is waiting for loader lock
  - `RtlpWaitOnCriticalSection` (`RtlEnterCriticalSection` calls `RtlpEnterCriticalSectionContended`, which calls this function) checks if the contended critical section is `LdrpLoaderLock` and if so, sets `TEB.WaitingOnLoaderLock` equal to one
    - This branch condition runs every time any contended critical section gets waited on, which is interesting (monolithic much?)
  - `RtlpNotOwnerCriticalSection` (called from `RtlLeaveCriticalSection`) also checks `LdrpLoaderLock` (and some other information from `PEB_LDR_DATA`) for special handling
    - However, this is only for error handling and debugging because a thread that doesn't own a critical section should have never attempted to leave it in the first place

- Flags in `TEB.SameTebFlags`, including: `LoadOwner`, `LoaderWorker`, and `SkipLoaderInit`
  - All of these were introduced in Windows 10 (`SkipLoaderInit` only in 1703 and later)
  - `LoadOwner` (flag mask `0x1000`) is state that a thread uses to inform itself that it's the one responsible for completing the work in progress (`ntdll!LdrpWorkInProgress`)
    - The `LdrpDrainWorkQueue` function sets the `LoadOwner` flag on the current thread immediately after setting `ntdll!LdrpWorkInProgress` to `1`, thus directly connecting these two pieces of state
    - The `LdrpDropLastInProgressCount` function unsets this flag along with `ntdll!LdrpWorkInProgress`
    - Any thread doing loader work (e.g. `LoadLibrary`) will temporarily receive this TEB flag
    - This state is local to the thread (in the TEB), so it doesn't require the protection of a lock
  - `LoaderWorker` (flag mask `0x2000`) identifies loader worker threads
    - These show up as `ntdll!TppWorkerThread` in WinDbg
    - On thread creation, `LdrpInitialize` checks if the thread is a loader worker and, if so, handles it specially
    - This flag can be set on a new thread using [`NtCreateThreadEx`](https://ntdoc.m417z.com/ntcreatethreadex)
  - `SkipLoaderInit` (flag mask `0x4000`) tells the spawning thread to skip all loader initialization
      - In `LdrpInitialize` IDA decompilation, you can see `SameTebFlags` being tested for `0x4000`, and if present, loader initialization is completely skipped (`_LdrpInitialize` is never called)
    - This could be useful for creating new threads without being blocked by loader events
    - This flag can be set on a new thread using [`NtCreateThreadEx`](https://ntdoc.m417z.com/ntcreatethreadex)

- `ntdll!LdrpMapAndSnapWork`
  - An undocumented, global structure that loader worker (`LoaderWorker` flag in `TEB.SameTebFlags`) threads read from to get mapping and snapping work
  - The first member of this undocumented structure is an atomic reference counter that gets incremented whenever work is enqueued (`ntdll!LdrpQueueWork` function) to the global work queue (`ntdll!LdrpWorkQueue` linked list) and decremented whenever a loader worker thread consumes work
  - The undocumented structure is incremented by the `ntdll!TppWorkPost` function (called by `ntdll!LdrpQueueWork`) and decremented by the `ntdll!TppIopCallbackEpilog` function (called by thread pool internals in a loader worker thread), which means this undocumented structure belongs to thread pool internals and so is a bit out of scope here

### Atomic State

An atomic state value is modified using a single assembly instruction. On an [SMP](https://en.wikipedia.org/wiki/Symmetric_multiprocessing) operating system (e.g. Windows and very likely your build of Linux; check with the `uname -a` command) with a multi-core processor, this instruction must include a `lock` prefix (on x86) so the processor knows to synchronize that memory access across CPU cores. The x86 ISA mandates that a single memory read/write operation is atomic by default. It is only when combining multiple reads or writes at once that the `lock` prefix is necessary to guarantee atomicity (e.g. a read + write operation, typically performed atomically, is needed to increment or decrement a reference counter). Only a few key pieces of the Windows loader atomic state I came across are listed here.

- `ntdll!LdrpProcessInitialized`
  - This value is modified atomically with a `lock cmpxchg` instruction
  - As the name implies, it indicates whether process initialization has been completed (`LdrpInitializeProcess`)
  - This is an enum ranging from zero to two; here are the state transitions:
    - NTDLL compile-time initialization starts `LdrpProcessInitialized` with a value of zero (process is uninitialized)
    - `LdrpInitialize` increments `LdrpProcessInitialized` to one zero early on (initialization event created)
      - If the process is still initializing, newly spawned threads jump to calling `NtWaitForSingleObject`, waiting on the `LdrpInitCompleteEvent` loader event before proceeding
      - Before the loader calls `NtCreateEvent` to create `LdrpInitCompleteEvent` at process startup, spawning a thread into a process causes it to use `LdrpProcessInitialized` as a spinlock (i.e. a busy loop)
      - For example, if a remote process calls `CreateRemoteThread` and the thread spawns before the creation of `LdrpInitCompleteEvent` (an unlikely but possible race condition)
    - `LdrpProcessInitializationComplete` increments `LdrpProcessInitialized` to two (process initialization is done)
      - This happens immediately before setting the `LdrpInitCompleteEvent` loader event so other threads can run
      - After `LdrpProcessInitializationComplete` returns, `NtTestAlert` processes the asynchronous procedure call (APC) queue, and finally, `NtContinue` yields code execution of the current thread to `KERNEL32!BaseThreadInitThunk`, which eventually runs our program's `main` function

- `LDR_DATA_TABLE_ENTRY.ReferenceCount`
  - This is a [reference counter](https://en.wikipedia.org/wiki/Reference_counting) for `LDR_DATA_TABLE_ENTRY` structures
  - On load/unload, this state is modified by `lock` `inc`/`dec` instructions, respectively (although, on the initial allocation of a `LDR_DATA_TABLE_ENTRY` before linking it into any shared data structures, of course, no locking is necessary)
  - The `LdrpDereferenceModule` function atomically decrements `LDR_DATA_TABLE_ENTRY.ReferenceCount` by passing `0xffffffff` to a `lock xadd` assembly instruction, causing the 32-bit integer to overflow to one less than what it was (x86 assembly doesn't have an `xsub` instruction so this is the standard way of doing this)
    - Note that the `xadd` instruction isn't the same as the `add` instruction because the former also atomically exchanges (hence the "x") the previous memory value into the source operand
      - This is at the assembly level; in code, Microsoft is likely using the [`InterlockedExchangeSubtract`](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-interlockedexchangesubtract) macro to do this
    - The `LdrpDereferenceModule` function tests (among other things) if the previous memory value was 1 (meaning post-decrement, it's now zero; i.e. nobody is referencing this `LDR_DATA_TABLE_ENTRY` anymore) and takes that as a cue to unmap the entire module from memory (calling the `LdrpUnmapModule` function, deallocating memory structures, etc.)

- `ntdll!LdrpLoaderLockAcquisitionCount`
  - This value is modified atomically with the `lock xadd` prefixed instructions
  - It was only ever used as part of [cookie generation](https://doxygen.reactos.org/d7/d55/ldrapi_8c.html#a03431c9bfc0cee0f8646c186eb0bad32) ([web archive](https://web.archive.org/web/20240830091830/https://doxygen.reactos.org/d7/d55/ldrapi_8c.html#a03431c9bfc0cee0f8646c186eb0bad32), because Doxygen links can change) in the `LdrLockLoaderLock` function
    - On both older/modern loaders, `LdrLockLoaderLock` adds to `LdrpLoaderLockAcquisitionCount` every time it acquires the loader lock (it's never decremented)
    - In a legacy (Windows Server 2003) Windows loader, the `LdrLockLoaderLock` function (an NTDLL export) was often used internally by NTDLL even in `Ldr` prefixed functions. However, in a modern Windows loader, it's mostly phased out in favor of the `LdrpAcquireLoaderLock` function
    - In a modern loader, the only places where I see `LdrLockLoaderLock` called are from non-`Ldr` prefixed functions, specifically: `TppWorkCallbackPrologRelease` and `TppIopExecuteCallback` (thread pool internals, still in NTDLL)

## Reverse Engineered Windows Loader Functions

### `LdrpDrainWorkQueue`

The `LdrpDrainWorkQueue` function is responsible for the [high-level synchronization of the loader](#high-level-loader-synchronization) and for [helping to process work when it is immediately available](code/windows/loadlibrary-concurrent-work/README.md). See the [Parallel Loader Overview](#parallel-loading-overview) section for contextual information.

```c
// Variable names are my own

typedef enum {
    LoadOwner,
    LoaderWorker
} LoadType;

// The caller decides whether the context LdrpDrainWorkQueue should work under
// From the perspective of module initialization, the call to LdrpDrainWorkQueue that acquires a loader event before running module initialization routine must work under the load owner context unless the routine is reentering the loader. In the reentrant case, LdrpLoadCompleteEvent will have already been acquired so it cannot be acquired again (an event object is not a reentrant synchronization mechanism)
// This behavior can be seen in the ntdll!LdrpLoadDllInternal function, if it is to call LdrpDrainWorkQueue then it will always go in with the load owner context if the current thread is not already the load owner (it checks for `LoadOwner` in `TEB.SameTebFlags`); otherwise, there are additional conditions that must pass for ntdll!LdrpLoadDllInternal to call LdrpDrainWorkQueue with the loader worker context
struct PTEB LdrpDrainWorkQueue(LoadType LoadContext)
{
    HANDLE EventHandle;
    BOOL CompleteRetryOrReturn;
    BOOL LdrpDetourExistAtStart;
    PLIST_ENTRY LdrpWorkQueueEntry;
    //PLIST_ENTRY LinkHolderCheckCorruptionTemp; // This variable is inlined by the call to RtlpCheckListEntry
    PTEB CurrentTeb;
    PLIST_ENTRY LdrpRetryQueueEntry;
    PLIST_ENTRY LdrpRetryQueueBlink;

    CompleteRetryOrReturn = FALSE

    EventHandle = (LoadContext == LoadOwner) ? LdrpLoadCompleteEvent : LdrpWorkCompleteEvent;

    while ( TRUE )
    {
        while ( TRUE )
        {
            RtlEnterCriticalSection(&LdrpWorkQueueLock);
            // LdrpDetourExists relates to LdrpCriticalLoaderFunctions, find a list of these functions within this repo
            LdrpDetourExistAtStart = LdrpDetourExist;
            if ( !LdrpDetourExist || LoadContext == LoaderWorker )
            {
                LdrpWorkQueueEntry = &LdrpWorkQueue;
                // Corruption check on LdrpWorkQueue list: https://www.alex-ionescu.com/new-security-assertions-in-windows-8/
                RtlpCheckListEntry(LdrpWorkQueueEntry);

                LdrpWorkQueueEntry = LdrpWorkQueue.Flink;

                // Test if LdrpWorkQueue is empty
                if ( &LdrpWorkQueue == LdrpWorkQueueEntry ) {
                    if ( LdrpWorkInProgress == LoadContext ) {
                        LdrpWorkInProgress = 1;
                        CompleteRetryOrReturn = 1;
                    }
                } else {
                    if ( !LdrpDetourExistAtStart )
                        ++LdrpWorkInProgress;
                    // LdrpUpdateStatistics is a very small function with one branch on whether we're a loader worker thread
                    LdrpUpdateStatistics();
                }
            }
            else
            {
                if ( LdrpWorkInProgress == LoadContext ) {
                    LdrpWorkInProgress = 1;
                    CompleteRetryOrReturn = TRUE;
                }

                LdrpWorkQueueEntry = &LdrpWorkQueue;
            }
            RtlLeaveCriticalSection(&LdrpWorkQueueLock);

            // We only synchronize on a loader event if both conditions are met:
            // 1. CompleteRetryOrReturn is FALSE
            // 2. The work queue is not empty
            // The LdrpDrainWorkQueue function can return without synchronizing on a loader event (I have seen this happen in testing with the LdrpLoadCompleteEvent loader event)

            if ( CompleteRetryOrReturn )
                break;

            // Test if LdrpWorkQueue is empty
            if ( &LdrpWorkQueue == LdrpWorkQueueEntry )
            {
                // No mapping and snapping work left to do, just wait our turn
                NtWaitForSingleObject(EventHandle, 0, NULL);
            }
            else
            {
                // Help process the work while we're here
                // LdrpProcessWork processes (i.e. mapping and snapping) the specified work item
                // LdrpWorkQueueEntry - 8: Navtigate to the item above the list link in the undocumented LDRP_LOAD_CONTEXT structure (this reverse engineered code should be using the CONTAINING_RECORD macro like the real code would be doing)
                LdrpProcessWork(LdrpWorkQueueEntry - 8, LdrpDetourExistAtStart);
            }
        }

        // Test if we were called in the LoadOwner context OR if LdrpRetryQueue is empty
        //
        // WinDbg disassembly (IDA disassembly with "cs:" and decompilation is poor here):
        // lea     rbx, [ntdll!LdrpRetryQueue (7ffb5bebc3a0)]
        // cmp     qword ptr [ntdll!LdrpRetryQueue (7ffb5bebc3a0)], rbx
        // je      ntdll!LdrpDrainWorkQueue+0xb1 (7ffb5bdaea85)
        // https://stackoverflow.com/a/68702967
        if ( LoadContext == LoadOwner || &LdrpRetryQueue == LdrpRetryQueue.Flink )
            break;

        RtlEnterCriticalSection(&LdrpWorkQueueLock);

        // Complete a retried mapping and snapping operation

        // Add a work item to LdrpWorkQueue from LdrpRetryQueue then clear LdrpRetryQueue
        // Reverse engineered based on WinDbg disassembly due to the IDA issue described above
        // TODO: Use proper list modification macros
                                                            // r12 is ntdll!LdrpWorkQueue
                                                            // rbx is ntdll!LdrpRetryQueue
        // Add first entry of LdrpRetryQueue to LdrpWorkQueue and remove that entry from LdrpRetryQueue
        LdrpRetryQueueEntry = LdrpRetryQueue.Flink;         // mov     rax, qword ptr [ntdll!LdrpRetryQueue (7ffb5bebc3a0)]
                                                            // lea     rcx, [ntdll!LdrpWorkQueueLock (7ffb5bebc3c0)]
                                                            // xorps   xmm0, xmm0 (any value xor'd by itself is zero)
        LdrpRetryQueueEntry.Blink = &LdrpWorkQueue;         // mov     qword ptr [rax+8], r12
        LdrpWorkQueue.Flink = LdrpRetryQueueEntry.Flink;    // mov     qword ptr [ntdll!LdrpWorkQueue (7ffb5bebc3f0)], rax
                                                            // mov     rax, qword ptr [ntdll!LdrpRetryQueue+0x8 (7ffb5bebc3a8)]
        LdrpRetryQueue.Blink = &LdrpWorkQueue;              // mov     qword ptr [rax], r12
        LdrpWorkQueue.Blink = LdrpRetryQueue.Blink;         // mov     qword ptr [ntdll!LdrpWorkQueue+0x8 (7ffb5bebc3f8)], rax

        // Clear the LdrpRetryQueue list
        LdrpRetryQueue.Blink = &LdrpRetryQueue;             // mov     qword ptr [ntdll!LdrpRetryQueue+0x8 (7ffb5bebc3a8)], rbx
        LdrpRetryQueue.Flink = &LdrpRetryQueue;             // mov     qword ptr [ntdll!LdrpRetryQueue (7ffb5bebc3a0)], rbx

        // Clear ntdll!LdrpRetryingModuleIndex
        // Global used by ntdll!LdrpCheckForRetryLoading function (may be called during mapping by ntdll!LdrpMinimalMapModule or ntdll!LdrpMapDllNtFileName functions)
        // ntdll!LdrpRetryingModuleIndex is a red-black tree (LdrpCheckForRetryLoading modifies it by calling RtlRbInsertNodeEx)
        // Each entry in ntdll!LdrpRetryingModuleIndex is a structure of the undocumented LDRP_LOAD_CONTEXT type
        LdrpRetryingModuleIndex = NULL;                     // movdqu  xmmword ptr [ntdll!LdrpRetryingModuleIndex (7ffb5bebd350)], xmm0

        RtlLeaveCriticalSection(&LdrpWorkQueueLock);
        CompleteRetryOrReturn = FALSE;
    }

    // Give context to this thread as the LoadOwner, which is state used by the loader
    // A thread can have the LoadOwner flag while not owning the ntdll!LdrpLoadCompleteEvent lock
    CurrentTeb = NtCurrentTeb();
    CurrentTeb->SameTebFlags |= LoadOwner; // 0x1000
    return CurrentTeb;
}
```

### `LdrpDecrementModuleLoadCountEx`

```c
NTSTATUS LdrpDecrementModuleLoadCountEx(LDR_DATA_TABLE_ENTRY Entry, BOOL DontCompleteUnload)
{
    LDR_DDAG_NODE Node;
    NTSTATUS Status;
    BOOL CanUnloadNode;
    DWORD_PTR LdrpReleaseLoaderLockReserved; // Not used or even initialized, so I still consider this function fully reverse engineered (also, LdrpReleaseLoaderLock never touches this parameter)

    // If the next reference counter decrement will drop the LDR_DDAG_NODE into having zero references then we may want to retry later
    // Specifying DontCompleteUnload = FALSE requires that the caller has made this thread the load owner
    if ( DontCompleteUnload && Entry->DdagNode->LoadCount == 1 )
    {
        // Retry when we're the load owner
        // NTSTATUS code 0xC000022D: https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-erref/596a1078-e883-4972-9bbc-49e60bebca55
        return STATUS_RETRY;
    }

    RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);
    Node = Entry->DdagNode;
    Status = LdrpDecrementNodeLoadCountLockHeld(Node, DontCompleteUnload, &CanUnloadNode);
    RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);

    if ( CanUnloadNode )
    {
        LdrpAcquireLoaderLock();
        // LdrpUnloadNode runs a module's DLL_PROCESS_DETACH
        // It also walks the dependency graph to unload any other now unused modules
        LdrpUnloadNode(Node);
        LdrpReleaseLoaderLock(LdrpReleaseLoaderLockReserved, 8); // Second parameter is an ID for use in log messages
    }
}
```

### `LdrpDropLastInProgressCount`

Please see the [High-Level Loader Synchronization](#high-level-loader-synchronization) section for the reverse engineering of this function.

### `LdrpProcessWork`

Please see the [High-Level Loader Synchronization](#high-level-loader-synchronization) section for a partial reverse engineering of this function.

## What is COM?

According to official Microsoft documentation, the [Component Object Model (COM)](https://learn.microsoft.com/en-us/windows/win32/com/component-object-model--com--portal) is:

> … a platform-independent, distributed, object-oriented system for creating binary software components that can interact.

In the book *Essential COM* by Don Box, the creator of COM, the answer to "What is COM?" is stated to be:

> … an architecture for component reuse that allows dynamic and efficient composition of systems from independently developed binary components.

In [*House of COM: Is COM Dead?*](https://learn.microsoft.com/en-us/archive/msdn-magazine/2000/december/house-of-com-is-com-dead) by Don Box, he said:

> COM is many things to many people. To me, COM is a programming model based on integrating components based on type. Period.

In this context, "type" refers to the interface ID or IID which is a globally unique identifier (GUID) representing a single COM component. The IID of a component is its interface name often used in COM code that [maps at compile-time to a random or developer-specified GUID for that component](https://github.com/mingw-w64/mingw-w64/blob/2e87f03fc17adf6a1a4f94d6793a4f5ae74fdc89/mingw-w64-headers/include/unknwnbase.h#L71-L75). In other words, "if you ask for `IID_IAbc` you are contractually obligated to receive a virtual table with these methods in this order, with this calling convention. This guarantee rests on the fact that [COM defines a binary standard](https://learn.microsoft.com/en-us/windows/win32/com/the-component-object-model#:~:text=COM%20is%20referred%20to%20as%20a%20binary%20standard).

In summary:

**COM is a binary standard for software module interaction in the [object-oriented paradigm](https://en.wikipedia.org/wiki/Object-oriented_programming).**

## Object-Oriented Software Frameworks Overview

**Note:** WORK IN PROGRESS! This section was formerly called "Component Model Overview" but "comonent model" is just a dated marketing term that has no place in technical writing. It can also be very broad with no real definition: I have seen .NET described as a component model as well as dynamic linking. I also want to refactor and simplify this section in other ways now that I better understand these technologies. Maybe add Java Remote Method Invocation (RMI) too. Also that OMG published incomplete standards seemingly on purpose causing there to be many incompatible vendor implementations of CORBA.

A component model is an object-oriented framework used for creating modular, reusable software units, and facilitating communication between them. A component is the term for a module (i.e. a deployable unit of code like an EXE, DLL, or JAR) that contains an object for use in a component model. Using these components together allows for development in the [object-oriented paradigm](https://en.wikipedia.org/wiki/Object-oriented_programming). A framework for transporting objects, calls, or messages is useful to ease communication between components and abastract away implementation details that may be separating them (i.e. [location transparency](https://en.wikipedia.org/wiki/Location_transparency)). If used with intent to solve the right problem, such a framework is a fantastic tool with no cons other than some acceptable overhead.

The centerpiece to any component model is that [each connection by a client corresponds to an instance of a component or object on the server](https://blogger.googleusercontent.com/img/b/R29vZ2xl/AVvXsEjmejLXbVAlYMo31XH0Mbzy2KFzhxyPxQTrO9_4k5n6gIPTP_QjRV50JTs82wdhPc8Agkbz4ONtCc3-ciZ1zc1oEt7hyI3iJZ_TLclmqrCk9w6xcSotuh3RJbqWVWmPIKWDjU9TKVVnxUXqFDI14CswLDIJCGv9v9nmGb9oX0SLSJHL46HKQbefSgzJ/s600/image5.png). As we will see, there are other common elements, but all component models revolve around this core idea of a connection to an instance (e.g. `CoCreateInstance` on Windows or `NSXPCConnection` on Mac) of an object thus enabling interaction with the services, methods, or interfaces exposed by that component.

### Microsoft Component Object Model (COM)

Component Object Model (COM) is Microsoft's component model. A component in COM is described by its binary interface thus creating an ABI to communicate with. A programmer writes interface descriptions in [Microsoft Interface Definition Language (MIDL)](https://en.wikipedia.org/wiki/Interface_description_language#Examples), which turns into that component's binary interface. COM works intra-process, inter-proess, and between machines (with the addition of DCOM, which has since [been integrated into COM](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-com/8b9b80c2-764f-4483-bfeb-43df402d1fb7)). [DCOM internally relies on Microsoft RPC.](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-dcom/7201a0d3-5b9f-43f6-895e-bdca8bda6d61) COM is deeply integrated into Windows, with many Windows APIs being implemented in COM (e.g. the [Task Scheduler API](https://learn.microsoft.com/en-us/windows/win32/api/taskschd/)) or using COM internally. The COM base interface class is [`IUnknown`](https://en.wikipedia.org/wiki/IUnknown), which is the root interface from which all other interfaces are derived. [COM and DCOM are openly specified protocols](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-dcom/4a893f3d-bd29-48cd-9f43-d9777a4415b0). COM is for Windows and isn't a cross-platform technology. Note that [XPCOM](https://en.wikipedia.org/wiki/XPCOM), by Mozilla, bares no direct relation to COM.

While COM is indeed a component model, where it differs most from other component model which we will cover is that Microsoft encourages its in-process use as an alternative to dynamic linking. In-process COM is by far the most commonly used type of COM within Windows and outside of Microsoft. Other component models like CORBA or Apple's NSXPCConnection also support their in-process use in theory but a focus on distributed objects by CORBA (besides binary incompatibilities between CORBA vendors also making it unviable) and a focus on out-of-process use for security sandboxing in NSXPCConnection means each call would require mediation by the technology's respective broker or backend thus raising overhead instead of making comparably fast virtual table calls directly to the target object like COM does when used in-process.

See [*What is COM?*](#what-is-com) for a clean and conscience definition of COM.

### Common Object Request Broker Architecture (CORBA)

Common Object Request Broker Architecture (CORBA) is an open, vendor-neutral standard that defines a framework for object-oriented communication across different platforms and programming languages. CORBA was developed by the Object Management Group (OMG), a consortium that creates and maintains standards for distributed computing. CORBA interfaces fit an exact binary description (like a C structure) that a programmer describes in [OMG IDL](https://en.wikipedia.org/wiki/Interface_description_language#Examples). CORBA can be used intra-process, inter-process, and between machines. Inter-process and machine-to-machine communication is done using the [General Inter-ORB Protocol (GIOP)](https://en.wikipedia.org/wiki/General_Inter-ORB_Protocol). The object request broker (ORB) is the central piece in CORBA, responsible for brokering requests between clients and server objects (similar to how an [object-relational mapping or ORM](https://en.wikipedia.org/wiki/Object%E2%80%93relational_mapping) framework, a technology that came after component model technology, allows interacting with an SQL database using the natural features of a given programming language except directly between languages for elegant, platform agonistic communication). ORBs allow for mapping objects between programming languages. The base interface class in CORBA is simply named `Object`. CORBA was one of the first component models, it gained traction throughout the 1990s, but after inspiring [many](https://en.wikipedia.org/wiki/Jakarta_Enterprise_Beans#Version_history) [other](https://en.wikipedia.org/wiki/Bonobo_(GNOME)) early component models, it fell by the wayside for a [variety of reasons](https://cacm.acm.org/practice/the-rise-and-fall-of-corba/) (including the rise of simple but powerful technologies, where often applicable, such as [REST](https://en.wikipedia.org/wiki/REST)).

### GNU/Linux Component Models and History

There does not strictly exist a component models common to GNU/Linux systems. Historically, [Bonobo](https://en.wikipedia.org/wiki/Bonobo_(GNOME)), based on CORBA, was the component model of choice by the GNOME Shell. GNOME officially deprecated Bonobo in [2009](https://web.archive.org/web/20090807072219/https://library.gnome.org/devel/api) (for context GNOME existed starting 1999) and has sinc switched to simpler and more modular technologies for doing the job of a comonent framework. These include D-Bus for inter-process comunication, GObject for object-oriented development, GIO for location transparency (abstracting network file locations on the file system), and GTK technology for embedding application views. D-Bus is the Desktop Bus, it was [designed specifically for communication between desktop apps, the desktop, and the operating system](https://dbus.freedesktop.org/doc/dbus-tutorial.html#uses). [The KDE graphical shell uses KParts as its component model technology.](https://techbase.kde.org/Development/Architecture/KDE3/KParts#The_KDE_Component_Framework) [KDE Frameworks](https://en.wikipedia.org/wiki/KDE_Frameworks) is based on Qt. As a result, KParts is unique in that interfaces are described not in IDL but in the Qt Meta-Object System, which is a C++ class including the [`Q_OBJECT` macro](https://doc.qt.io/qt-6/qobject.html#Q_OBJECT). KPart's use of the Qt Meta-Object System can be dynamic (evalute at run-time) and cannot be reduced to a binary interface. `KParts::Part` is the base interface class from which all other interfaces inherit. KParts splits network transparency into its own KIO component (like GNOME GIO). Indeed, GUI frameworks common to GNU/Linux were once and in some parts still are implemented in terms of components belonging to a component framework. After all, GNOME stands for GNU Network Object Model Environment, which speaks to its component model roots. The [GNOME desktop was also heavily inspired by KDE](https://mail.gnome.org/archives/gtk-list/1997-August/msg00123.html), sharing some code in the beginning. The X Windowing System and protocol is well-known for its built-in networking transparency (a common component model feature) because, throughout the 1980s and 1990s, expensive computing resources were often hosted centrally and shared between thin clients. Wayland didn't carry on the network transparency feature of X11. However, it is important to highlight that, in all cases, this complex and all encompassing component model technology was only ever used for creating [user inteface components](https://api.kde.org/frameworks/kparts/html/index.html) (justifiable by the inherently complex nature of a GUI framework). In general, Unix-like systems (including MacOS) commonly use simple but powerful [Berkeley/POSIX sockets](https://en.wikipedia.org/wiki/Berkeley_sockets) for inter-process communication.

**Todo:** Add information about the GObject portable object system. [This framework is still typically only used in GUI programs](https://en.wikipedia.org/wiki/GObject#Usage) or other inherently complex software where exposing a language-neutral object makes sense like GStreamer. Unlike COM, GObject never automatically does dynamic library loading. It is a separate thing via a [GModule](https://docs.gtk.org/gmodule/modules.html) if desired—it is not commonly used and in fact the functionality for this even exists in a separate `libgmodule` library instead of the base `libgobject` library—but unlike COM with an in-process `CoCreateInstance` always being a `LoadLibrary` call. [GCC got Itanium C++ ABI support in version 3](https://gcc.gnu.org/onlinedocs/libstdc++/manual/abi.html#:~:text=From%20GCC%20version%203%20onwards%20the%20GNU%20C%2B%2B%20compiler%20uses%20an%20industry%2Dstandard%20C%2B%2B%20ABI%2C%20the%20Itanium%20C%2B%2B%20ABI.) released in [Feburary 2002](https://gcc.gnu.org/gcc-3.0/). GObject was released by the GNOME project in [March 2002](https://en.wikipedia.org/wiki/GObject). Thing is about C++ is that there are some unweidly stuff about it like exceptions that are not easy to handle across programming languages despite the Itanium C++ ABI standardizing the ABI for those exceptions to occur. C will always be the most portable programming language and so GObject does a good job of adding a thin object-oriented layer on top of it while not having to concern one's self about some of the things C++ adds on top of just an object type system. In some ways, it could be said that GObject is what COM should have been.

### MacOS Distributed Objects and NSXPCConnection

On MacOS, component model technology exists as [distibuted objects](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/DistrObjects/Concepts/AboutDistributedObjects.html) (DO). The original design focus of [component model](https://en.wikipedia.org/wiki/Component-based_software_engineering) and [distributed object](https://en.wikipedia.org/wiki/Distributed_object) technologies varies in that the former focuses on modularity and encapsulation, whereas the latter focuses on remote object-oriented communication. However, in practice, implementations largely overlap to fulfill both purposes. Distributed objects are implemented using [NSXPCConnection](https://developer.apple.com/documentation/foundation/nsxpcconnection) in modern MacOS. A distibuted objects interface is described with an [Objective-C protocol](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/ProgrammingWithObjectiveC/WorkingwithProtocols/WorkingwithProtocols.html) making it dynamic, unlike an IDL-based component model. `NSObject` is the root object from which all other objects inherit. An `NSPort` provides location transparency. Let's do a small walk through the history of component model technology on MacOS and put this technology in context. [Cocoa](https://en.wikipedia.org/wiki/Cocoa_(API)) is a general object-oriented framework for developing native applications targeting the MacOS platform. Developing for the Cocoa framework is typically done in Objective-C or Swift; however, other language bindings also exist. Cocoa eases interaction with core MacOS frameworks, including the Foundation framework. Within the Foundation framework, there exists the legacy and deprecated [`NSConnection`](https://developer.apple.com/documentation/foundation/nsconnection) API that "forms the backbone of the distributed objects mechanism". A new API for doing IPC, [XPC was internally added to MacOS in its 10.7 Lion release (2011)](https://launchd-dev.macosforge.narkive.com/xYLsgYJR/the-machservice-key#post2). XPC streamlines inter-object and simple inter-process communication with its modular, lightweight, secure design. In MacOS 10.8 Mountain Lion (2012), [NSXPCConnection](https://developer.apple.com/documentation/foundation/nsxpcconnection) was introducted to provide inter-object communication based on the new XPC backend. [Apple superseded the `NSConnection` API for distributed objects with this new `NSXPCConnection` API](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/DistrObjects/DistrObjects.html). In MacOS 10.10 Yosemite (2014), [XPC](https://developer.apple.com/documentation/xpc) was published as its own inter-process communication mechanism. Bringing it together, on MacOS, XPC exists in two distinct forms, [XPC provided by the Foundation framework](https://developer.apple.com/documentation/foundation/xpc) for building object-oriented APIs (component model technology) and [XPC provided by libSystem](https://developer.apple.com/documentation/xpc) for performing low-level messaging (typical IPC). Note that low-level XPC documentation says it communicates in ["objects"](https://developer.apple.com/documentation/xpc/xpc_objects); however, these these objects are more like structures in that they can only store primitive data types along with some custom binary types. Moreover, a low-level XPC connection doesn't map to an instance of any object on the server-side, which is what makes it more comprable to typical IPC.

### Fun Facts

**Apple NSXPC Woes:** The dynamic nature of interfaces supported by Apple's distributed objects may be [too dynamic for its own good](https://googleprojectzero.blogspot.com/2022/03/forcedentry-sandbox-escape.html#h.fyh8k0aja0xk) (not that COM would make a good technology to communicate with a sandboxed process with, either). An interesting observation is that Apple iOS didn't publicly support [low-level XPC until iOS 17.4](https://developer.apple.com/documentation/xpc) (significantly later than MacOS), which was only released in [March 2024](https://en.wikipedia.org/wiki/IOS_17#Release_history).

[**Microsoft Aggressively Promoted COM:**](https://web.archive.org/web/19991013050302/https://microsoft.com/com/) This promotion included popular COM-based technologies at the time like MTS and ActiveX. In particular, COM on Windows NT was marketed as superior to the Unix platform.

[**Is COM Dead? (2000) by COM founder Don Box:**](https://learn.microsoft.com/en-us/archive/msdn-magazine/2000/december/house-of-com-is-com-dead#so-is-com-dead) COM was already falling out of fashion in 2000 but lives on inside of .NET (the CLR) and within Windows.

**Brief NeXTSTEP History:** The `NS` prefix on some Macintosh APIs refers [NeXT](https://en.wikipedia.org/wiki/NeXT), which is a company Apple acquired to merge the NeXTSTEP operating system into the classic Mac OS X, thus giving us the Mac OS X we know today (that's where the "X" comes from). As a result, Apple inherited lots of NeXTSTEP technology, including distributed objects.

## The Process Lifetime

In user-mode, code runs in the lifetime of a process. Within a process there are three kinds of lifetimes:

1. The application lifetime
    - **Birth:** The `main` function or process entrypoint is called, starting with constructors in the program before `main`
    - **Death:** The `main` function returns, ending with destructors in the program after `main`
2. Library subsystem lifetimes
    - **Birth:** Library initializers/constructors (e.g. Windows `DLL_PROCESS_ATTACH` or legacy Unix `_init`)
    - **Death:** Library initializers/constructors (e.g. Windows `DLL_PROCESS_DETACH` or legacy Unix `_fini`)
3. Stack lifetimes
    - **Birth:** Data is pushed onto the stack
    - **Death:** Data is popped off the stack
    - The stack is an abstract LIFO data structure, which in theory has an infinite size
        - Beyond that, its implementation can technically be anything, but on modern systems a stack is implemented by adding and subtracting to a stack pointer register
    - Also referred to as block scope or automatic storage duration

All other lifetimes occur as a result of these three lifetimes. For instance, heap allocation lifetimes inherit from these three types of lifetime because one of them must keep a reference to a given heap allocation within its scope. Thread-local data is just stack memory with the lifetime of the entire stack. Abstracting further, it could be said that the stack lifetime of the main thread is the parent of all lifetimes, especially in a single-threaded application, but also in a multithreaded application because references to threads are stored in the stack or are indirectly referenced by the stack, excluding threads remotely spawned by the outside environment. These three levels, though, define the optimal level of abstraction for defining lifetime as it relates to modern user-mode processes.

## Defining Loader and Linker Terminology

A loader and linker are two components essential for program execution or building.

As the first component to run code when a process starts, a loader is responsible for setting up a new process and bringing in dependencies of the application as they are required. Setting up a process includes tasks like initializing critical data structures, loading dependencies, and running the program. The term loader is often interchangeable with dynamic linker except that loader is also a catch all term for any operations done at process startup.

A linker can be dynamic, working at application run-time to resolves dependencies between executable modules. A linker can also work at compile-time as the next step after compilation where it is used to write information into a program or library about where a dynamic linker or loader can find its dependencies at process load-time/run-time, or to stitch executable modules together to create a runnable program.

[Dynamic linking](https://man7.org/linux/man-pages/man8/ld.so.8.html) resolves dependencies between executable modules. Dynamic linking typically occurs at process load-time; however, it can also occur later due to a lazy linking optimization.

Static linking is when a linker stitches object files together into a single executable at build-time. Linkers like the [`ld` program](https://man7.org/linux/man-pages/man1/ld.1.html) on GNU/Linux or the MSVC toolchain `link.exe` program are used as part of building. Compilers, like GCC, Clang, and MSVC, commonly invoke linkers (GCC actually includes its linker in the GCC program itself), although they can be also be used as standalone tools. Note that some Microsoft [sources](https://learn.microsoft.com/en-us/cpp/build/reference/imports-dumpbin) [conflate](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi2/nf-libloaderapi2-queryoptionaldelayloadedapi#remarks) "statically linked" DLLs with dynamically linked DLLs; however, this use of terminology is incorrect. Static is an overloaded term that generally refers to the entire lifetime of the process. As a result, a static dependency can refer to a dynamically linked dependency that exists for the lifetime of the process. However, equating static linking and dynamic linking is just wrong.

[Dynamic loading](https://github.com/bpowers/musl/blob/master/src/ldso/dlopen.c) refers to loading a library at run-time such as with `dlopen`/`LoadLibrary` or library lazy loading functionality.

[Static loading](https://learn.microsoft.com/en-us/windows/win32/dlls/dllmain#parameters) refers to loading dynamically linked dependencies that exist for the lifetime process. Static loading Microsoft-specific terminology. The term is confusing because loading is inherently a dynamic operation.

A dynamic library is a library that is compiled for use in dynamic for use in dynamic linking or loading. A static library is a library for use in static linking.

## Concurrency and Parallelism in the Loader

Concurrency is the property of a system that allows multiple tasks to make progress independently and interact with resources at overlapping times. A "running task" typically refers to a thread of execution within the process or kernel. Concurrency on its own is easy; concurrency after introducing shared resources or states can often become challenging to navigate. Protecting shared resources or states is where locks and atomic primitives become relevant in concurrency. When concurrency is managed correctly, the outcome of an operation will be the same regardless of execution order.

At its most fundamental level, the loader is a [state machine](https://en.wikipedia.org/wiki/Finite-state_machine). Threads may call upon different parts of the loader at overlapping times and when this happens it is the job of the loader to ensure each request is serviced while maintaining a consistent state to produce a consistent result.

Parallelism and concurrency are related but distinct concepts. Parallelism is about running multiple tasks simultaneously (executing at the same time). Concurrency is about managing multiple tasks at once (interleaving execution). The former requires a multi-core processor with while the latter requires only a single-core processor and works by [multitasking](https://en.wikipedia.org/wiki/Computer_multitasking) (i.e. continually starting and stopping different tasks, thereby interleaving their execution). Parallelism is needed for the efficient execution of CPU-bound workloads because making a single-core processor multitask to complete work would be slower than that same processor executing each work item to its completion sequentially due to the overhead introduced by multitasking.

The modern Windows loader can offload its [dynamic linking](#defining-loader-and-linker-terminology) or "mapping and snapping", as the Windows loader calls it, work to other willing threads. These operations are good fit for parallel processing because mapping requires making lots of slow CPU-bound system calls ([each user-mode thread corresponds to a kernel-mode thread](https://en.wikipedia.org/wiki/Thread_(computing)#1:1_(kernel-level_threading))) and snapping is a purely CPU-bound operation, not requiring any I/O, where the loader resolves import names depended on by one module to addresses in another module. It is the job of the kernel, specifically its scheduler, to delegate which core of a multicore processor to run each thread on.

Performing a task that must only happen once process-wide, such is the case with module initialization or finalization routines, means contending threads must yield to the the thread that started the given task until its sequential completion. Additionally, tasks that require a strict order of operations, such as when the loader runs module initialization or finalization routines because each module's code within these routines may depend on each other, makes concurrent or parallelized processing infeasible.

## ABBA Deadlock

An [ABBA deadlock](https://www.oreilly.com/library/view/hands-on-system-programming/9781788998475/edf57b67-a572-4202-8e56-18c85c2141e4.xhtml) is a deadlock due to lock order inversion. Whether lock order inversion results in an ABBA deadlock is probabilistic because it depends on whether at least two threads **interleave** while acquiring the locks in a different order. Agreeing on an order to acquire locks in, thereby avoiding lock order inversion, prevents ABBA deadlock. Failure to follow an agreed upon order for acquiring locks is known as a lock hierarchy violation.

A system can realize ABBA deadlock due to lock order inversion in a couple ways. First, lock order inversion can occur in the lock hierarchy of a single subsystem if it's poorly programmed or in distinct cases where there is an intentional goal of maximizing concurrency at the cost of making some subsystem operations unsafe for external code to perform at that time. Secondly, an ABBA deadlock can occur due to the more complex case whereby separate subsystems nest their respective lock hierarchies within a thread. The latter case can be tricky because there may not necessarily be a defined order for interacting with separate subsystems, each of which impose their own lock hierarchy, and when nested form one grand lock hierarchy between them.

The simplest solution to ABBA deadlock in a system is good composition: within a program or subsystem, code naturally forms a tree shape and recursion should be handled with care; between subsystems, architecural layering ensures a lower-level subsystem will be called into from higher-level subsystems in a consistent order.

Microsoft refers to an ABBA deadlock as a ["deadlock caused by lock order inversion"](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-best-practices#deadlocks-caused-by-lock-order-inversion). These are synonyms. However, I prefer the more concrete term common throughout the Linux ecosystem.

It is even possible to [ABBA deadlock in Rust](https://stackoverflow.com/a/66574315) because "fearless concurrency" only extends to memory safety and not logical issues in synchronization.

For a conceptual exploration of synchronization issues that can occur in multithreaded applications and how to resolve them, refer to the [Dining Philosophers Problem](#dining-philosophers-problem).

## ABA Problem

The [ABA problem](https://en.wikipedia.org/wiki/ABA_problem) (sometimes written as the A-B-A problem) is a low-level concept that can cause data structure inconsistency in lock-free code (i.e. code that relies entirely on atomic assembly instructions to ensure data structure consistency):

> In multithreaded computing, the ABA problem occurs during synchronization, when a location is read twice, has the same value for both reads, and the read value being the same twice is used to conclude that nothing has happened in the interim; however, another thread can execute between the two reads and change the value, do other work, then change the value back, thus fooling the first thread into thinking nothing has changed even though the second thread did work that violates that assumption.

> A common case of the ABA problem is encountered when implementing a lock-free data structure. If an item is removed from the list, **deleted**, and then a new item is allocated and added to the list, it is common for the allocated object to be at the same location as the deleted object due to MRU memory allocation. A pointer to the new item is thus often equal to a pointer to the old item, causing an ABA problem.

This description explains how atomic [compare-and-swap instructions](https://en.wikipedia.org/wiki/Compare-and-swap) (CAS, e.g. `lock cmpxchg` on x86) has the potential to mix up list items on concurrent deletion and creation because a new list item allocation at the **same address in memory** as the just deleted list item (e.g., in a linked list) could **interleave**, thus causing the ABA problem.

As a result, the risk of naively programmed lock-free code realizing the ABA problem is probabilistic. It's highly probabilistic with dynamically allocated memory, particularly when considering that modern heap allocators avoid returning the same block of memory in too close succession as a form of exploit mitigation (i.e. modern heap allocators may not do MRU memory allocation as the ABA problem Wikipedia page suggests).

The shortcoming of CAS is that it only atomically guarantees whether two pointers match. However, if for instance, a `malloc` reuses a memory address to store data about a lock-free data structure, then two pointers can match, causing CAS to believe the underlying data is unchanged, which is not always true (even when, of course, using a synchronized heap).

Here's a minimal demonstartion on how the ABA problem can manifest when using a single CAS operation to modify a singly linked list:

1. Initial state
    - Node `A` points to node `B` (i.e., `A.next` = `B`).
2. ABA scenario
    - Thread 1: Reads `A.next` and sees it points to `B`.
    - Thread 2: Removes node `B` from the list and adds a new node `C` in place of B (i.e., A.next is updated to `C`).
    - Thread 2: Later, it removes node `C` and adds node `B` back to the list (i.e., `A.next` is updated back to `B`).

3. Result
    - Thread 1 attempts to perform a CAS operation to change `A.next` to `D` assuming `A.next` is still `B`.
    - Since `A.next` has been restored to `B`, the CAS operation succeeds, even though the node that Thread 1 was expecting (`B`) may have a different underlying value

The problem is that while `A.next` equals the same **pointer value**, the **underlying value** of `A.next` (or `B`) could have been changed to something entirely different by thread 2 in the interim, thus causing the ABA problem when the CAS incorrectly succeeds. It's possible to create the [ABA problem even in safe Rust code](https://play.rust-lang.org/?version=stable&mode=debug&edition=2021).

Modern instruction set architectures (ISAs), such as AArch64 (ARM64), support [load-link/store-conditional (LL/SC)](https://en.wikipedia.org/wiki/Load-link/store-conditional) instructions, which provide a stronger synchronization primitive than compare-and-swap (CAS) instructions. LL/SC solves the ABA problem by failing the conditional store (SC) if the data at the address referenced by the LL is modified (this can be detected at the ISA-level). In other words, it includes the initial read as part of the atomic modification. LL/SC also cannot livelock (an inflite busy loop between two or more threads) because one or more LL/SC pairs failing implies another succeeded.

On older architectures (e.g. x86), one must use a workaround to create correct lock-free code that avoids the ABA problem. However, these workarounds, such as tagged pointers or hazard pointers, can be complex and are often difficult to verify the correctness of.

## Dining Philosophers Problem

The dining philosophers problem is a scenario originally devised by Dijkstra to illustrate synchronization issues that occur in concurrent algorithms and how to resolve them. [Here's the problem statement.](https://en.wikipedia.org/wiki/Dining_philosophers_problem#Problem_statement)

The simplest solution: the philosophers pick a side (e.g., the left side) and then agree always to take a fork from that side first. If a philosopher picks up the left fork and then fails to pick up the right fork because someone else is holding it, he puts the left fork back down. Otherwise, now holding both forks, he eats some spaghetti and then puts both forks back down. By the philosophers agreeing on an order to pick up forks in, they effectively create (lock) hierarchies between the left and right forks, thus preventing deadlock.

What we just described is the resource hierarchy solution. I encourage you to explore [other solutions](https://en.wikipedia.org/wiki/Dining_philosophers_problem#Solutions). Anyone who has learned about concurrency throughout their computer science program would be familiar with this classic problem.

## License

The document you just read is under a CC BY-SA License.

This repo's code is triple licensed under the MIT License, GPLv2, or GPLv3 at your choice.

Copyright (C) 2023-2025 Elliot Killick <contact@elliotkillick.com>

**Big thanks to Microsoft for successfully nerd sniping me!**

Thank you also to the people in my life who took the time to review my work, provide feedback, and ask questions that improved my writings.

EOF
