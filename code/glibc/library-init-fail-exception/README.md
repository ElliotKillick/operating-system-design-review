# Library Initialization Fails Due to an Exception

We evaulate what happens when an exception is thrown from a constructor in the module scope at process startup.

## Result

On glibc, throwing an exception will check for registered exception handlers on the thread. If none exists, POSIX abort signal is sent to terminate the process. The glibc loader never attempts to catch the exception from a module constructor.

```
$ ./main
Hello from Library 1!
terminate called after throwing an instance of 'std::bad_alloc'
  what():  std::bad_alloc
Aborted (core dumped)
```

```
$ gdb ./main
Reading symbols from ./main...
(gdb) run
Starting program: /home/user/Documents/operating-system-design-review/code/glibc/library-init-fail-exception/main 

This GDB supports auto-downloading debuginfo from the following URLs:
  <https://debuginfod.fedoraproject.org/>
Enable debuginfod for this session? (y or [n]) y
Debuginfod has been enabled.
To make this setting permanent, add 'set debuginfod enabled on' to .gdbinit.
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib64/libthread_db.so.1".
Hello from Library 1!
terminate called after throwing an instance of 'std::bad_alloc'
  what():  std::bad_alloc

Program received signal SIGABRT, Aborted.
__pthread_kill_implementation (threadid=<optimized out>, signo=signo@entry=6, no_tid=no_tid@entry=0)
    at pthread_kill.c:44
44	      return INTERNAL_SYSCALL_ERROR_P (ret) ? INTERNAL_SYSCALL_ERRNO (ret) : 0;
(gdb) backtrace
#0  __pthread_kill_implementation (threadid=<optimized out>, signo=signo@entry=6, no_tid=no_tid@entry=0)
    at pthread_kill.c:44
#1  0x00007ffff7aae8a3 in __pthread_kill_internal (signo=6, threadid=<optimized out>) at pthread_kill.c:78
#2  0x00007ffff7a5c8ee in __GI_raise (sig=sig@entry=6) at ../sysdeps/posix/raise.c:26
#3  0x00007ffff7a448ff in __GI_abort () at abort.c:79
#4  0x00007ffff7ca4d39 in __gnu_cxx::__verbose_terminate_handler ()
    at ../../../../libstdc++-v3/libsupc++/vterminate.cc:95
#5  0x00007ffff7cb4f6c in __cxxabiv1::__terminate (handler=<optimized out>)
    at ../../../../libstdc++-v3/libsupc++/eh_terminate.cc:48
#6  0x00007ffff7cb4fd7 in std::terminate () at ../../../../libstdc++-v3/libsupc++/eh_terminate.cc:58
#7  0x00007ffff7cb5238 in __cxxabiv1::__cxa_throw (obj=<optimized out>, 
    tinfo=0x7ffff7e43a28 <typeinfo for std::bad_alloc>, dest=0x7ffff7cb3600 <std::bad_alloc::~bad_alloc()>)
    at ../../../../libstdc++-v3/libsupc++/eh_throw.cc:98
#8  0x00007ffff7fbd192 in hello1 () at lib1.cpp:10
#9  0x00007ffff7fce237 in call_init (env=0x7fffffffdf08, argv=0x7fffffffdef8, argc=1, l=<optimized out>)
    at dl-init.c:74
#10 call_init (l=<optimized out>, argc=1, argv=0x7fffffffdef8, env=0x7fffffffdf08) at dl-init.c:26
#11 0x00007ffff7fce32d in _dl_init (main_map=0x7ffff7ffe2c0, argc=1, argv=0x7fffffffdef8, env=0x7fffffffdf08)
    at dl-init.c:121
#12 0x00007ffff7fe4bf0 in _dl_start_user () from /lib64/ld-linux-x86-64.so.2
#13 0x0000000000000001 in ?? ()
#14 0x00007fffffffe1d2 in ?? ()
#15 0x0000000000000000 in ?? ()
(gdb) exit
A debugging session is active.

	Inferior 1 [process 126976] will be killed.

Quit anyway? (y or n) y

```
