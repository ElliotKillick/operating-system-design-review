# Module Constructor and Destructor Writings Timeline

This is a collection of information on the first time different Microsoft, Microsoft-affiliated, or other credible sources wrote about module constructors and destructors on the Windows platform in chronological order:

- **June 1992**: [*Undocumented Windows*](https://bitsavers.computerhistory.org/pdf/microsoft/windows_3.1/Schulman_-_Undocumented_Windows_1992.pdf) by Andrew Schulman, David Maxey, and Matt Pietrek
  - Briefly covers the `LibMain` and Windows Exit Procedure (WEP) components of Windows 3.1
  - Not Microsoft-affiliated people (actually, Andrew Schulman was later a [consulting expert to Caldera in its 1999 lawsuit against Microsoft](https://www.oreilly.com/pub/au/33))
- **April 1993**: [*Windows Internals*](https://bitsavers.computerhistory.org/pdf/microsoft/windows_3.1/Pietrek_-_Windows_Internals_1993.pdf) by Matt Pietrek
  - In-depth coverage on the `LibMain` and Windows Exit Procedure (WEP) components of Windows 3.1
  - Released shortly before the first Windows NT release of Windows NT 3.1 on July 27, 1993
  - Matt Pietrek did not work for Microsoft when he wrote this book but he later did as a software engineer from 2004 to 2011
- **August 2002**: [*DllMain*](https://web.archive.org/web/20021001213130/http://msdn.microsoft.com/library/en-us/dllproc/base/dllmain.asp) by Microsoft
- **April 15, 2003**: [*Initializing code*](https://cbrumme.dev/initializing-code) by Chris Brumme (@cbrumme)
- **December 12, 2003**: [*DllMain and life before birth*](http://web.archive.org/web/20040117163921/http://weblogs.asp.net/oleglv/archive/2003/12/12/43068.aspx) and [*DllMain : a horror story*](http://web.archive.org/web/20040117164206/http://weblogs.asp.net/oleglv/archive/2003/12/12/43069.aspx) by Oleg Lvovitch (@oleglv)
- **January 27, 2004**: [*Some reasons not to do anything scary in your DllMain*](https://devblogs.microsoft.com/oldnewthing/20040127-00/?p=40873) by Raymond Chen
  - Raymond Chen has been a [member of the Windows Shell team since 2000](https://www.microsoft.com/en-us/windowsinsider/podcasts/the-start-of-something-different-part-1#:~:text=The%20shell%20team%2C%20that%E2%80%99s%20right.), at least as of 2020, and started writing for his blog titled *The Old New Thing* in July 2003
- **June 17, 2005**: [*How the NT Loader works*](https://learn.microsoft.com/en-us/archive/blogs/mgrier/how-the-nt-loader-works) by Michael Grier (@mgrier)
- **May 17, 2006**: [*Best Practices for Creating DLLs*](https://view.officeapps.live.com/op/view.aspx?src=http%3A%2F%2Fdownload.microsoft.com%2Fdownload%2Fa%2Ff%2F7%2Faf7777e5-7dcd-4800-8a0a-b18336565f5b%2FDLL_bestprac.doc) by Microsoft
  - [See here for information about Microsoft's official whitepaper covering module constructors and destructors](dll-best-practices/README.md)

This timeline aims to be comprehensive but is probably still missing some artifacts. The earliest public discussions on many Windows topics likely including module constructors and destructors took place on [external Microsoft mailing lists and Usenet groups](microsoft-technical-soruces.md).

**Note:** The information presented by these sources may be partially or entirely out-of-date, do not rely on them for information on how things work now.
