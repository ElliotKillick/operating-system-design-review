# Glibc Debugging Commands

These are some helpful debugging tips and commands to aid analysis.

## `link_map` Analysis

List all library `link_map` structures:

```python
python
map = int(gdb.parse_and_eval("((struct r_debug *) &_r_debug)->r_map->l_next"))
while map:
    print(gdb.parse_and_eval(f'((struct link_map *){map})->l_name'))
    map = int(gdb.parse_and_eval(f'((struct link_map *){map})->l_next'))
```

We get a list head from the `r_debug` structure (run `ptype struct r_debug` in GDB to get this structure's definition) then iterate the linear (i.e. non-circular) `link_map` list while printing the library names including their full paths (`l_name`). Notice we skip over the list head immediately by accessing `l_next` before iterating.

GDB doesn't have a `!list` macro command like WinDbg, so we use scripting. GDB supports in-line scripting with Python. This is unlike WinDbg where in-line scripting with it's choice language of JavaScript is not possible. A one-liner is not Pythonic and Python's syntax doesn't adapt well to that usage, so we use a short multi-line script (which is more readable anyways, as Python intends) that you can easily copy & paste into your GDB prompt. We choose to frequently run GDB commands with `gdb.parse_and_eval` over using the native Python functions because working with the C-style types is easier.

Example Output:

```
0x7ffff7fc7371 "linux-vdso.so.1"
0x7ffff7fc1080 "/lib64/libc.so.6"
0x400318 "/lib64/ld-linux-x86-64.so.2"
0x4052a0 "/home/user/Documents/operating-system-design-review/code/glibc/dlopen/lib1.so"
0x406610 "/home/user/Documents/operating-system-design-review/code/glibc/dlopen/lib2.so"
```

## Get TCB and Set GSCOPE Watchpoint

Read the [thread control block](https://en.wikipedia.org/wiki/Thread_control_block) on GNU/Linux (equivalent to TEB on Windows):

```
set print pretty
print *(tcbhead_t*)($fs_base)
```

Set a watchpoint on this thread's GSCOPE flag:

```
print &((tcbhead_t*)($fs_base))->gscope_flag
watch *<OUTPUT_OF_LAST_COMMAND>
```
