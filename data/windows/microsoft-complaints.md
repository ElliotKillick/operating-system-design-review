# Microsoft Complaints

I've provided Microsoft lots of constructive criticism and ways to fix their systems throughout this writeup, but now I need to vent. Everyone has their personal list of complaints about Microsoft and Windows, so here's mine. Microsoft is unlikely to fix much of these issues due to either remaining backward compatible with rash decisions or because it would not benefit them as a business. Although I personally run Linux or BSD on my own devices, I have no choice but to use or support Windows in some cases, so I have the right complain about it. Note that my complaints are directed at the Microsoft Corporation as a company, the relevant decision makers at Microsoft, or the specific individual or team at Microsoft (or elsewhere) who are responsible. No one person at Microsoft or any big company oversees (or even has access to, for security reasons) all the code. With that said:

## Table of Contents

- [Microsoft Complaints](#microsoft-complaints)
  - [Table of Contents](#table-of-contents)
  - [Downloading Windows is Bloated](#downloading-windows-is-bloated)
  - [Windows Got String Encoding Wrong](#windows-got-string-encoding-wrong)
  - [GUI/Console Kernel Program Types Illustrate Bad Design](#guiconsole-kernel-program-types-illustrate-bad-design)
  - [UAC is Broken](#uac-is-broken)
  - [The Win32 Driver Model Sucked](#the-win32-driver-model-sucked)
  - [Windows Got File Names and Paths Wrong](#windows-got-file-names-and-paths-wrong)
  - [Registry Strings Are Broken](#registry-strings-are-broken)
  - [Locked EXEs and DLLs Are Annoying](#locked-exes-and-dlls-are-annoying)
  - [PowerShell Null Comparison "Design"](#powershell-null-comparison-design)
  - [No](#no)
  - [Recall Cannot be Uninstalled](#recall-cannot-be-uninstalled)
  - [How Was It Possible to Design CMD Variables To Be This Broken?](#how-was-it-possible-to-design-cmd-variables-to-be-this-broken)
  - [Okay—Great Idea—So, Get This: We Have GUI... Buttt We Put in the Kernel!!](#okaygreat-ideaso-get-this-we-have-gui-buttt-we-put-in-the-kernel)
  - [COM is a COMplexity Nightmare](#com-is-a-complexity-nightmare)
  - [ALPC is Bloat](#alpc-is-bloat)
  - [Useless Error Messages](#useless-error-messages)
  - [PatchGuard Was Never Robust](#patchguard-was-never-robust)
  - [The AARD Code](#the-aard-code)
  - [NTFS Alternate Data Streams: Dumb Pseudo-Secret Files](#ntfs-alternate-data-streams-dumb-pseudo-secret-files)
  - [Microsoft Locking Down the Kernel](#microsoft-locking-down-the-kernel)
  - [Faulty Default Search Path For Process Creation and Library Loading](#faulty-default-search-path-for-process-creation-and-library-loading)
  - [Native Loader Tight Coupling with .NET Loader](#native-loader-tight-coupling-with-net-loader)
  - [Internet Explorer Stifled Innovation Leading to a Lost Decade of Web Browser Development](#internet-explorer-stifled-innovation-leading-to-a-lost-decade-of-web-browser-development)
  - [Windows Communication Foundation (WCF) Bloatware Utopia](#windows-communication-foundation-wcf-bloatware-utopia)
  - [`CIM_DATETIME` Reinvents the Wheel](#cim_datetime-reinvents-the-wheel)
  - [Unix UGO \> Windows ACLs](#unix-ugo--windows-acls)
  - [Corporate Pseudointellectualism](#corporate-pseudointellectualism)
  - [Microsoft's Anti-Competitive Licensing Practices](#microsofts-anti-competitive-licensing-practices)
  - [Microsoft is Back to Its Anti-Competitive Licensing Practices, Now in the Cloud!](#microsoft-is-back-to-its-anti-competitive-licensing-practices-now-in-the-cloud)
  - [RNDIS is the Kind of Garbage Technology Microsoft is Known For](#rndis-is-the-kind-of-garbage-technology-microsoft-is-known-for)
  - [Long-Time Microsoft Sycophant Larry Osterman Cobbles Together a BS Answer for "Why wasn't Windows built on top of Unix?"](#long-time-microsoft-sycophant-larry-osterman-cobbles-together-a-bs-answer-for-why-wasnt-windows-built-on-top-of-unix)
  - [Dave Cutler is a Cringe Unix Hater](#dave-cutler-is-a-cringe-unix-hater)
  - [Microsoft and Engineering Inferiority](#microsoft-and-engineering-inferiority)
  - [Happy 50th Anniversary, Microsoft!](#happy-50th-anniversary-microsoft)
  - [Bill Gates Has Repeatedly Advocated for Policies that Help Big Pharma Monopolists Instead of Providing People with Lifesaving Drugs](#bill-gates-has-repeatedly-advocated-for-policies-that-help-big-pharma-monopolists-instead-of-providing-people-with-lifesaving-drugs)
  - [Microsoft's Office Open XML (OOXML) format Is An Attempt to Supplant Open Document Format (ODF), a Real Open Office Standard](#microsofts-office-open-xml-ooxml-format-is-an-attempt-to-supplant-open-document-format-odf-a-real-open-office-standard)
  - [Windows 11 Broke Dual-Booting Into Some Linux Distros Then Conveniently Took Nine Months to Fix the Issue](#windows-11-broke-dual-booting-into-some-linux-distros-then-conveniently-took-9-months-to-fix-the-issue)
  - [Microsoft Moves to Extinguish Chrome](#microsoft-moves-to-extinguish-chrome)
  - [Cookie Stuffing Malware in Microsoft Edge](#cookie-stuffing-malware-in-microsoft-edge)
  - [Bonus](#bonus)

## [Downloading Windows is Bloated](https://www.microsoft.com/en-us/software-download/windows11)

- The API "design" for fetching these downloads consists of multiple Microsoft endpoints returning HTML that the browser uses DOM selectors to parse (I guess Microsoft got bored of modern web API design patterns like returning JSON using HTTP REST APIs or, more recently, GraphQL?)
 - I had to make an [entire project](https://github.com/ElliotKillick/Mido) (**Note:** I need to do a bit of work on this project as it stands... I am busy) to work around this Microsoft bloatware (stop wasting people's time, Microsoft)

## Windows Got String Encoding Wrong

- A classic tale of Unix getting it right
  - UTF-8 was invented by Ken Thompson (Unix co-founder) and Rob Pike (former Unix developer at Bell Labs and now a Google engineer) in 1992.
  - The first version of Windows to ship with "wide" functions for UCS-2 unicode support (the predecessor to UTF-16) was Windows NT 3.1 in 1993. Microsoft was a Unicode Consortium member that heavily influenced the creation of UCS-2 so NT could have standardized multilingual support.
- UTF-8 is simply the best: Backwards compatible with ASCII, 2x smaller for Latin text than its UTF-16 counterpart (with its small size being crucial for the web), and no horrendous BOM
- Microsoft is now stuck with [separate "wide" unicode functions](https://learn.microsoft.com/en-us/windows/win32/intl/unicode-in-the-windows-api) bloating the Windows API forever, [format specifier disagreements between standard C compilers and the MSVC compiler](https://devblogs.microsoft.com/oldnewthing/20190830-00/?p=102823#:~:text=table), and things like PowerShell annoyingly default to UTF-16 LE with [mandatory](https://stackoverflow.com/a/65192064) [BOM](https://stackoverflow.com/a/66553739)
- Despite all the known shortcomings that UCS-2 encoding of Unicode codepoints came with compared to the genius of UTF-8, the Microsoft "engineers" behind the decision to implement UCS-2 in the operating system said ["trust me bro"](https://www.youtube.com/watch?v=HDicLHBvQQM&t=5248s), and that was it. In retrospect, Raymond Chen thinks it was a "good call" (to be fair, by Microsoft standards it probably is one of their better calls—it is also worth acknowledging the inherit bias: as someone who works for Microsoft and has worked at Microsoft for a long time, Raymond has nothing to gain, at least not from Microsoft, by openly criticizing them).

## [GUI/Console Kernel Program Types Illustrate Bad Design](https://stackoverflow.com/a/494000)

- The problem with monoliths is that once you "design" one tightly coupled set of components, it becomes harder to make all the other parts modular, so it sprials out control quickly until you're left with Windows

## [UAC is Broken](https://github.com/hfiref0x/UACME#references)

- This is an issue (one fundamentally of poor design)

## [The Win32 Driver Model Sucked](https://en.wikipedia.org/wiki/Windows_Driver_Model#Criticism)

- It wasn't until 2019 that Microsoft replaced it with Windows Driver Frameworks (WDF)
- Looks like I found out why my work-provided laptop fails to wake up from sleep sometimes thus wasting valuable company time (to be fair, maybe the company I work for should also preload less low quality software/drivers on their systems... the battery life on a fresh and modern laptop from them is a whopping 2 hours and used to burn me until I disabled some background tasks so the thing would not run so hot)

## [Windows Got File Names and Paths Wrong](https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file#naming-conventions)

- Windows reserved filenames (e.g. `NUL` or `AUX`) continue to be [problematic in the Linux source tree](https://github.com/torvalds/linux/blob/master/include/soc/arc/aux.h) (or even when a filename just includes a `:` or `?`, or the worst is `\` path separators because programming languages recognize them as escape sequences)
- I understand Windows inherited the reserved filename issue from MS-DOS, which inherited it from 86DOS/QDOS (which itself inherited the issue because CP/M DOS originally had no file system hierarchy causing the developers to taint the global file namespace), but Microsoft could have employed a per-application/per-module compatibility layer that used versioning or an embedded timestamp to maintain support with the old applications relying on this functionality while not screwing all future Win32 applications with this weird quirk (a versioning mechanism could also have been added to Batch scripts). Technical debt like this always adds up until developing for a system becomes a herculean task or needs endless layers of abstraction to remove the developer from the broken stuff so anything can get done efficiently. [Microsoft only started making per-application compatibility mechanisms in Windows XP!](https://devblogs.microsoft.com/oldnewthing/20031224-00/?p=41363) As a result, Microsoft remains accountable.

## [Registry Strings Are Broken](https://devblogs.microsoft.com/oldnewthing/20040824-00/?p=38063)

- Well, nobody thought that one through (even a single step ahead)... seems like that's the story everything Microsoft and Windows follows

## [Locked EXEs and DLLs Are Annoying](https://stackoverflow.com/a/196908)

- Running into that whole "This action can't be completed because the file is open in \<Application Name\>" error when trying to do things on Windows is the worst (the error message makes it sound kind of reasonable but in practice it just sucks)
- Among other things, this poor design is the root cause for [installers being unable to remove themselves](https://devblogs.microsoft.com/oldnewthing/20230911-00/?p=108749), which leads to them using hacks
  - I've looked around and apparently, if you don't want to use a polling script (which is not guaranteed to work in locked down corporate environments), you can [do this trick](https://www.codeproject.com/Articles/17052/Self-Deleting-Executables#Conclusion6) (at the cost of looking like malware, of course), which appears to be documented

## [PowerShell Null Comparison "Design"](https://learn.microsoft.com/en-us/powershell/utility-modules/psscriptanalyzer/rules/possibleincorrectcomparisonwithnull)

- Hey, this is how it "works by-design", the PowerShell designers really went back to the drawing board to engineer some state of the art stuff here (nothing against the PowerShell devs personally, of course, but you're making this too easy for me)
- This random [Yota notation](https://en.wikipedia.org/wiki/Yoda_conditions) footgun built right into the language never fails to get a laugh out of me
- To be fair, this masterpiece could be a product of goated Microsoft employees purposely slipping comically rubbish design into Windows (in which case, bravo—I like it a lot)

## [No](https://elliotonsecurity.com/perfect-dll-hijacking/shellexecute-initial-deadlock-point-stack-trace.png)

## [Recall Cannot be Uninstalled](https://www.theverge.com/2024/9/2/24233992/microsoft-recall-windows-11-uninstall-feature-bug)

- Nobody wants Recall, literally noone
- Microsoft originally wanted Recall to be enabled by default but they backed off because of the backlash (we will probably have to wait for Windows 12 or until the outcry has died down for Microsoft to try and slip in some updates that enable it by default again, it's not like it would be out of character for the slimeballs behind decisions like this at Microsoft to do)

## [How Was It Possible to Design CMD Variables To Be This Broken?](https://nvd.nist.gov/vuln/detail/CVE-2024-24576#vulnDescriptionTitle)

- The shell is my favorite, and yours sucks
- Okay, CMD and Batch is so horribly broken—like I don't even get how someone can make something so garbage

## [Okay—Great Idea—So, Get This: We Have GUI... Buttt We Put in the Kernel!!](https://stackoverflow.com/questions/28532190/why-does-windows-handle-scrollbars-in-kernel)

- How is it that the Windows GUI is still slower than it is on Mac or Linux even though the former two do not obliterate security by putting something as complex as [GUI infrastructure into the kernel](https://j00ru.vexillium.org/syscalls/win32k/64/) like Microsoft does?
- You know what? It's fine. Microsoft is finally switching up their lazy loading tendencies for [preloading the graphical shell](https://www.techpowerup.com/343459/preloading-file-explorer-in-windows-11-doubles-ram-usage-offers-minimal-speed-boost)—definitely worth the 2x increase in memory usage

## [COM is a COMplexity Nightmare](https://devblogs.microsoft.com/oldnewthing/20220210-00/?p=106243)

- A wise man once said: "If you can't explain it simply, you don't understand it well enough." I don't think Einstein ever anticipated the atrocity that is COM when he made that statement

## [ALPC is Bloat](https://infocon.org/cons/SyScan/SyScan%202014%20Singapore/SyScan%202014%20presentations/SyScan2014_AlexIonescu_AllabouttheRPCLRPCALPCandLPCinyourPC.pdf)

- ALPC (part of the `rpcrt4.dll` internal Windows subsystem) is actually an abstraction layer over its weird hybrid model that internally combines the use of message-based communication for small messages and shared memory to reduce copying overhead for large messages. So, ALPC is not composable and breaks the Unix philosophy of "do one thing and do it well". A better solution would be for subsystems to use the best purpose-built communication mechanism for their use case (i.e. use the best tool for the job). However, COM with all of its call marshalling is the biggest user of ALPC today and Microsoft does way too much systems stuff in COM, so I guess magic begets magic (the original LPC was purely message-oriented and LPC came before COM but COM came before ALPC).
- ALPC bloats by including a full asynchronous communication stack (not just non-blocking `read` like Unix systems have with the `O_NONBLOCK` socket option) that bakes in use of the Windows I/O completion model (similar to the Unix I/O readiness model using the `epoll` or `select` APIs) and callbacks for kernel-mode ALPC so the server can reach back into the client that called it
- ALPC benefits from the uniform interface for security on Windows (loosely because it still uses a special "Connect" privilege where Unix would use read/write), the handle object (like files on Unix, even POSIX message ques are exposed as files in `/dev/mqeueue` on Unix systems), but then adds so much garbage on top that all boils down to "if you are using this feature of ALPC, the composition of your service is wrong and needs refactroing". Rubbish like per-message security and impersonation (i.e. the server doing work using the client's token) and all of the unnecessary security controls ALPC creates from this.
- [Look at how much complexity is involved in doing anything in ALPC](https://github.com/csandker/InterProcessCommunication-Samples/blob/master/ALPC/CPP-ALPC-Basic-Client-Server/CPP-ALPC-Basic-Server/CPP-ALPC-Basic-Server.cpp), [see how many lies of code that is?](https://github.com/csandker/InterProcessCommunication-Samples/blob/855d79452859400da9826941133553d3b5b049e9/ALPC/CPP-ALPC-Basic-Client-Server/CPP-Util/CommonALPC.cpp#L362) Now compare that to to Unix `read`/`write` or POSIX message queues, or any of the other IPC APIs. And so many vulnerabilities have all ready been found in this complexity hell hole. Honestly, it's so convoluted it makes me a bit sick.
- [ALPC is the digital equivalent of scrap metal](https://slideplayer.com/slide/4683183/#:~:text=Windows%20codenamed%20%E2%80%9CLonghorn%E2%80%9D%20solution%3A%20ALPC) that was salvaged from the [Windows Longhorn disaster](https://www.youtube.com/watch?v=vxmZPMg7vIs)
- A lot of things can ["work"](https://devblogs.microsoft.com/oldnewthing/20150717-00/?p=90881), that doesn't make it the best option or even a good option
  - It's also worth noting that like misinformation, over-engineered software or bloatware follows [Brandolini's law](https://en.wikipedia.org/wiki/Brandolini%27s_law)

## [Useless Error Messages](https://elliotonsecurity.com/perfect-dll-hijacking/offlinescannershell-mpclient-dll-unsigned-by-microsoft-error.png)

- Rude, not helpful, and suspiciously vague (also, security through obscurity vibes)
- This sketchy error message and the surrounding context of trying to run a non-Microsoft signed binary when Microsoft's signature is enforced has a very [AARD code feel](https://en.wikipedia.org/wiki/File:Windows_3.10.068_setup_AARD_code.png) to it, not good
- "Bad Image" is a generic Windows error message but its use in this circumstance is stupid or even malignant

## [PatchGuard Was Never Robust](https://en.wikipedia.org/wiki/Kernel_Patch_Protection#Criticisms)

- Security through obscurity (having an arbitrary read-write primitive on the kernel, such as when you're a driver, will always defeat PatchGuard)
- Alas, the only thing that could tear Microsoft's relationship with security through obscurity would be if they made the NT kernel open source (or at least source available) as Apple does for their [XNU kernel](https://github.com/apple-oss-distributions/xnu/tags), and we all know Microsoft doesn't have the balls to do that
- In addition, while I don't think this group policy warrants its own complaint, meaningless and superficial [controls that give organizations and users a false sense of security](https://devblogs.microsoft.com/oldnewthing/20241001-22/?p=110330) arguably do more harm than good (unless you value compliance box-checking for the sake of checking boxes)

## [The AARD Code](https://web.archive.org/web/20190727234624/https://blogs.msdn.microsoft.com/larryosterman/2004/08/12/aardvarks-in-your-code/)

- [The AARD code is a classic, predatory case of Microsoft killing interoperability to ensure the success of MS-DOS and Windows](/data/windows/aard-code-market-effects.md)
- The AARD code purportedly originates from tight coupling between the MS-DOS system and Windows GUI layer that effectively bundled them together in a way that not only terrible design but also anti-competitive. According to Larry Osterman of Microsoft, this tight coupling happened because the "Windows team had a problem to solve, and they did not care *how* they solved it" (problem not specified). It is not a normal, okay, or a good thing to not care about how a problem is solved. The means and ends through which a goal is accomplished both matter for anyone who wishes to act [righteously](https://devblogs.microsoft.com/oldnewthing/20211116-00/?p=105927). Larry stating "I totally agree with the basic concept of Windows checking for an alternative version of MS-DOS and refusing to run on it" as a result of the "sleazy" (Larry's own descriptor) actions of Windows towards MS-DOS amounts to nothing more than blameshifting. Good software design has architectural layering: a GUI is so high-level that letting it tinker with the lowest-level bits of the operating system like that is unholy; the GUI shell depends on the operating system, hence the internals of the operating system should not circularly depend on the GUI shell through modification via patching.
  - In the example Larry gave, instead of manual patching by the Windows program, the operating system on its own should be modified with a documented interface for setting the maximum number of open files thus increasing the size of the "internal file table" or "SFT" on MS-DOS (like the `setrlimit` system call on Unix systems which has been present since the early 1980s—long before Windows 3.1 ever came into the picture). Calling this new system interface should have been the default approach for Windows running on MS-DOS versions that had support for it. Supporting older MS-DOS versions without this new interface would have been best accomplished by Windows detecting an MS-DOS version number below a certain threshold, then loading a program (like `ulimit` on Unix systems) or independent module that was distributed with Windows to make the necessary changes to MS-DOS internals. Another benefit of this approach is that it would enable any program that requires opening many files to simply use this interface, not just Windows.
    - Windows groveling into the internal MS-DOS SFT data structure meant that the structure had to stay the same to remain backward compatible (barring big hacks). When the SFT data structure changed in MS-DOS 4.0, MS-DOS had to do some really wacky and hacky stuff to retain application compatibility with the Windows program. As Larry stated, he had to change MS-DOS to detect when the `WIN.COM` program started then patch the running Windows program with the SFT's updated ABI definition. Therefore, Microsoft even made technical problems for themselves by taking the expedient shortcut they earlier did!
  - Anyway, the anti-debugging features and other tricks included in the AARD code proves that Microsoft's goal in making the AARD code was to cripple its competitors, and not for any technical reason
  - Larry did a great job at exposing himself as one of the ["coders" at Microsoft](https://www.youtube.com/watch?v=29RkHH-psrY&t=5598s) by taking the stance and making the admissions he did here, at least, at the time he wrote this article in 2004
- [Microsoft was eventually sued over the AARD code.](https://en.wikipedia.org/wiki/AARD_code#Lawsuit_and_settlement) The lawsuit was brought by Caldera many years after the fact after DR-DOS had already been sold to the company by Novell. [In 2000](https://web.archive.org/web/20000819023434/http://www.oreillynet.com/pub/a/network/2000/02/07/schulman.html), the suit resulted in a settlement of $280 million USD paid by Microsoft and reputational damage. However, this settlement came way too late to save DR-DOS as a viable competitor. The financial settlement paled in comparison to the competitive advantage Microsoft gained by effectively blocking DR-DOS during a critical period, allowing Microsoft to maintain its dominant position in the PC operating system market.

## [NTFS Alternate Data Streams: Dumb Pseudo-Secret Files](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-fscc/e2b19412-a925-4360-b009-86e3b8a020c8)

- It's just not a good feature, it violates [POLS](https://en.wikipedia.org/wiki/Principle_of_least_astonishment)

## [Microsoft Locking Down the Kernel](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/deprecation-of-software-publisher-certificates-and-commercial-release-certificates)

- All kernel-mode drivers now require Microsoft's signature (*not* just a signature from any trusted root authority), it's clear Microsoft will continue locking down Windows until it becomes a Fisher-Price toy if we let them get away with it

## [Faulty Default Search Path For Process Creation and Library Loading](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessa#parameters)

- Big search path blunder for programs and libraries, most notably: including the current working directory (CWD) by default
- Loading programs from the CWD by default makes it impossible to safely use the CMD shell upon navigating to an untrusted directory (PowerShell implements a high-level hack around this issue to provide the Unix shell `./` behavior)
- This current working directory mess started sensibly with CP/M DOS because the system only had one directory. MS-DOS originates from CP/M DOS thus why it too has this broken default. But keeping a feature as broken and insecure as this one is and remains a bad choice. For instance, starting with Windows NT, since that marked the introduction of security and privilege levels into Windows, a change could have been made so that new 32-bit executables do not load libraries from insecure places like the current working directory by default since that was only a left over relic from CP/M DOS, while still remaining compatbile with the old behavior for 16-bit applications from DOS.

## [Native Loader Tight Coupling with .NET Loader](https://repnz.github.io/posts/dotnet-executable-load/)

- Can someone please explain to me why someone thought it would be a good idea to integrate .NET with the native loader? Imagine if every high-level language did this. (In Windows decompiled code, `LdrpInitializeProcess` may call `LdrpCorInitialize` to do a bunch of .NET specific process initialization)
- While we're at it, I for one would welcome JVM's integration into the Windows loader (assuming it is not some evil [Visual J++](https://en.wikipedia.org/wiki/Visual_J%2B%2B) version of Java, and how about we go for the Amazon Corretto Java distribution, that would be good)

## [Internet Explorer Stifled Innovation Leading to a Lost Decade of Web Browser Development](https://www.investopedia.com/ask/answers/08/microsoft-antitrust.asp#mntl-sc-block_7-0)

- Imagine how much further along the world's web technology could be if not for Internet Explorer unfairly monopolizing the market for so long (I wasn't alive then but probably a good decade ahead, if what I've heard from some people is correct... a lost decade of web development)
  - To be fair, Microsoft's new venture with Edge has been [contributing back](https://www.windowslatest.com/2020/11/19/new-data-proves-microsoft-is-the-best-thing-to-happen-to-chrome/) signficant improvements to the base Chromium project, which should be celebrated. At the same time, we as people, including people working at Microsoft, [must proceed with caution](https://gizmodo.com/microsoft-windows-google-chrome-feature-broken-edge-1850392901) whenever [one company controls an entire vertical](https://www.investopedia.com/is-ticketmaster-a-monopoly-6834539)

## [Windows Communication Foundation (WCF) Bloatware Utopia](https://en.wikipedia.org/wiki/Windows_Communication_Foundation)

- Windows Communication Foundation (WCF) is some horrendous Microsoft technology based on the [WS-\*](https://en.wikipedia.org/wiki/List_of_web_service_specifications) web service standards, probably some of the grossest standards ever divised
  - WS-\* is all highly complex, slow, poorly interoperable, and over-engineered technology
  - Microsoft played the primary role in creating these standards then Windows completely bought into all the WS-\* technologies
  - WCF is layered bloat on top of .NET, which is just agonizing
- SOAP, made by Microsoft for web APIs and later WS-\*, is bloat on top of XML-RPC (also by Microsoft), which itself is bloat on top of XML
  - Microsoft also helped the W3C standardize XML itself in 1998, so I'm not dedicating a separate complaint to this
- Of course, [WCF solved a problem at the time](https://download.microsoft.com/download/c/2/5/c2549372-d37d-4f55-939a-74f1790d4963/introducing_wcf_in_net_framework_4.pdf), but as I've emphasized prior, that doesn't make it the best solution or even a good one. Even at the time, [people knew](https://www.reddit.com/r/programming/comments/exodm/soap_the_s_stands_for_simple_well_not_really_funny/) that [WS-* was low quality tech](https://web.archive.org/web/20150314044423/http://wanderingbarque.com/nonintersecting/2006/11/15/the-s-stands-for-simple/) (luckily for us, this common sense did not require 20/20 hindsight)
- Microsoft eventually came to their senses on this matter and now [recommends Google's gRPC as a WCF alternative](https://learn.microsoft.com/en-us/dotnet/framework/wcf/whats-wcf#grpc-as-an-alternative-to-wcf)

## [`CIM_DATETIME` Reinvents the Wheel](https://learn.microsoft.com/en-us/windows/win32/wmisdk/cim-datetime)

- There's this great thing called [ISO-8601](https://en.wikipedia.org/wiki/ISO_8601) circa 1988, you should try it ([ISO-8601 also supports high precision](https://stackoverflow.com/a/31477453) like `CIM_DATATIME`)
- I have a POSIX-compliant shell one-liner for parsing these out to a Unix timestamp and I would share, but I need to find it

## Unix UGO > Windows ACLs

- UGO achieves permission granularity by defining clear groups and then assigning users to any number of those groups where each file can have one group, while ACLs work by tacking permissions directly on files themselves
  - Unlike Windows ACLs, UGO groups cannot be nested, which seems like it could be a weakness until you realize that the flat model for groups avoids nasty side effects such as the possibility of group cycles while avoiding excessive permissions and keeping file access times low (nested groups can be simulated by assigning users to more groups, anyway)
- In practice, ACLs are unmanageable, inefficient, and insecure (in terms of auditability for preventing backdoors) because a file system can have tons of files and each file can have up to [about 1,820 permissions](https://learn.microsoft.com/en-us/troubleshoot/windows-server/windows-security/error-add-user-to-security-permissions#cause) meanwhile the UGO model is so elegant and clean, especially with every state of each letter fitting squarely into its own octect for perfect memory efficiency
- Changing security information on Windows is always slow as molasses in my experience with [those dialog boxes](https://us1.discourse-cdn.com/spiceworks/original/4X/b/f/3/bf3443805e9ad84107c66028c27d7c585385bcfc.jpeg) that have no progress indicator
- On top of UGO, modern Unix systems also optionally support ACLs if throwing permissions directly on files is what you want to do, but UGO undoubtably works best as the base permission model for an operating system
- Like most everything, Unix got this right before MS-DOS or Windows even existed ([Microsoft would know](https://www.pcjs.org/software/pcx86/sys/unix/ibm/xenix/1.0/), pro tip: on Xenix enter `fd /xenix.fd` as the boot program then run `ls -l`)

## Corporate Pseudointellectualism

- Corporate pseudointellectualism is a new term I'm coining to describe the immense "intellect" that is often used to justify a bad product, corporate greed, and general overreach by a corporation while also enabling workers to justify their creations to themselves and those unwitting enough to believe them (like all pseudo-intellectuals, these people believe they are smart but it is only hubris)
- In trying to create something "smart" or different, companies can often overcomplicate things when a simpler and more effective solution existed
- Corporate pseudointellectualism accounts for the entire backwards software architecture that is Windows (or rather, "Wrongdows")
  - Raymond Chen talking about the potential for a bug in the loader then recognizing now as a good time to glaze the people in charge of loader lock as ["really smart"](https://devblogs.microsoft.com/oldnewthing/20140808-00/?p=293#:~:text=a%20lot%20of%20really%20smart%20people%20are%20in%20charge%20of%20the%20loader%20lock) (to be fair and interpret with context, by Microsoft standards these people probably are some of the smarter ones).

## [Microsoft's Anti-Competitive Licensing Practices](https://www.washingtonpost.com/archive/politics/1994/07/17/microsoft-settles-case-with-justice/dd31497e-612d-4ae9-9b22-9c3a428deaa2/)

- These predatory licensing techniques, namely the company-wide vendor lock-in aspect their "per-processor" licensing deals, were hugely helpful in allowing Microsoft to lock out its competitors since most people will use the operating system that comes with an OEM-provided computer instead of buying an operating system separately
- Microsoft began these shady licensing practices in 1988 when [MS-DOS 4.0](https://en.wikipedia.org/wiki/MS-DOS_4.0_(multitasking)) released and before [Windows 3.0](https://en.wikipedia.org/wiki/Windows_3.0) came out. The unfair licensing agreements ended in July 1994 upon being struck down by the Justice Department, which came after the first Windows NT release of Windows NT 3.1

## [Microsoft is Back to Its Anti-Competitive Licensing Practices, Now in the Cloud!](https://cloud.google.com/blog/topics/inside-google-cloud/filing-eu-complaint-against-microsoft-licensing)

- Microsoft is screwing other cloud platforms by unfairly licesning Windows Server and SQL Server to be 5x more expensive on non-Azure platforms (Google complained at end of September in 2024)
- Microsoft trying not to be anti-competitive challenge (impossible)
- It's almost as if Microsoft needs to be separated from Windows

## [RNDIS is the Kind of Garbage Technology Microsoft is Known For](https://en.wikipedia.org/wiki/RNDIS)

- [Linux developers are trying to take out this Microsoft RNDIS trash](https://www.phoronix.com/news/Linux-RNDIS-Removal-EOY2024) because it is some impossible to secure, tightly coupled, junk—let's hope they succeed
- Don't worry NT kernel, I would never forget about you

## [Long-Time Microsoft Sycophant Larry Osterman Cobbles Together a BS Answer for "Why wasn't Windows built on top of Unix?"](https://www.youtube.com/watch?v=hI36xNbpBoU)

- The reason Windows NT exists is because [Microsoft wanted to control the operating system standard](the-rise-of-microsoft.md), that is why Bill Gates, the ultimate decision-maker at Microsoft, decided to abandon Xenix and consequently Unix
  - There is, nor was there ever, any technical merit to the existence of Windows NT; it exists solely due to business reasons
- Larry's point about historic Unix implementations having a "single global lock to enter the operating system" (i.e. the [big kernel lock](https://en.wikipedia.org/wiki/Giant_lock)) was irrelevant when Unix implementors made the big kernel lock and for the longest time after that because CPUs with more than one core were non-existent except in hardware labratories—the first mainstream dual core processor is widely regarded as the [Intel Core 2 Duo](https://en.wikipedia.org/wiki/Intel_Core_2) in 2006. Therefore, it made absolute sense for Unix-like systems to start with a big kernel lock as a quick fix to ensure the operating system's correctness on multi-core machines, while gradually reducing its use over time to improve multi-core performance.
  - Larry says it would have required "significant engineering" to change this implementation detail of Unix, so the obvious approach is to create a new operating system from scratch (sarcasm). This line of reasoning is a non-sequitur. If the design of Unix system call interfaces made them inherently tightly coupled in a way that made splitting up the big kernel not entirely doable, that would be a different story and a truly tricky engineering challenge. But that is not the case here, Unix implementors only created the big kernel lock as a primitive support mechanism for multi-core processors while they were still in incubation, then slowly split that lock up to improve performance.
  - In addition, up until Windows 7, a lock known as the [dispatcher lock](https://www.youtube.com/watch?v=OAAiOEQhsK0&t=859s) was present in NT, which functioned similarly to a big kernel lock except that it serialized access of any system call that passed in a handle to a kernel object—which is a lot of them. Solving this problem was was a (real) significant engineering challenge because it came as a consequence of NT tightly coupling many unrelated concepts together into the category of being an object controlled by the [Object Manager](https://en.wikipedia.org/wiki/Object_Manager) with it [being possible to wait for many independent object types at the same time](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitformultipleobjects#remarks). Today, this problem is still not fully solved in the ["wait all case"](https://www.youtube.com/watch?v=OAAiOEQhsK0&t=2831s) (i.e. [`WaitForMultipleObjects`](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitformultipleobjects) with `bWaitAll` equal to `TRUE`) because the broad synchronization requirement is inherit in the design of NT's interfaces.
  - The dispatcher lock problem was also worsened by the fact that Windows and Microsoft software commonly used—and this is still true today—[event objects](https://learn.microsoft.com/en-us/windows/win32/sync/event-objects) as its sychronization mechanism thereby always calling into the kernel and necessitating use of the former big lock. In contrast, user-mode synchronization mechanisms (e.g. POSIX mutexes or Windows critical sections) do not require any system call into the kernel when their is no contention on the lock, which comes with the added advantage of avoiding context switch overhead, too.
- A "hyperscale cloud operating system", huh? [Microsoft runs Azure on Linux](https://www.wired.com/2015/09/microsoft-using-linux-run-cloud/) and [most of the machines on Microsoft's own cloud platform run Linux](https://build5nines.com/linux-is-most-used-os-in-microsoft-azure-over-50-percent-fo-vm-cores/). Now Larry's just throwing around buzzwords... bro, just put the fries in the bag.
- I have to commend the comments section of this video for really standing up to Larry's part in exercising [Brandolini's law](https://en.wikipedia.org/wiki/Brandolini%27s_law)

## Dave Cutler is a Cringe Unix Hater

- Let's dissect some reasons Dave Cutler has given for his despise of Unix
- A frequently quoted remark from *Showstopper! the Breakneck Race to Create Windows NT and the Next Generation at Microsoft* that highlights Dave's disdain for Unix is: “Unix is like Cutler’s lifelong foe,” said one team member who’d worked with Cutler for nearly two decades. “It’s like his Moriarty [Sherlock Holmes’s nemesis]. He thinks Unix is a junk operating program designed by a committee of Ph.D.s. There’s never been one mind behind the whole thing, and it shows. So he’s always been out to get Unix. But this is the first time he’s had the chance.”
  - There is little substance to indicate why Dave hates Unix here, accept maybe some anti-intellectualism, so let's move on
- [Dave Cutler supposedly dislikes Unix's synchronous-first I/O model](https://retrocomputing.stackexchange.com/a/14151)
  - **Note:** [Is it unverifiable whether Dave said the quote recited in this Stack Exchange question](https://en.wikipedia.org/w/index.php?title=Dave_Cutler&diff=prev&oldid=1062826270) since the [VMS vs. UNIX debate in which he said the line was presumably never recorded](https://en.wikipedia.org/w/index.php?title=Dave_Cutler&diff=prev&oldid=490478793#Attitude_towards_Unix)
  - VMS and Windows had an asynchronous-first I/O model
  - Asynchronous I/O has two parts:
    - Non-blocking read: Unix did not always have support for non-blocking I/O (the `O_NONBLOCK` file descriptor option), but by Unix System V ABI Release 4 it did due to inheriting [the feature](https://github.com/calmsacibis995/svr4-src/blob/main/uts/i386/sys/fcntl.h#L38) [from BSD](https://en.wikipedia.org/wiki/UNIX_System_V#SVR4) [after it was defined in the first POSIX release](https://archive.org/details/POSIX.1-1988/mode/2up?q=NONBLOCK).
    - I/O readiness notifications: POSIX received its [first I/O readiness notification APIs of `select` and `poll` in POSIX Issue 4, Version 2](https://pubs.opengroup.org/onlinepubs/9799919799/functions/select.html#tag_17_403_11) released in 1994
  - Unix I/O is stream-oriented whereas the Windows Asynchronous Procedure Calls (APC) model is packet-oriented/message-oriented. Message-oriented communication can be useful and that is why today we have [POSIX message queues](https://pubs.opengroup.org/onlinepubs/9799919799/) for this purpose. However, message-oriented communication is generally reserved for specialized use cases whereby preserving message boundaries and atomicity is critical, such as enent-oriented interprocess communication, real-time systems, or scenarios requiring strict message ordering and priority handling.
  - The takeaway is that any shortcomings Unix may have had in asynchronous I/O at the time of Dave Cutler's original comment have since long been resolved—and that Unix's simple, unopinionated, and composable design philosophy ultimately led it to adopt the right I/O communication model as its basis.
- ["Everything is a file" is bad](https://www.youtube.com/watch?v=xi1Lq79mLeE&t=6454s)
  - A file is the universal interface of Unix. The filesystem is like one big namespace everything in it is easily accessible through generic tools. This is a good thing, not a bad thing. Objects in Windows exist in their own namespace, effectively a virtual file system, that need specialized tools to view and access.
    - This goes back to a philosophical difference in VMS vs Unix: VMS prefers structured data, specialized tools and Unix prefers unstructured data, generic tools. I would argue that Unix's approach is superior because it is effectively an unopinionated operating system design the flexibility of which allows anything to easily be implemented on top.
    - Even POSIX message queues are exposed as files in `/dev/mqueue`
  - Windows objects can be named objects (i.e. public objects) or unnamed objects (i.e. private objects). Named objects live in the object namespace (e.g. `\Sessions\0\BaseNamedObjects\MyEvent`). Unix systems do not have private files or objects but do have numbered file descriptors (e.g. `/proc/1/fd/0` is the standard output of PID 1), which achieve the same semantics. I think Unix does a better job at allowing a file to be the universal interface here because even the internal entitity of one process can easily be accessed by another process without any [confusing handle duplication](https://devblogs.microsoft.com/oldnewthing/20070829-00/?p=25363) or worrying about namespace collisions in the same session for named objects.
  - I just don't like the vibe of kernel resources being "objects". It just feels wrong to me. Like I rather call `pthread_join` than `WaitForSingleObject` on a thread. To me, the latter feels overly abstract. Unix systems do not have a special system call for joining threads (not that there would necessarily be anything wrong with that), either. Instead, `pthread_join` is implemented in user space through the use of a futex. Cases like these show how Windows objects promote centralization of system components in the kernel.
    - This centralization of system components in the kernel has proven to be harmful due to the [dispatcher lock before Windows 7](https://www.youtube.com/watch?v=OAAiOEQhsK0&t=859s) and apparently [handle lookup cost](https://www.youtube.com/watch?v=xi1Lq79mLeE&t=6510s)
  - Dave Cutler could be taking ["everything is a file"](https://en.wikipedia.org/wiki/Everything_is_a_file) too literally, but I also want to know who these "grand poobahs of the academic world" are that Dave is referring to
- In the end, it is clear that Dave Cutler intentionally designed Windows NT with the goal of making it not Unix-like and in doing so he created an operating system with a completely backwards and hypocritical design

## Microsoft and Engineering Inferiority

- Here are some good takes highlighting the poor engineering standards embedded in Microsoft's culture
- NT's chief architect on the culture at Digital Equipment Corporation (DEC) versus Microsoft: ["The environment was a lot different because DEC was a engineering run company and we're always based on engineering excellence. Microsoft I think justifiably I would say is a marketing-based company and they weren't based on engineering excellence. So, there was sort of a little clash between our culture and the Microsoft culture. But I think that we instilled a lot of that into the Microsoft culture but we still had a lot of things going on that were sort of outside our control that were within the Microsoft culture. So, it made the project a lot longer—and the environ—it was very different."](https://www.youtube.com/watch?v=29RkHH-psrY&t=5598s) ― Dave Cutler, NT chief architect
- Interviewer Grant Saviers reciting a comment Dave Cutler made to him off camera earlier: ["Somewhere along the word [sic], you said to me, you know, there were a lot of coders at Microsoft but no software engineering and you had to bring the discipline of software engineering to Microsoft."](https://www.youtube.com/watch?v=29RkHH-psrY&t=5646s) Dave Cutler: "Yeah, I think that one of the things that we brought was the engineering discipline on building software and I don't think that Microsoft had the engineering discipline that we [at DEC] had at that time."
  - Dave claims to have introduced "engineering discipline on building software" meanwhile, in the first release of the next-generation operating system he designed, Windows NT: his own new VMS-inspired inter-process communication (IPC) layer, embedded as part of a remote procedure call (RPC) library that primarily served in "local RPC" as the main IPC mechanism of the system, cyclically depended on a new "advanced" system library that existed as a grab bag in place of a coherent API design for Microsoft's successor to MS-DOS. Windows NT's IPC system was influenced by Dave Cutler's VMS experience, featuring an asynchronous, message-oriented design—especially because one of his main gripes with Unix was its synchronous, stream-oriented IPC model. What Dave Cutler says here bears no relation to reality, at least not in any practical sense.
- ["All of the software that Microsoft produced that was operating system stuff was pretty pathetic. It was all assembler, it was non-portable."](https://www.youtube.com/watch?v=pQ326T9cqpU&t=16s) ― Dave Cutler
  - The second comment arises because an assembler-only OS was behind the times when Dave Cutler was looking at it with plans to make NT in the late 1980s (although to be fair, when 86-DOS/MS-DOS was first made in the early 1980s, an assembler-only OS was more typical). Making an OS only in assembly language discourages clean architectural separation between the OS and the hardware it runs on. Your operating system can end up relying on hacks in the underlying CPU architecture, which MS-DOS on the IBM PC did. (**Side note:** Many applications that ran on MS-DOS relied also relied on hardware hacks making these applications non-transferable to Windows NT)
- On the level of talent in the VMS team versus the NT team: ["In VMS, we had—probably the whole project had 20 people—and that included the compiler people—and the main thing, the tools there was the Fortran compiler because we were selling that as a Fortran machine. But every single one of those people were, you know, really just top-notch people—and so it was a very, very talented group, extremely talented—and we did not have that with the NT group, completely. I mean we had a good core and we had great people but it wasn't as small a core and we had to do more stuff, right?"](https://www.youtube.com/watch?v=xi1Lq79mLeE&t=6163s) ― Dave Cutler
- On the designers behind Micah (not to be confused with DEC Mica, another operating system Dave Cutler led the development of), Microsoft's codename for the OS/2 personality of Windows NT: ["A lot of people that were on the design team had almost zero experience with actually building an operating system"](https://www.youtube.com/watch?v=xi1Lq79mLeE&t=9728s) ― Dave Cutler
- ["Unlike other companies, Microsoft never developed a true system for innovation. Some of my former colleagues argue that it actually developed a system to thwart innovation. Despite having one of the largest and best corporate laboratories in the world, and the luxury of not one but three chief technology officers, the company routinely manages to frustrate the efforts of its visionary thinkers."](https://www.theguardian.com/technology/2010/feb/05/microsoft-innovation) ― Dick Brass, former corporate vice president at Microsoft
  - It seems like part of the problem at Microsoft is that there are too many visionaries and not enough doers

## [Happy 50th Anniversary, Microsoft!](https://apnews.com/article/microsoft-build-israel-gaza-protest-worker-fired-a395ac137b74002886b2ad727b5ae5c2)

- Let's take the time to look back fondly on the [disgusting circle jerk book of lies](https://www.amazon.ca/Inside-Out-Microsoft-Our-Words/dp/0446527394) that Microsoft put out on their 25th anniversary

## [Bill Gates Has Repeatedly Advocated for Policies that Help Big Pharma Monopolists Instead of Providing People with Lifesaving Drugs](https://www.wired.com/story/opinion-the-world-loses-under-bill-gates-vaccine-colonialism/)

- "On top of steering the global health community towards Covax rather than patent-free technology sharing, last year Gates bragged about convincing Oxford University not to open-license its vaccine. Gates leveraged his $750 million donation to the university for vaccine research—even though its vaccine was developed in a publicly funded lab."
- Way to show your true colors, Bill

## [Microsoft's Office Open XML (OOXML) format Is An Attempt to Supplant Open Document Format (ODF), a Real Open Office Standard](https://archive.fosdem.org/2022/schedule/event/lotech_odfbetterthanooxml/)

- [OOXML came out in 2006 as a reaction to ODF releasing a year earlier in 2005](https://en.wikipedia.org/wiki/Comparison_of_Office_Open_XML_and_OpenDocument) because Microsoft was scared they might lose their office suite's vendor lock-in (users might recognize the transition to OOXML as the office file extensions gaining an `x` at the end like: `.doc` turned to `.docx`, `.ppt` to `.pptx`, and likewise)
- The OOXML file format standard is disgusting and horribly inconsistent from a technical point-of-view
- Microsoft keeps lying by claiming that current Microsoft 365 office programs produce documents that are compliant to the OOXML format by default when they do not. There are also many proprietary extensions conveniently left out of the standard that only Microsoft office programs know how to use.
- Why has Microsoft not received more serious antitrust scrutiny over this move yet? Apparently, Microsoft has done a lot to pay their way. A high up court of law in the US, the EU, or elsewhere needs to see the OOXML format for what it is then ban it for being anti-competitive. Next, Microsoft should be mandated to move all their Office programs to using ODF. If Microsoft tries to pay their way again then keep extracting their money and set up the verdict so another Justice can re-raise the case soon after. In an ideal scenario, everybody wins except Microsoft and especially there many unethical decision makers.

## [Windows 11 Broke Dual-Booting Into Some Linux Distros Then Conveniently Took Nine Months to Fix the Issue](https://www.neowin.net/news/linux-windows-11-dual-boot-to-finally-play-well-as-microsoft-fixes-nine-month-old-bug/)

- This is so obviously the same FUD techniques Microsoft employed with the AARD code to take out DR-DOS. A blind man could see that.
- Of course, Windows is already leveraging their control of secure boot to do enact any nasty business practice they can
- It really does suck that Microsoft deserves all the detestation it gets and more

## [Microsoft Moves to Extinguish Chrome](https://www.youtube.com/watch?v=NUSfeQKhZYE&t=10s)

- Remember when Microsoft Edge moved to the Chromium engine and many people, including Google officials, rejoiced about how what a [nice](https://www.windowslatest.com/2020/11/19/new-data-proves-microsoft-is-the-best-thing-to-happen-to-chrome/) and [technologically progressive](https://www.theverge.com/2018/12/6/18129287/google-microsoft-edge-chromium-response) [move this was by Microsoft](https://learn.microsoft.com/en-us/shows/one-dev-minute/will-microsoft-contribute-back-to-the-chromium-project--one-dev-question)? Yeah, that time is over.
- Microsoft's old AARD code playbook is coming into effect!
- If I was a Google official for Chrome, I would acknowledge this action by Microsoft for the blatant anti-competitive sabotage that it is then sue Microsoft before it is too late
  - It could also be argued that Google has a monopoly over web browsers which gives them an unfair advantage in the web browser market, but that does not make the actions of decision makers at Microsoft any better. At least Google got there by engineering a technologically robust piece of open source software. Whereas Microsoft cheated and has historically been responsible for the most collateral damage to the web of any company with Internet Explorer. It is justifiable to say that Microsoft is a much more nefarious company than Google.

## Cookie Stuffing Malware in Microsoft Edge

- [Microsoft ran an affiliate marketing commision scam natively in their web brower](https://www.youtube.com/watch?v=wwB3FmbcC88&t=2796s), stealing money from creators who make a living on affiliate commissions and consumers when business have to raise product prices due to thieving companies taking a cut of every sale
- [On March 11, 2025, Google launched Chrome Web Store policy updates to curb affiliate marketing commission scams in their own Chrome Web Store](https://developer.chrome.com/blog/cws-policy-update-affiliate-ads-2025); however, Microsoft was unaffected by this policy change because they have their own Edge web browser (a browser that gained popularity due to being bundled with Windows). Also, Microsoft has their own Chrome Web Store clone, the the Edge Add-ons store.
- Microsoft shut down their [cookie stuffing malware](https://en.wikipedia.org/wiki/Cookie_stuffing) on [May 31, 2025 only after creators filed lawsuits against Microsoft for engaging in this practice](https://hellopartner.com/2025/05/30/microsoft-discontinues-coupon-browser-extension-amid-legal-disputes/)

## [Bonus](https://en.wikipedia.org/wiki/Criticism_of_Microsoft)

- Here's a Wikipedia page full of extra Microsoft and Windows criticisms, in case you were ever in need of some more
