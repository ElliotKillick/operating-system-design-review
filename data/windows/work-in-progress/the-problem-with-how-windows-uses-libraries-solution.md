# Problem Solved?

**NOTE:** Work in progress. Not in its final state. Do not take anything here as done or complete. I have plans on how to re-work this whole part. Most of this text was originally written when the project as a whole was in a much earlier state of research and development.

Identifying issues is important, but it's even more valuable to pair that with ideas for solutions. So, let's come with some actionable solutions for the root issue we explored here!

## Organize Subsystems

With Windows 7 came the introduction of [API Sets](https://www.geoffchappell.com/studies/windows/win32/apisetschema/index.htm). API Sets are an application compatibility mechanism designed as an altenative to activation contexts for finding the correctly versioned DLL to load (i.e. to help in the fight against DLL Hell). API Sets are promising because they neatly sort the Windows API into smaller and more modular units.

As it stands, an API Set is merely an alias that maps to a real DLL on disk. In this solution, we propose an API Sets extension that allows API Set DLL names become or more closely imitate real DLLs. Perhaps they could be called "virtual DLLs". By organizing with functional cohesion (the best type of cohesion) and enough granularity, the hope is that Windows DLLs would naturally lose their circular dependencies because they were using separate parts of the same DLL.

A caveat to this solution exists if the circular dependency is formed because a specific API in one "real DLL A" requires functionality from "real DLL B" while "real DLL B" also requires functionality from "real DLL A" within the scope of that API call. In this case, no level of API granularity could break the circular dependency. Before reaching per-API granularity, there could also be other practical refactoring limitations due to the underlying implementation.

In cases where increasing the granularity in the set of APIs provided by a library fails to remove circular dependencies, it may be warranted to reorganize by merging multiple libraries into one. Generally, it is always possible to remove dependency feedback loops by decoupling subsystems and employing [cycle breaking strategies](https://en.wikipedia.org/wiki/Acyclic_dependencies_principle#Cycle_breaking_strategies).

**NOTE:** The modern RPCRT4.dll library depends on WINHTTP.dll. This level of dependency carnage cannot be fixed with API Sets since WinHTTP depends on practically everything else on the operating system. RPCRT4.dll must drop the dependency (refactoring + versioning needed) or WINHTTP.DLL must be included in the base operating system libraries like NTDLL, KERNELBASE, or KERNEL32—which is insane.

## Reimplementation

With the realization that the Windows API more closely resembles a dog chasing its own tail, planets orbiting an NT kernel star, or simply a web of libraries more than it does a directed acyclic graph (DAG) of modular subsystems, reimplementing large parts of the Windows API to use a different backend becomes a viable solution.

For instance, Windows is known to use COM in its lower-level subsystems despite COM being a high-level abstraction, with tons of dependencies, that has no place in the core parts of an operating system. In places where COM cannot be used due to creating circular dependencies, Microsoft should directly use their lower-level inter-process communication frameworks such as ALPC (or maybe a simplified wrapper around ALPC since the technology has a reputation of being difficult to program for). [Apple does exactly this with the MacOS COM-equivalent of NSXPCConnection that is part of the Foundation framework and the lower-level XPC framework provided by the `libSystem` library.](https://developer.apple.com/documentation/xpc#:~:text=If%20your%20project%20doesn%E2%80%99t%20or%20can%E2%80%99t%20link%20against%20Foundation%2C%20use%20the%20lower%2Dlevel%20libSystem%20APIs%20in%20the%20XPC%20framework.)

Wine is already making progress here by making translating DirectX into Vulkan (DKVK) and Windows audio/video APIs into using GStreamer or FFmpeg. This solution also comes with other benefits such as typically improving performance and efficiency over the Microsoft Windows alternatives.

TODO: Add in how DirectX uses COM as a hack to workaround dependency loops, because if you look at DirectX dlls you will see they all loop with the only imported interface between them being the `CreateDXGIFactory`/`CreateDXGIFactory1`/`CreateDXGIFactory2` [DirectX COM factory creation function](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-createdxgifactory)

```
user@host ~/Desktop/dumpbin-imports
$ grep -rin CreateDXGIFactory
...
d3d11.dll.txt:390:                           5 CreateDXGIFactory2
D3D12Core.dll.txt:547:                                    0000000180102497     5 CreateDXGIFactory2
d3d9on12.dll.txt:161:                           5 CreateDXGIFactory2
dcomp.dll.txt:751:                                    000000018006E6E2     5 CreateDXGIFactory2
ddisplay.dll.txt:337:                           5 CreateDXGIFactory2
ddraw.dll.txt:484:                           4 CreateDXGIFactory1
...
duser.dll.txt:524:                                    0000000180016000     4 CreateDXGIFactory1
...
DXCaptureReplay.dll.txt:392:                           3 CreateDXGIFactory
DXCaptureReplay.dll.txt:393:                           5 CreateDXGIFactory2
DXCaptureReplay.dll.txt:394:                           4 CreateDXGIFactory1
dxdiagn.dll.txt:678:                           4 CreateDXGIFactory1
DXToolsOfflineAnalysis.dll.txt:31:                           3 CreateDXGIFactory
DXToolsOfflineAnalysis.dll.txt:32:                           4 CreateDXGIFactory1
...
```

## Conclusion

Solving the problem we explored here is a colossal task that requires a multifaceted resolution. Once the Windows API is organized, it will be up to Microsoft Windows developers to remain conscientious about the dependenices their subsystems create. The vastness of the Windows API does not make coming up with the best solution to its circular depenendency problem easy, but I maintain that it is possible and that application compatibility can come along for the ride.
