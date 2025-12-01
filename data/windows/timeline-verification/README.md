# Timeline Verification

Understanding history requires knowing the timeline (i.e. the order in which things happened) to get an accurate depiction of events. Let's collect some details on the files included throughout Windows versions to help establish timeline information on its development.

## Methods

### DLL Imports/Exports

Contained are the imports and/or exports of the core DLLs from these Windows version:

- Windows 3.1 `KRNL286.EXE` and `KRNL386.EXE` (16-bit and 32-bit base DLLs)
- Windows NT 3.1 early pre-release `BASE.DLL`
- Windows NT 3.1 `NTDLL.dll` and `KERNEL32.dll`

To obtain these file artifacts, see the [Legacy Software Analysis](/data/windows/legacy-software-analysis.md#operating-systems) document.

#### Windows 3.1 Files

```sh
> file KRNL286.EXE
KRNL286.EXE: MS-DOS executable, NE for MS Windows 3.x (3.0) (DLL or font)
> file KRNL386.EXE
KRNL386.EXE: MS-DOS executable, NE for MS Windows 3.x (3.0) (DLL or font)
```

16-Bit Windows executables are in the [New Executable](https://en.wikipedia.org/wiki/New_Executable) file format. Early 16-bit library executables did not support imports because Windows 3.1 did not have a dynamic linking mechanism. An EXE had to manually load another EXE, for instance by calling `LoadLibrary`, to use its functionality.

We use the `exehdr` tool to get infromation about a 16-bit executable, which can be obtained by downloading an [early version of the VC++ development tools](https://winworldpc.com/product/visual-c/1x).

#### Windows NT 3.1 Early Pre-Release Files

```sh
> file BASE.DLL
BASE.DLL: PE Unknown PE signature 0xffff8300 (Microsoft compiled help format 2.0), for MS Windows, 2 sections
```

This Windows NT pre-release used a botched [Portable Executable](https://en.wikipedia.org/wiki/Portable_Executable) file format because the new format did not have a stable ABI yet (hence why the `Microsoft compiled help format 2.0` detail is inaccurate). By the time of Windows NT's formal release, Microsoft split `BASE.DLL` into the `KERNEL32.dll` and `NTDLL.dll` files to separate the public and private Windows APIs.

There is no tool available that can parse this half-baked executable format, DLL information is obtained using the `strings` command.

#### Windows NT 3.1 Files

```sh
> file KERNEL32.DLL
KERNEL32.DLL: PE32 executable (DLL) (console) Intel 80386 (stripped to external PDB), for MS Windows, 8 sections
> file NTDLL.DLL
NTDLL.DLL: PE32 executable (DLL) (console) Intel 80386 (stripped to external PDB), for MS Windows, 10 sections
```

Execetubles in the final Windows NT release are in the [Portable Executable](https://en.wikipedia.org/wiki/Portable_Executable) file format.

We use the `dumpbin` tool to get DLL information, which can be obtained by installing Visual Studio with the C++ development pack.
