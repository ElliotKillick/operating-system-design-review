# `dlopen` Thread Join `dlopen` Experiment

Spawning a thread from a module initializer, waiting on the thread to exit, meanwhile loading another library in the new thread.

This experiment is our control. It will predictably deadlock because the first `dlopen` function already holds `dl_load_lock` for the thread, then we create a new thread that tries to run `dlopen` necessitating `dl_load_lock` for that thread, and wait for the new thread to exit from the first thread thus causing a deadlock.

Unix architecture does not heavily rely on dynamic library library or multithreading though, so in practice this deadlock scenario does not typically occur. [Some libc distributions such as musl even purposely do not support dynamic library loading.](https://github.com/bpowers/musl/blob/master/src/ldso/dlopen.c)

The only way a loader, while still supporting dynamic library loading, could avoid deadlocking in this scenario would be to increase the synchronization granularity of the loader's module initialization phase.

**Note:** While it is true that library 1 depends on library 2 because library 1 dynamically loads library 2 at run-time, having the initialization of library 2 run while library 1 is not fully initialized it self it doable because library 2 initialization only becomes a prerequisite for library 1 initialization to continue when it calls `dlopen` on library 2 from another thread.
