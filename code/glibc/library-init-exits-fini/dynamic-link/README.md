# Library Initialization Exits Process (Dynamic Linking)

We evaluate how the loader responds to exiting the process from a module constructor at process startup. Do module destructors run?

## Result

```
Hello from Library 1!
Hello from Library 2!
```

When Library 2 invokes process exit, no more module constructors run and none of the module destructors run. The GNU loader treats exiting the process at process startup the same as an non-graceful exit as done by `_exit`.
