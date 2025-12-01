# `dlopen` Initialization Interruption Experiment

We evaluate how the loader responds to module initialization being interrupted by a constructor in that module dynamically loading a library that holds a circular dependency on the currently initializing module.

See the [Windows equivalent](/code/windows/loadlibrary-init-circular-dependency/README.md) experiment.

## Result

```
Start: Library 1, init1_1
Start: Library 2, init2_1
Library 1, func1_1
End: Library 2, init2_1
End: Library 1, init1_1
Library 1, init1_2
Library 1, init1_3
```

The glibc loader did not go back and initialize the other constructors in library 1 before running a library 2 initializer. Technically, there is no correct behavior here because the circular dependency means taking either approach in initialization order could lead to unexpected results or crash the process. However, I would consider it "safer" for the loader to first fully initialize all the other constructors in library 1 upon calling into `dlopen` instead of initializing library 2 first. I find this approach "safer" because library 1 being fully initialized other than the one constructor calling `dlopen` seems like a probabilistically less error-prone approach than starting library 2 initialization while multiple constructors in library 1 are fully uninitialized.

Note that this discussion on the per-routine safety of module initialization interruption is only relevant to begin with because the ELF executable format distinctly exposes each routine in a module using the `.init_array` section.

**Conclusion:** The GNU loader does not utilize its knowledge of individual initialization routines to help mitigate the poor outcomes of circular dependenices on module initialization. Not trying to compensate for the potential error of some developers (because circular dependencies are always wrong), potentially at the cost of extra checks that could impact performance for everyone else, is a reasonable choice.
