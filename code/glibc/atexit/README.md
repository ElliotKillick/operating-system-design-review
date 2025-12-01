# `atexit` and Concurrency

`atexit` is a call defined by the C standard for registering a function to be called on program exit.

Since the lifetime of a library may be shorter than the lifetime of a program, it is generally invalid for a library to call `atexit`: a library that is dynamically loaded at application run-time then closed will be unloaded before program exit occurs. However, the C standard does not ban the use of `atexit` from a library and so most C library implementations will support this use of `atexit` to some degree.

## Routine List Protection

glibc uses a lock made specifically for protecting shared `atexit` data called: [`__exit_funcs_lock`](https://elixir.bootlin.com/glibc/glibc-2.38/source/stdlib/exit.h#L70-L77). The shared data it protects is the `exit_function_list` data structure. The `exit_function_list` linked list is made up of `exit_function` nodes. [The `__new_exitfn` function is responsible for adding new exit functions to this list.](https://elixir.bootlin.com/glibc/glibc-2.38/source/stdlib/cxa_atexit.c#L44). glibc [unlocks this lock before calling into an `atexit` routine then relocks it after](https://elixir.bootlin.com/glibc/glibc-2.38/source/stdlib/exit.c#L87-L90).

Separating list protection from routine protection is a good concurrent property of how `atexit` is implemented on glibc because if, for instance, an `atexit` routines waits for a thread that calls `atexit` before joining back, then no deadlock will occur.

## Running Exit Routine Protection

When running `atexit` routines in the context of program exit, glibc does not hold any locks while running these routines. Therefore, the program is tasked with implicit synchronization of the exit routines since glibc does not explicitly impose any locks. As long as:

1. exit routines are used within the program scope;
2. the programmer sensibly does not keep an exit routine running while, for instance, restarting process exit on another thread without waiting for the thread; and
3. no bad libraries are hijacking the lifetime of the program by calling `exit()` thus not allowing the program to cleanup, especially if the call happens to occur from a thread other than the main thread

then this property can be part of a beneficial concurrent design that prevents deadlocks.

`atexit` should only be used for the dynamic case of registering at routine to run at application run-time. If the exit routine is known ahead of time then it is simpler to manually call it before the end of the given program's `main()` (this is okay because no other code depends on a program such is the case for a library) or use the program's module destructor.

A library should never use an `atexit` routine to cleanup or deinitialize in any way because another library could depend on the library that incorrectly used `atexit` to do some of its cleanup thus leading to issues when the depending library goes to run its module destructor and potentially calls into its dependency that is not in a usable state. So, would a library using an `atexit` routine for auditing or hooking purposes represent a legal use case of `atexit` from a library? Unfortunately no, because a concurrent library close operation by a thread in the module scope could unload a library that registered an `atexit` routine: when library close occurs, `atexit` routines created by that library will run as part of the library destruction under the protection of `dl_load_lock` by the loader; however, this fact does not matter because the program exit running of `atexit` routines is not also holding this lock. Therefore, a race condition exists whereby the library could unload due to a concurrent library close operation on one thead while another thread runs one of the library's `atexit` routines at program exit thus leading to a crash when the library is unmapped from memory.

In the case running a library's `atexit` routines due to library closure, `atexit` will also have failed to live up to its intended purpose of running a routine when the program exits. Therefore, never call `atexit` from a library because this is a lifetime violation except under one circumstance: the library increases its own reference count in the loader by calling `dlopen` on itself before calling `atexit` thereby making the library lifetime match the program lifetime.

[GDB Log Viewer](https://html-preview.github.io/?url=https://raw.githubusercontent.com/ElliotKillick/operating-system-design-review/blob/main/code/glibc/atexit/gdb-log.html)
