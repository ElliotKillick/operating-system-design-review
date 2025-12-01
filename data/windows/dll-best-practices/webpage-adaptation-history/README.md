## *Best Practices for Creating DLLs* Current Webpage Adaptation Git History

We collect the Git history of *Best Practices for Creating DLLs* as it exists in its present form: a webpage on Microsoft's official documentation website. This collection is possible because Microsoft maintains their Win32 documentation in a public git repo, specifically on GitHub.

```sh
git clone https://github.com/MicrosoftDocs/win32
cd win32
git blame desktop-src/Dlls/dynamic-link-library-best-practices.md > blame.txt
git log -p desktop-src/Dlls/dynamic-link-library-best-practices.md > commits.txt
```
