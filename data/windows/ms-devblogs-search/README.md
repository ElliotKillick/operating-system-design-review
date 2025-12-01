# Microsoft Developer Blogs Search

Let's collect some technical information about Windows. Provided are commands to grep *The Old New Thing* articles for the terms "loader lock" and "DllMain". This comprehensive search is helpful for learning about the relevant Windows internals.

For the download script that facilitates searching, please see the [Microsoft Developer Blogs Search repo](https://github.com/ElliotKillick/ms-devblogs-search).

Grep articles:

```shell
grep -in -B5 -A5 'DllMain' * > ../dllmain.txt
```

```shell
grep -inE -B5 -A5 'loader\s+lock|LoaderLock' * > ../loader-lock.txt
```

We use a shell wildcard (`*`) instead of a recursve `grep` (`grep -r`) to automatically sort the search results alphabetically by filename.

We use `\s+` instead of a single space is to catch multiple whitespace (e.g. double space). Some articles can contain erroneous extra whitespace between words in their plaintext form, an issue that goes unnoticed when viewing the same articles in a web browser because the DOM renders multiple whitespace into looking the same as a single whitespace.
