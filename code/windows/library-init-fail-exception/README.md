# Library Initialization Fails Due to an Exception

We evaulate what happens when an exception is thrown from a constructor in the module scope at process startup.

## Result

```
DllMain: DLL_PROCESS_ATTACH
```

After our code prints this message is when we raises an exception in the module constructor. [The loader catches the thrown exception and records it as the DLL failing module initializization.](https://github.com/reactos/reactos/blob/e326b0698385d328b96013e61ed8c63e1cc915a4/dll/ntdll/ldr/ldrinit.c#L838-L864) Since the module failing its initialization is required for process startup in this case, the loader proceeds to exit the process early. If this module was loaded outside of the application's dependency graph due to a `LoadLibrary` operation then only loading that library would fail and the application would not exit.

For the error message, a GUI dialog is presented that states:

> The application was unable to start correctly. (0xc0000142)
> Click OK to close the application.
