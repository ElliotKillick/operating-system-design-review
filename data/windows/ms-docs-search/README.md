# Microsoft Documentaion Search

Let's search Microsoft documentation Microsoft documentation for mentions of key terms.

Get Microsoft documentation repos:

```
git clone https://github.com/MicrosoftDocs/win32
git clone https://github.com/MicrosoftDocs/sdk-api
git clone https://github.com/MicrosoftDocs/cpp-docs
```

Grep each repo:

```shell
grep -rinI 'DllMain' > ../<OUTPUT_FILE>
```

```shell
grep -rinI 'loader lock' > ../<OUTPUT_FILE>
```
