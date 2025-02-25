# `LoadLibrary` Initialization Circular Dependency Experiment

We evaluate the loader's behavior when encountering a circular dependency during module initialization.

See the [glibc equivalent](/code/glibc/dlopen-init-interruption/README.md) experiment.

## Result

```
Start: DLL 1 Init
Start: DLL 2 Init
DLL 1, func
End: DLL 2 Init
End: DLL 1 Init
```

**Conclusion:** As expected, the Windows loader has no choice but to initialize DLL 2 while DLL 1 could be partially initialized due to a circular dependency. The PE executable format exposes all initialization and finalization tasks as merged into one externally callable `EntryPoint` and arbitrary code cannot be interrupted or preempted, so the Windows loader's current behavior is the best it can be under the conditions.
