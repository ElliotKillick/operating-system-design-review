# Remote Process Termination

Let's see how killing a process works on the Windows platform.

On Windows, applications are distinguished at the kernel level as either being a Windows subsystem application or console subsystem application. A Windows subsystem application spawns with no visible window and can be a background process or graphical user interface (GUI) application. A console subsystem application always spawns with a visible command prompt or terminal window. The `taskkill` program implements the canonical way of killing a process on Windows. How the `taskkill` program gracefully kills a process (i.e. without the `/f` option) works differently for each application type.

For a Windows subsystem application, [`taskkill` works by sending a `WM_CLOSE` message to all top-level windows owned by the application.](https://github.com/wine-mirror/wine/blob/fb6b1c91bbf412f5eba260ae52ee38176fd693d4/programs/taskkill/taskkill.c#L119-L134) This action assumes the application has a [Windows message loop](https://en.wikipedia.org/wiki/Message_loop_in_Microsoft_Windows), is listening to it, and of course that it will respond correctly to this polite request for it to terminate by gracefully exiting.

For a console subsystem application, `taskkill` will trigger the system ([specifically the CSRSS component of the system](https://reactos.org/wiki/Remove_CSRSS)) to send a [`CTRL+CLOSE` control signal](https://learn.microsoft.com/en-us/windows/console/ctrl-close-signal) to the application, spawning a new thread that causes the application to invoke the default handler (`KERNELBASE!DefaultHandler`), if no custom handler is specified with `SetConsoleCtrlHandler`, which invokes process exit (`ntdll!RtlExitUserProcess`). However, this signal will only be sent if the target application is in the same [process group](https://learn.microsoft.com/en-us/windows/win32/procthread/process-creation-flags#:~:text=CREATE_NEW_PROCESS_GROUP,-0x00000200) as the process issuing the `taskkill`. If interfacing with the Windows API then the [`GenerateConsoleCtrlEvent`](https://learn.microsoft.com/en-us/windows/console/generateconsolectrlevent) allows a programmer to specify a process group to send a `CTRL+CLOSE` signal to. For users though, the `taskkill` program can only gracefully kill console applications in the same process group as the one `taskkill` is in.

## Examples

### Windows Subsystem

On a modern Windows release, open the Calculator application and try to kill the process using `taskkill` (**note:** the underlying Calculator executable file name has changed between Windows versions a number of times from `calc.exe` to `Calculator.exe` to `CalculatorApp.exe`, so use `tasklist` or check Task Manager for the correct image name):

```cmd
C:\Users\user>taskkill /im CalculatorApp.exe
SUCCESS: Sent termination signal to the process "CalculatorApp.exe" with PID 7076.

C:\Users\user>taskkill /im CalculatorApp.exe
SUCCESS: Sent termination signal to the process "CalculatorApp.exe" with PID 7076.

C:\Users\user>taskkill /im CalculatorApp.exe
SUCCESS: Sent termination signal to the process "CalculatorApp.exe" with PID 7076.

C:\Users\user>taskkill /im CalculatorApp.exe
SUCCESS: Sent termination signal to the process "CalculatorApp.exe" with PID 7076.

C:\Users\user>taskkill /im CalculatorApp.exe
ERROR: The process "CalculatorApp.exe" with PID 7076 could not be terminated.
Reason: This process can only be terminated forcefully (with /F option).

C:\Users\user>taskkill /im CalculatorApp.exe
ERROR: The process "CalculatorApp.exe" with PID 7076 could not be terminated.
Reason: This process can only be terminated forcefully (with /F option).

C:\Users\user>taskkill /im CalculatorApp.exe /f
SUCCESS: The process "CalculatorApp.exe" with PID 7076 has been terminated.
```

[The modern Windows calculator is a UWP application using the XAML UI framework](https://github.com/microsoft/calculator/blob/main/docs/ApplicationArchitecture.md). Internally though, it still uses the tradtional Windows message loop calling functions like `USER32!DispatchMessage` and `USER32!PeekMessage` from `Windows_UI_Xaml!DirectUI::DXamlCore::RunMessageLoop` (confirmed via debugging). So, the calculator application being unresponsive to `WM_CLOSE` could be due to the UI framework it depends on not having implemented handling for the message, at least not in the framework version currently used by the application ([possible](https://github.com/microsoft/microsoft-ui-xaml/issues/8492#issuecomment-1963220757)). Thus, forceful termination is the only option for killing this process.

For non-GUI applications using the Windows subsystem such as background processes, there is no way for them to handle an exit request without integrating the traditional Windows message loop into the given application (which could easily be undesirable if the application wants to do asynchronous communication with the RPCRT4 framework introduced in Windows NT, is a web server application that listens on a port, uses a custom communication framework, etc.) or by running a dedicated thread just for receving `WM_CLOSE` messages then synchronizing with the main event loop thread to close the application.

### Console Subsytem

Starting a program that will reside in the same process group as the `taskkill` process means graceful termination works as expected:

```cmd
C:\Users\user>start "" conhost.exe choice.exe

C:\Users\user>taskkill /im choice.exe
SUCCESS: Sent termination signal to the process "choice.exe" with PID 3844.

C:\Users\user>taskkill /im choice.exe
ERROR: The process "choice.exe" not found.
```

`choice.exe` got a new thread spawned into it which exited the process. The entrypoint of the spwaned thread is `KERNELBASE!CtrlRoutine` which gets passed a first argument of [`CTRL_CLOSE_EVENT` (2)](https://learn.microsoft.com/en-us/windows/console/handlerroutine#parameters). The `choice.exe` program does set a custom control handler, but its only action is to call the C runtime (CRT) `exit` function, anyway (so this way in Windows, any `atexit` routines registered by the program will be called).

**Note:** Specifying `conhost.exe` before the new application is necessary starting with Windows 11 because `cmd.exe` now redirects to the new Terminal application. This Terminal application gets started in a new process group than the console process spawning it. So, we explicitly specify `conhost.exe` to spawn a classic `conhost.exe` CMD window instead of the fancy Terminal application. `conhost.exe` is the Windows subsytem application that creates the console window for hosting the console whereas `cmd.exe` is the console client that interacts with the operating system console server. `conhost.exe` was introduced with Windows Vista. Before then, CSRSS (`csrss.exe`) owned the per-console window that handled user interaction. The core console server still runs out of CSRSS in modern Windows.

## Conclusion

Gracefully requesting an application to shut down on Win32 is cumbersome and no generic mechanism for signalling this intention among others was ever brought to Windows NT.

The lack of a generic and reliable mechanism for signalling a process to exit on the Windows platform comes with some [notable consequences](https://www.betaarchive.com/wiki/index.php?title=Microsoft_KB_Archive/178893).

Acting on signals recieved at an arbitrary time is inherently a non-trivial problem. Where a Windows subsystem application would typically require multithreading and reading the Windows message loop just to handle an external event like a request for application shutdown (which in practice almost nobody does); an application on a Unix-like system can have its execution interrupted by a signal such as `TERM`—including system call interruption if execution is waiting in a system call—to atomically set a "stop" flag that can safely be checked and acted upon in an event loop or atomically write a "stop" message into an event loop message queue. In both cases, acting on the signal (e.g. exiting the process) safely occurs at a known time. An important distinction is that the former signalling mechanism works in user-mode by leveraging the inter-process communication framework within USER32 whereas the latter works in kernel-mode by interrupting a system call and restarting the system call (`SA_RESTART`) in case it turns out the application wants to ignore the signal it received. The Unix approach is more minimal and avoids coupling to a specific IPC framework thus making it a truely generic signalling mechanism that can mold to fit any application.
