# WinHTTP `DllMain` Debugging

Let's experiment with what happens when we try doing actions at the upper bound of complexity from `DllMain`. For this purpose, we will debug what happens upon trying to use WinHTTP to make an HTTP request from the `DLL_PROCESS_ATTACH` routine of the `DllMain` function in a library.

A valid use case for using WinHTTP from a module constructor or destructor is to initiate a connection to a service at process startup or module load then send another message to gracefully end that connection at process exit or module unload. WinHTTP in the module lifetime is an unlikely requirement for general-purpose applications but could prove useful in the context of microservices. WinHTTP could also become used as a side effect like for reporting an error that occurs during initialization to the appropriate vendor.

[The experiment source code can be found on Microsoft's documentation website.](https://learn.microsoft.com/en-us/windows/win32/winhttp/winhttp-sessions-overview#downloading-resources-from-the-web)

## WinHTTP Debugging #1 (`WinHttpOpen` with the `WINHTTP_ACCESS_TYPE_DEFAULT_PROXY` flag)

Sending an HTTP request with WinHTTP fails with the [`ERROR_INVALID_STATE` Win32 error](https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes--4000-5999-#:~:text=ERROR_INVALID_STATE) following `WinHttpSendRequest`.

To work around this DLL's initialization code being unsafe to run from `DllMain`, the WebIO DLL does two-phase module initialization (sometimes also known as late initialization or lazy initialization of the module; the definition of two-phase initialization can vary across ecosystems). Two-phase initialization is an anti-pattern that comes with particularly poor outcomes when applied to the module scope. But, Windows commonly employs two-phase module initialization to workaround `DllMain` issues. In some cases, the late initialization code of a library enforces that it is not run from within `DllMain` because that would defeat the workaround. The WebIO DLL is one of these cases.

Two-phase module initialization initialization call chain: `WINHTTP!WINHTTP_DLL::GetWebIOApiHandle` ➜ `webio!WebInitialize` ➜ `webio!WapInitialize` ➜ `webio!WaIncrementDllInitializationCount` ➜ `webio!WapInitializeDll`

As we can see, safely using the WebIO DLL requires getting an API handle to it (before calling any of its other DLL exports), then the DLL reference counts its own API handles and checks for initialization, initializing if required. Essentially, the API handle mechanism works as an extension to the loader (it is a hack).

The `webio!WapInitialize` function calls `webio!WapIsLoaderLockHeld` (see the trace) to check for the status of loader lock (`ntdll!LdrpLoaderLock`), which does this check by getting loader lock from the PEB then calling `ntdll!RtlIsCriticalSectionLockedByThread` on it. If loader lock is not held, then DLL initialization proceeds with `webio!WapInitialize` calling `webio!WaIncrementDllInitializationCount` ➜ `webio!WapInitializeDll`.

Investigating the `webio!WapInitializeDll` function reveals it does the following actions which would be unsafe to perform from `DllMain`:

1. Acquires external locks
    - We can see this when `webio!WapInitializeDll` calls `webio!WaInitializeConnections` ➜ `ws2_32!WSAStartup`
      - I've confirmed by looking at the decompilation of `WSAStartup` in `ws2_32.dll` that this function broadly acquires a critical section lock in the `.data` section of `ws2_32.dll` (so, this lock is external to the WebIO DLL and the loader/NTDLL)
    - This trace in particular demonstrates the loader being forced to the bottom of a lock hierarchy by delay loading because the `WINHTTP!WINHTTP_GLOBALS::GetWebIOApiHandle` function acquires a lock (`ntdll!RtlEnterCriticalSection`), calls into the delay loaded `webio.dll` import for the first time causing that library to load (see the `ModLoad` message in the trace), then `WINHTTP!WINHTTP_GLOBALS::GetWebIOApiHandle` releases the lock after that delay load is done
2. Sets up and possibly uses a thread pool
    -  Thread pool creation call chain #1: `webio!WapInitializeDll` calls `webio!WaInitializeUtil` ➜ `webio!WaInitializeThreadPoolModule` ➜ `KERNEL32!CreateThreadpoolCleanupGroup`
    -  Thread pool creation call chain #2: `webio!WapInitializeDll` calls `webio!WaInitializeUtil` ➜ `WapInitializeTwTimerModule` ➜ `webio!WaCreateTimer` ➜ `KERNEL32!CreateThreadpoolTimer`
    - Thread pool use call chain: `webio!WapInitializeDll` calls `webio!WaInitializeUtil` ➜ `webio!WapInitializeNetworkChange` ➜ `webio!WxRegisterForNetworkChangeNotification` ➜ `webio!StartGlobalNetworkChangeMonitor` ➜ `webio!NetworkChangeMonitor::Startup` ➜ `webio!NetworkChangeMonitor::OnDemandNotificationHandler` ➜ `webio!NetworkChangeMonitor::OnChangeNotification` ➜ `webio!QueueRefCountWorkItem` ➜ `KERNEL32!QueueUserWorkItem`
    - The thread pool group of functions are known to have spikes included in them by Microsoft to inhibit their use in the context of `DllMain` (especially in a `DLL_PROCESS_DETACH` routine due to the `NtTerminteProcess` killing threads) and thread blockers would prevent new threads from spawning
    - **Note:** The WebIO DLL uses the legacy [`QueueUserWorkItem` function](https://learn.microsoft.com/en-us/windows/win32/api/threadpoollegacyapiset/nf-threadpoollegacyapiset-queueuserworkitem) to add work items to a process-wide shared Windows thread pool instead of creating its own thread pool with `CreateThreadpool`
3. May dynamically load a library with a circular dependency on the currently initializing module
    - Library load call chain: `webio!WapInitializeDll` calls `webio!WaInitializeHttpLibrary` ➜ `webio!P2pInitializeInternal` ➜ `webio!P2P_PEER_DIST_API::LoadPeerDist` ➜ `KERNEL32!LoadLibraryExW`
    - The loaded library is the DLL at `HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\PeerDist\Extension` under the `PeerdistDllName` value in the registry, which by default is set to `peerdist.dll`
    - The modern loader fully supports reentrancy but if that library has a circular dependency on a library that is partially initialized/uninitialized then use of that library which has not completed initialization can occur (remember, the Windows API is circular dependencies all the way down)

### Trace

```
0:000> wt -i ntdll -oR
   10     0 [  0] TestDLL!DllMain
   38     0 [  1]   WINHTTP!WinHttpSendRequest
   16     0 [  2]     WINHTTP!HANDLE_OBJECT::IsValid rax = 0
  134    16 [  1]   WINHTTP!WinHttpSendRequest
   16     0 [  2]     ntdll!EtwEventActivityIdControl rax = 0
  151    32 [  1]   WINHTTP!WinHttpSendRequest
   14     0 [  2]     ntdll!EtwEventActivityIdControl rax = 0
  164    46 [  1]   WINHTTP!WinHttpSendRequest
   16     0 [  2]     WINHTTP!HANDLE_OBJECT::IsValid rax = 0
  206    62 [  1]   WINHTTP!WinHttpSendRequest
   16     0 [  2]     WINHTTP!HANDLE_OBJECT::IsValid rax = 0
  239    78 [  1]   WINHTTP!WinHttpSendRequest
   13     0 [  2]     WINHTTP!HANDLE_OBJECT::IsValid
    1     0 [  3]       WINHTTP!guard_dispatch_icall_nop
    2     0 [  3]       WINHTTP!HTTP_REQUEST_HANDLE_OBJECT::GetHandleType rax = 71655248
   21     3 [  2]     WINHTTP!HANDLE_OBJECT::IsValid rax = 0
  253   102 [  1]   WINHTTP!WinHttpSendRequest
   50     0 [  2]     WINHTTP!HTTP_REQUEST_HANDLE_OBJECT::InitBeginSendRequest
   10     0 [  3]       ntdll!RtlEnterCriticalSection rax = 0
   60    10 [  2]     WINHTTP!HTTP_REQUEST_HANDLE_OBJECT::InitBeginSendRequest
   19     0 [  3]       ntdll!RtlLeaveCriticalSection rax = 0
   75    29 [  2]     WINHTTP!HTTP_REQUEST_HANDLE_OBJECT::InitBeginSendRequest
   17     0 [  3]       WINHTTP!CWebIORequestHeadersShim::SetHeader
   10     0 [  4]         ntdll!RtlEnterCriticalSection rax = 0
   20    10 [  3]       WINHTTP!CWebIORequestHeadersShim::SetHeader
   11     0 [  4]         ntdll!RtlEnterCriticalSection rax = 0
   43    21 [  3]       WINHTTP!CWebIORequestHeadersShim::SetHeader
    1     0 [  4]         WINHTTP!memset
   14     0 [  4]         ucrtbase!memset rax = 0
   47    36 [  3]       WINHTTP!CWebIORequestHeadersShim::SetHeader
   13     0 [  4]         ntdll!RtlLeaveCriticalSection rax = 0
   54    49 [  3]       WINHTTP!CWebIORequestHeadersShim::SetHeader
    1     0 [  4]         WINHTTP!operator new[]
    4     0 [  4]         WINHTTP!operator new
   22     0 [  4]         ntdll!RtlAllocateHeap
   95     0 [  4]         ntdll!RtlpAllocateHeapInternal rax = 000002e0`e9814220
   65   171 [  3]       WINHTTP!CWebIORequestHeadersShim::SetHeader
    1     0 [  4]         WINHTTP!memcpy
   14     0 [  4]         ucrtbase!memcpy rax = 000002e0`e9814220
   71   186 [  3]       WINHTTP!CWebIORequestHeadersShim::SetHeader
    1     0 [  4]         WINHTTP!memcpy
   12     0 [  4]         ucrtbase!memcpy rax = 000002e0`e981422b
   78   199 [  3]       WINHTTP!CWebIORequestHeadersShim::SetHeader
   16     0 [  4]         WINHTTP!CWebIORequestHeadersShim::_ExpandHeaderBuffer
   18     0 [  5]           WINHTTP!CArray<_WEB_HTTP_HEADER,unsigned long>::SetSize
    1     0 [  6]             WINHTTP!operator new[]
    4     0 [  6]             WINHTTP!operator new
   22     0 [  6]             ntdll!RtlAllocateHeap
   95     0 [  6]             ntdll!RtlpAllocateHeapInternal rax = 000002e0`e98205f0
   29   122 [  5]           WINHTTP!CArray<_WEB_HTTP_HEADER,unsigned long>::SetSize
    1     0 [  6]             WINHTTP!memcpy
   10     0 [  6]             ucrtbase!memcpy rax = 000002e0`e98205f0
   39   133 [  5]           WINHTTP!CArray<_WEB_HTTP_HEADER,unsigned long>::SetSize
    1     0 [  6]             WINHTTP!memset
   12     0 [  6]             ucrtbase!memset
  263     0 [  6]             ucrtbase!memset_repmovs rax = 000002e0`e98205f0
   50   409 [  5]           WINHTTP!CArray<_WEB_HTTP_HEADER,unsigned long>::SetSize rax = 0
   30   459 [  4]         WINHTTP!CWebIORequestHeadersShim::_ExpandHeaderBuffer
    1     0 [  5]           WINHTTP!memset
   12     0 [  5]           ucrtbase!memset
  263     0 [  5]           ucrtbase!memset_repmovs rax = 000002e0`e98205f0
   38   735 [  4]         WINHTTP!CWebIORequestHeadersShim::_ExpandHeaderBuffer rax = 0
   93   972 [  3]       WINHTTP!CWebIORequestHeadersShim::SetHeader
   19     0 [  4]         ntdll!RtlLeaveCriticalSection rax = 0
  106   991 [  3]       WINHTTP!CWebIORequestHeadersShim::SetHeader rax = 0
   92  1126 [  2]     WINHTTP!HTTP_REQUEST_HANDLE_OBJECT::InitBeginSendRequest
   10     0 [  3]       ntdll!RtlEnterCriticalSection rax = 0
  148  1136 [  2]     WINHTTP!HTTP_REQUEST_HANDLE_OBJECT::InitBeginSendRequest
   19     0 [  3]       ntdll!RtlLeaveCriticalSection rax = 0
  234  1155 [  2]     WINHTTP!HTTP_REQUEST_HANDLE_OBJECT::InitBeginSendRequest
   32     0 [  3]       WINHTTP!HTTP_REQUEST_HANDLE_OBJECT::QueryInfo
   10     0 [  4]         ntdll!RtlEnterCriticalSection rax = 0
   41    10 [  3]       WINHTTP!HTTP_REQUEST_HANDLE_OBJECT::QueryInfo
    1     0 [  4]         WINHTTP!guard_dispatch_icall_nop
    7     0 [  4]         WINHTTP!HTTP_BASE_OBJECT::Reference rax = 000002e0`e9824d80
   43    18 [  3]       WINHTTP!HTTP_REQUEST_HANDLE_OBJECT::QueryInfo
   19     0 [  4]         ntdll!RtlLeaveCriticalSection rax = 0
   68    37 [  3]       WINHTTP!HTTP_REQUEST_HANDLE_OBJECT::QueryInfo
   10     0 [  4]         ntdll!RtlEnterCriticalSection rax = 0
  124    47 [  3]       WINHTTP!HTTP_REQUEST_HANDLE_OBJECT::QueryInfo
   19     0 [  4]         ntdll!RtlLeaveCriticalSection rax = 0
  131    66 [  3]       WINHTTP!HTTP_REQUEST_HANDLE_OBJECT::QueryInfo
    1     0 [  4]         WINHTTP!guard_dispatch_icall_nop
   10     0 [  4]         WINHTTP!HTTP_BASE_OBJECT::Dereference rax = 1
  135    77 [  3]       WINHTTP!HTTP_REQUEST_HANDLE_OBJECT::QueryInfo
    6     0 [  4]         WINHTTP!_security_check_cookie rax = 2f76
  145    83 [  3]       WINHTTP!HTTP_REQUEST_HANDLE_OBJECT::QueryInfo rax = 2f76
  265  1383 [  2]     WINHTTP!HTTP_REQUEST_HANDLE_OBJECT::InitBeginSendRequest
    6     0 [  3]       WINHTTP!_security_check_cookie rax = 0
  269  1389 [  2]     WINHTTP!HTTP_REQUEST_HANDLE_OBJECT::InitBeginSendRequest rax = 0
  260  1760 [  1]   WINHTTP!WinHttpSendRequest
   10     0 [  2]     ntdll!RtlEnterCriticalSection rax = 0
  269  1770 [  1]   WINHTTP!WinHttpSendRequest
    1     0 [  2]     WINHTTP!guard_dispatch_icall_nop
    7     0 [  2]     WINHTTP!HTTP_BASE_OBJECT::Reference rax = 000002e0`e9824d80
  271  1778 [  1]   WINHTTP!WinHttpSendRequest
   19     0 [  2]     ntdll!RtlLeaveCriticalSection rax = 0
  280  1797 [  1]   WINHTTP!WinHttpSendRequest
   16     0 [  2]     WINHTTP!HANDLE_OBJECT::IsValid rax = 0
  307  1813 [  1]   WINHTTP!WinHttpSendRequest
    1     0 [  2]     WINHTTP!guard_dispatch_icall_nop
    2     0 [  2]     WINHTTP!INTERNET_CONNECT_HANDLE_OBJECT::GetHandleType rax = 6e6f4348
  315  1816 [  1]   WINHTTP!WinHttpSendRequest
    9     0 [  2]     WINHTTP!WINHTTP_GLOBALS::GetWebIOApiHandle
   10     0 [  3]       ntdll!RtlEnterCriticalSection rax = 0
   17    10 [  2]     WINHTTP!WINHTTP_GLOBALS::GetWebIOApiHandle
   14     0 [  3]       WINHTTP!WINHTTP_DLL::GetWebIOApiHandle
    2     0 [  4]         WINHTTP!_imp_load_WebInitialize
   12     0 [  4]         WINHTTP!_tailMerge_webio_dll
    9     0 [  5]           WINHTTP!_delayLoadHelper2
    1     0 [  6]             KERNELBASE!ResolveDelayLoadedAPI
ModLoad: 00007ff8`c9a50000 00007ff8`c9ae8000   C:\Windows\SYSTEM32\webio.dll
   66     0 [  6]             ntdll!LdrResolveDelayLoadedAPI rax = 00007ff8`c9a56d00
   12    67 [  5]           WINHTTP!_delayLoadHelper2 rax = 00007ff8`c9a56d00
   23    79 [  4]         WINHTTP!_tailMerge_webio_dll
    3     0 [  4]         webio!WebInitialize
   17     0 [  5]           webio!WapInitialize
    4     0 [  6]             webio!WapIsLoaderLockHeld
    6     0 [  7]               ntdll!RtlIsCriticalSectionLockedByThread rax = 1
    9     6 [  6]             webio!WapIsLoaderLockHeld rax = 1
   24    15 [  5]           webio!WapInitialize
    6     0 [  6]             webio!_security_check_cookie rax = 139f
   29    21 [  5]           webio!WapInitialize rax = 139f
    7    50 [  4]         webio!WebInitialize rax = 139f
   23   161 [  3]       WINHTTP!WINHTTP_DLL::GetWebIOApiHandle rax = 139f
   22   194 [  2]     WINHTTP!WINHTTP_GLOBALS::GetWebIOApiHandle
   19     0 [  3]       ntdll!RtlLeaveCriticalSection rax = 0
   30   213 [  2]     WINHTTP!WINHTTP_GLOBALS::GetWebIOApiHandle rax = 139f
  325  2059 [  1]   WINHTTP!WinHttpSendRequest
    1     0 [  2]     WINHTTP!guard_dispatch_icall_nop
   10     0 [  2]     WINHTTP!HTTP_BASE_OBJECT::Dereference rax = 1
  345  2070 [  1]   WINHTTP!WinHttpSendRequest
   14     0 [  2]     ntdll!EtwEventActivityIdControl rax = 0
  351  2084 [  1]   WINHTTP!WinHttpSendRequest
   23     0 [  2]     ntdll!RtlSetLastWin32Error rax = 139f
  356  2107 [  1]   WINHTTP!WinHttpSendRequest
    6     0 [  2]     WINHTTP!_security_check_cookie rax = 0
  366  2113 [  1]   WINHTTP!WinHttpSendRequest rax = 0
   13  2479 [  0] TestDLL!DllMain
    1     0 [  1]   KERNEL32!GetLastErrorStub
    3     0 [  1]   KERNELBASE!GetLastError rax = 139f
0:000> !error 139f 0
Error code: (Win32) 0x139f (5023) - The group or resource is not in the correct state to perform the requested operation.
```

## WinHTTP Debugging #2 (`WinHttpOpen` with the `WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY` flag)

Calling `WinHttpOpen` with the `WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY` access type flag instead of the [`WINHTTP_ACCESS_TYPE_DEFAULT_PROXY` flag](https://learn.microsoft.com/en-us/windows/win32/api/winhttp/nf-winhttp-winhttpopen#parameters) (deprecated in favor of the former flag starting with Windows 8.1) causes the following deadlock before `WinHttpSendRequest` is ever called:

```
0:000> k
 # Child-SP          RetAddr               Call Site
00 000000ef`3ccfea58 00007ff8`d1b2920e     ntdll!NtWaitForSingleObject+0x14
01 000000ef`3ccfea60 00007ff8`cc8216ef     KERNELBASE!WaitForSingleObjectEx+0x8e
02 000000ef`3ccfeb00 00007ff8`cc8214f0     WINNSI!NsiRpcRegisterChangeNotificationEx+0x1ef
03 000000ef`3ccfec70 00007ff8`d0c42553     WINNSI!NsiRpcRegisterChangeNotification+0x60
04 000000ef`3ccfecf0 00007ff8`d0c42470     IPHLPAPI!InternalRegisterChangeNotification+0xa3
05 000000ef`3ccfed50 00007ff8`cc323d72     IPHLPAPI!NotifyIpInterfaceChange+0xa0
06 000000ef`3ccfed80 00007ff8`cc32449b     WINHTTP!NetworkChangeMonitor::Startup+0x7e
07 000000ef`3ccfedf0 00007ff8`cc323c5e     WINHTTP!StartGlobalNetworkChangeMonitor+0x113
08 000000ef`3ccfee30 00007ff8`cc32c915     WINHTTP!WxRegisterForNetworkChangeNotification+0x3e
09 000000ef`3ccfee70 00007ff8`cc32d8e4     WINHTTP!InitializeNetworkChangeMonitor+0x179
0a 000000ef`3ccfeee0 00007ff8`cc33e0a8     WINHTTP!INTERNET_SESSION_HANDLE_OBJECT::LoadAutomaticProxyResolvers+0xb4
0b 000000ef`3ccff010 00007ff8`cc34514b     WINHTTP!INTERNET_SESSION_HANDLE_OBJECT::SetProxySettings+0x88
0c 000000ef`3ccff040 00007ff8`cc34227c     WINHTTP!WinHttpSetOptionInternal+0xc8b
0d 000000ef`3ccff140 00007ff8`c77f10c2     WINHTTP!WinHttpOpen+0x41c
0e (Inline Function) --------`--------     TestDLL!myWinHTTP+0x20 [C:\Users\user\source\repos\TestDLL\TestDLL\dllmain.cpp @ 144]
0f 000000ef`3ccff4a0 00007ff8`c77f144b     TestDLL!DllMain+0x42 [C:\Users\user\source\repos\TestDLL\TestDLL\dllmain.cpp @ 272]
10 000000ef`3ccff4f0 00007ff8`d4209a1d     TestDLL!dllmain_dispatch+0x8f [d:\a01\_work\20\s\src\vctools\crt\vcstartup\src\startup\dll_dllmain.cpp @ 281]
11 000000ef`3ccff550 00007ff8`d425d307     ntdll!LdrpCallInitRoutine+0x61
12 000000ef`3ccff5c0 00007ff8`d425d09a     ntdll!LdrpInitializeNode+0x1d3
13 000000ef`3ccff710 00007ff8`d422d947     ntdll!LdrpInitializeGraphRecurse+0x42
14 000000ef`3ccff750 00007ff8`d420fbae     ntdll!LdrpPrepareModuleForExecution+0xbf
15 000000ef`3ccff790 00007ff8`d42073e4     ntdll!LdrpLoadDllInternal+0x19a
16 000000ef`3ccff810 00007ff8`d4206af4     ntdll!LdrpLoadDll+0xa8
17 000000ef`3ccff9c0 00007ff8`d1b32612     ntdll!LdrLoadDll+0xe4
18 000000ef`3ccffab0 00007ff6`a67c1012     KERNELBASE!LoadLibraryExW+0x162
19 000000ef`3ccffb20 00007ff6`a67c1240     TestProject!main+0x12 [C:\Users\user\source\repos\TestProject\TestProject\source.c @ 174]
1a (Inline Function) --------`--------     TestProject!invoke_main+0x22 [d:\a01\_work\20\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl @ 78]
1b 000000ef`3ccffb50 00007ff8`d2ab7374     TestProject!__scrt_common_main_seh+0x10c [d:\a01\_work\20\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl @ 288]
1c 000000ef`3ccffb90 00007ff8`d423cc91     KERNEL32!BaseThreadInitThunk+0x14
1d 000000ef`3ccffbc0 00000000`00000000     ntdll!RtlUserThreadStart+0x21
0:000> r @$argreg
$argreg=0000000000000140
0:000> !handle 140 ff
Handle 140
  Type         	Event
  Attributes   	0
  GrantedAccess	0x1f0003:
         Delete,ReadControl,WriteDac,WriteOwner,Synch
         QueryState,ModifyState
  HandleCount  	2
  PointerCount 	65537
  Name         	<none>
  Object Specific Information
    Event Type Auto Reset
    Event is Waiting
```

Further diagnosing with a trace shows that this event is created by an earlier call to `WINNSI!NsiRpcRegisterChangeNotificationEx`:

```
   67   852 [ 11]                       WINNSI!NsiRpcRegisterChangeNotificationEx
   29     0 [ 12]                         KERNELBASE!CreateEventW
    6     0 [ 13]                           ntdll!NtCreateEvent rax = 0
>> More than one level popped 12 -> 12
   36     6 [ 12]                         KERNELBASE!CreateEventW
   17     0 [ 13]                           ntdll!RtlSetLastWin32Error rax = 000000e3`97edc000
   47    23 [ 12]                         KERNELBASE!CreateEventW rax = 140
```

The `WINNSI!NsiRpcRegisterChangeNotificationEx` function then does RPC that creates a thread pool:

```
  119  1432 [ 27]                                                       RPCRT4!LRPC_CASSOCIATION::Connect
   11     0 [ 28]                                                         RPCRT4!LRPC_CASSOCIATION::EnableAsync
   10     0 [ 29]                                                           ntdll!RtlEnterCriticalSection rax = 0
   22    10 [ 28]                                                         RPCRT4!LRPC_CASSOCIATION::EnableAsync
   11     0 [ 29]                                                           RPCRT4!RPC_THREAD_POOL::CreateAlpc
   23     0 [ 30]                                                             RPCRT4!RPC_THREAD_POOL::InitializeCallbackEnvironmentIfNecessary
   11     0 [ 31]                                                               RPCRT4!AllocWrapper
   22     0 [ 32]                                                                 ntdll!RtlAllocateHeap
   95     0 [ 32]                                                                 ntdll!RtlpAllocateHeapInternal rax = 00000208`730f4dc0
   21   117 [ 31]                                                               RPCRT4!AllocWrapper rax = 00000208`730f4dc0
   38   138 [ 30]                                                             RPCRT4!RPC_THREAD_POOL::InitializeCallbackEnvironmentIfNecessary
    4     0 [ 31]                                                               KERNELBASE!CreateThreadpool
   12     0 [ 32]                                                                 ntdll!TpAllocPool
  300     0 [ 32]                                                                 ntdll!TpAllocPoolInternal rax = 0
   10   312 [ 31]                                                               KERNELBASE!CreateThreadpool rax = 00000208`731065b0
```

Two worker threads (`ntdll!TppWorkerThread`) are spawned into the process for the thread pool (I have confirmed these are not loader worker threads). The call to `WINNSI!NsiRpcRegisterChangeNotificationEx` does not deadlock us the first time but when `WINNSI!NsiRpcRegisterChangeNotificationEx` is later called again, the Windows API internally expects for any prior calls to be done already. However, the prior call to `WINNSI!NsiRpcRegisterChangeNotificationEx` remains incomplete because attempting to run `DLL_THREAD_ATTACH` routines on the new threads is blocked by our spawning thread waiting for those threads from the `DLL_PROCESS_ATTACH` rotuine of `DllMain`. In conclusion, this is a convoluted deadlock due to waiting on a thread from `DllMain` (which as we know, does not work on Windows).
