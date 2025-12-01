# Library Initialization Exits Process (Dynamic Loading)

We evaluate how the loader responds to exiting the process from a module constructor from a dynamic library load. Do module destructors run?

## Result

```
Hello from Library 1!
Hello from Library 2!
Goodbye from Library 1!
This is coming from a Library 2 destructor!
Goodbye from Library 2!
```

When Library 2 invokes process exit, no more module constructors run. However, all of the module destructors do run (even though the module is partially constructed because the loader has not run all of its module constructors yet).

The GNU loader does not factor in module initialization status to whether module destructors should run if process exit occurs from within a module constructor at process run-time.
