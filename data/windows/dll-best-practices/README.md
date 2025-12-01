# *Best Practices for Creating DLLs* Whitepaper from Microsoft Corporation

The *Best Practices for Creating DLLs* whitepaper is the Microsoft Corporation's original guidance laying out DLL best practices for the Windows platform, namely regarding `DllMain`.

The document was authored by engineers who were assigned to work on the DLL loader at Microsoft and published in cooperation with the [program manager (PM)](https://medium.com/@adiagashe/microsoft-program-manager-overview-everything-you-need-to-know-from-application-to-interview-33eab7fb0dde) [who owned the DLL loader](https://learn.microsoft.com/en-us/archive/blogs/larryosterman/best-practices-for-dllmain) ([blog announcement #2](https://learn.microsoft.com/en-us/archive/blogs/matt_pietrek/finally-some-more-concrete-guidance-on-the-mysterys-of-dllmain)).

A notable fact about the whitepaper's top reference to blog entries in *The Old New Thing*: Author Raymond Chen [did not approve of his work being cited for this whitepaper](https://devblogs.microsoft.com/oldnewthing/20070810-00/?p=25623).

**Note:** This old document states the following about loader lock: "Any function that must read or modify the per-process library-loader data structures must acquire this lock before performing such an operation" (in addition to protecting module initialization/deinitialization). This fact was only true of the legacy Windows loader (e.g. see ReactOS code based on the reverse engineered Windows Server 2003 loader). The modern Windows loader protects library-loader data structures using the `LdrpModuleDatatableLock` lock. The age of this document also makes it inapplicable to the modern Windows loader in other ways (this amendment is by no means comprehensive).

## Whitepaper Sources

- [Official whitepaper download (original release)](https://download.microsoft.com/download/a/f/7/af7777e5-7dcd-4800-8a0a-b18336565f5b/DLL_bestprac.doc)
- [Official whitepaper web view (original release)](https://view.officeapps.live.com/op/view.aspx?src=http%3A%2F%2Fdownload.microsoft.com%2Fdownload%2Fa%2Ff%2F7%2Faf7777e5-7dcd-4800-8a0a-b18336565f5b%2FDLL_bestprac.doc)
- [Legacy download page archived](https://web.archive.org/web/20101029013644/http://www.microsoft.com/whdc/driver/kernel/DLL_bestprac.mspx)
- [Current webpage adaptation of whitepaper](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-best-practices)