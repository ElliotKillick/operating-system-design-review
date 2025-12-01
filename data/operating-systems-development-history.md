# Operating Systems Development History

This document provides perspective between distinct systems on the history of key operating system components that are common to modern computers today, as well as the operating systems themselves.

**Disclaimer:** This document was primarily written to give myself context on operating system developments throughout history. I was not alive when most of these events unfolded. Hence, why I am writing this document to gain perspective. With that said, feel free to also refer to this research if you find it helpful.

## Table of Contents

- [Operating Systems Development History](#operating-systems-development-history)
  - [Table of Contents](#table-of-contents)
  - [Disk Operating System (DOS)](#disk-operating-system-dos)
  - [Virtual Address Spaces](#virtual-address-spaces)
  - [Portable Operating System Interface (POSIX)](#portable-operating-system-interface-posix)
  - [Graphical User Interface](#graphical-user-interface)

## Disk Operating System (DOS)

In the broad sense, a [disk operating system (DOS)](https://en.wikipedia.org/wiki/Disk_operating_system) refers to any operating system that resides on a disk storage device such as a floppy disk or hard disk drive. However, DOS typically refers to a family of simple operating systems that are 16-bit, single-user and single-tasking, have a CLI, and basic file/disk management capabilities. DOS systems did not typically offer advanced features such as task scheduling, permissions, and networking that more sophisticated operating environments like Unix-based systems had.

DOS systems that were popular include [CP/M](https://en.wikipedia.org/wiki/CP/M) (1974), [Apple DOS](https://en.wikipedia.org/wiki/Apple_DOS) (1978), [MS-DOS](https://en.wikipedia.org/wiki/MS-DOS) (1981), and [DR-DOS](https://en.wikipedia.org/wiki/DR-DOS) by Novell (1988).

## Virtual Address Spaces

**Windows:** Virtual memory first appeared in [Windows 3.0](https://en.wikipedia.org/wiki/Windows_3.0#Memory_modes) (1990), which supported the 386 Enhanced mode on Intel 386 processors. However, Windows 3.x (including Windows 3.0 and Windows 3.1 non-NT with a [few point release upgrades](https://en.wikipedia.org/wiki/Windows_3.1#Windows_3.11)) only supported using the virtual memory feature for extra memory (i.e. a page file or swap memory). All applications still ran in a single shared virtual address space because the Windows 3.x kernel and all of userland was still 16-bit. It wasn't until Windows NT 3.1 (1993, the first Windows NT release) that the Windows operating system was fully 32-bit and each processes was isolated to its own address spaces. [Windows 95 (1995) was the first DOS-based Windows to support a per-process virtual address space](https://web.archive.org/web/20150619005446/https://support.microsoft.com/en-us/kb/117567) for 32-bit processes, while also still having a single shared address space for 16-bit application compatibility and parts of the operating system that were still 16-bit.

**Unix-like OS:** First appeared in [3BSD](https://en.wikipedia.org/wiki/History_of_the_Berkeley_Software_Distribution#3BSD) (1978) on VAX (this system did not take advantage of the virtual memory capabilites of VAX for swapping to disk, though). On Intel 386, the first Unix-like OS to support virtual address spaces on Intel 386 was [386BSD](https://en.wikipedia.org/wiki/386BSD) (March, 1992).

**Macintosh:** First appeared in [System 7](https://en.wikipedia.org/wiki/System_7) (1991) using the Motorola 68k architecture (Apple only shipped Macintosh with their own hardware and not move to Intel 386 until 2006). While Macintosh is a Unix-like OS, Apple developed their own proprietary GUI solution for their operating system.

**Other:** [OS/2 1.0](https://en.wikipedia.org/wiki/OS/2#OS/2_1.0_(1987)) (1987) supported parts of Intel 386 protected mode through a compatibility layer, but it was limited by its 16-bit architecture. It wouldn't be until [OS/2 2.0](https://en.wikipedia.org/wiki/OS/2#OS/2_2.0_and_DOS_compatibility_(1992)) (April 1992, after the IBM and Microsoft split) that OS/2 would receive full Intel 386 Enhanced mode support (protected mode with per-process virtual address spaces + virtual memory with demand paging + preemptive multitasking).

There were many operating systems written for the Virtual Address eXtension (VAX) instruction set architecture (ISA). These include the original [OpenVMS](https://en.wikipedia.org/wiki/OpenVMS) OS (1977) which was *not* Unix-like and later some Unix-like OSs. VAX was ahead of its time, featuring full 32-bit protected mode support. However, factors such as being closely tied to the non-portable OpenVMS OS, strategic missteps of Digital Equipment Corporation (DEC) like being late to microcomputers with the MicroVAX I releasing in late 1984, the high expense of VAX systems, and competition from other architectures ultimately led to VAX not catching on.

**Academia:** First appeared in the [Atlas](https://en.wikipedia.org/wiki/Atlas_(computer)) (1962), which supported virtual address spaces as part of its virtual memory system. The computer was awarded an [IEEE Milestone award](https://ethw.org/Milestones:Atlas_Computer_and_the_Invention_of_Virtual_Memory,_1957-1962) for inventing virtual memory.

On a 32-bit microprocessor, 386BSD (March, 1992) was impressively the first major operating system to fully utilize the protected mode on Intel 386. This release date beats Windows NT 3.1 by a year. 386BSD was closely followed by  OS/2 2.0 (April, 1992), with only one month between their releases (it's too bad OS/2 was abandoned, Microsoft being the first to leave). On the VAX architecture, OpenVMS (1977) beat everybody to supporting virtual address spaces by a long shot.

## Portable Operating System Interface (POSIX)

The first Portable Operating System Interface (POSIX) release [came in 1988](https://en.wikipedia.org/wiki/POSIX#Parts_before_1997).

Threads in POSIX didn't come until 1995 because Unix architecture favored multiprocessing over multithreading (i.e. a preference for single-threaded applications and minimal processes that used `fork`). The complex nature of threads enabling overlapping operations within the same address space also required special care. So, the IEEE 1003 working group took their time to develop this part of the standard.

## Graphical User Interface

**Windows:** GUI first appeared as a graphical extension to MS-DOS. The first Windows version, [Windows 1.0](https://en.wikipedia.org/wiki/Windows_1.0), was released in 1985 and provided a basic graphical user interface (GUI) on top of MS-DOS. Originally, starting Windows required running the `WIN.COM` program in the MS-DOS command prompt. Later by Windows 95 (still based on MS-DOS), Windows started by default so manually running the `WIN.COM` program was no longer necessary. Windows Me was the last version of Windows based on MS-DOS.

Windows NT (New Technology) was introduced in 1993 as a separate line of operating systems built from the ground up. Unlike the DOS-based Windows versions, Windows NT was designed as a fully 32-bit, multi-user, and multitasking operating system with a focus on security and stability. All Windows operating systems from Windows NT 3.1 onward are based on the NT kernel and components. Windows NT 3.1 (1993) was the first Windows operating sytem based on the NT kernel. The GUI has become part of the Windows API with facilities we know today like `CreateWindow`, `GetMessage`, and the message loop. In Windows NT 4.0, Microsoft sacrificed some stability and security for performance by moving the GDI and USER subsystems (in charge of graphics and windowing tasks) from user-mode to kernel-mode (`win32k.sys`).

Windows 1.0 had color support; however, it was limited to a 4-bit color (16 colors) due to its reliance on the [IBM EGA](https://en.wikipedia.org/wiki/Enhanced_Graphics_Adapter#Color_palette) graphics adapter (or less colors at the same time if the older IBM CGA was used). [Windows 3.0 intoduced 256 possible colors](https://www.os2museum.com/wp/antique-display-driving/) on supporting [VGA graphics hardware](https://en.wikipedia.org/wiki/Video_Graphics_Array#Color_palette). [Windows NT 3.1 added SVGA support](https://en.wikipedia.org/wiki/Windows_3.1#Windows_3.1) thus [providing 24-bit color](https://en.wikipedia.org/wiki/Super_VGA#Specifications), at which point the number of supported colors mostly depended on a system's graphics hardware.

[Try out Windows 1.0 yourself here](https://www.pcjs.org/software/pcx86/sys/windows/1.01/ega/) (or on [v86](https://copy.sh/v86/?profile=windows1)), I recommend daily driving it for a week before making the switch.

Note that [Windows 3.1](https://en.wikipedia.org/wiki/Windows_3.1) (1992) and [Windows NT 3.1](https://en.wikipedia.org/wiki/Windows_NT_3.1) (1993) are distinct operating systems. Windows 3.10 received a few minor point upgrades, which a group are referred to as Windows 3.1x (still on MS-DOS, not NT). You can use [86Box](https://86box.net) or [v86](https://copy.sh/v86/?profile=windowsnt3) to easily run Windows NT 3.1.

**On Unix-like OSs:** The [X Window System](https://en.wikipedia.org/wiki/X_Window_System) first appeared in 1984. The X Windows System initially [received color support in 1985](https://en.wikipedia.org/wiki/X_Window_System#Release_history). [Graphic hardware support for color](https://www.x.org/wiki/X11R1/) and a [mature color scheme](https://en.wikipedia.org/wiki/X11_color_names) gradually improved over X releases.

**Macintosh**: The first Macintosh system to have a GUI was the Apple Macintosh 128K (hardware), which came with [System 1](https://en.wikipedia.org/wiki/System_1) (1984). However, color didn't come until the Macintosh II with [System 4](https://en.wikipedia.org/wiki/Classic_Mac_OS#System_1,_2,_3,_and_4) (1987).

**Other:** [Amiga OS](https://en.wikipedia.org/wiki/AmigaOS#AmigaOS_1.0_%E2%80%93_1.4) (1985, just before Windows 1.0 released) had a GUI with color support.

Windows NT 3.1 (1993) was well received and posed major competition to the largest market share holding Apple System 7 (1991) desktop consumer operating system. The X Window System on Unix-like operating systems was mostly used for academic, research, and certain commercial purposes.
