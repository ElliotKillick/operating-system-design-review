# Legacy Software Analysis

Let's document some processes for analyzing legacy software.

## Operating Systems

Here are the sources along with instructions for downloading and accessing the contents of different legacy Windows builds, we rely on disk images or boot disks so no Windows installation is necessary to analyze the contents.

### Sources

- [Windows 3.1 (1992)](https://archive.org/details/windows31vb)
- [Windows NT 3.1 Early Pre-Release (1991)](https://archive.org/details/nt-september-1991)
  - This September 17, 1991 build appears to be the earliest leaked build that is publicly available
  - In 1991, to prepare for the launch of Windows NT, Microsoft began distributing pre-release builds to select partners so they could ensure their applications would work on the new operating system. Some of these builds got leaked.
  - [Cross-referenced October, 16 1991 build](https://winworldpc.com/product/windows-nt-3x/nt-os-2)
- [Windows NT 3.1 (1993)](https://archive.org/details/windows-nt-3.1_202208)
- [Windows NT 4.0 (1996)](https://archive.org/details/winnt4vb_202010)
- [Windows 98 (1998)](https://archive.org/details/win98vb)
- [Windows 2000 (2000)](https://archive.org/details/windows2000virtualbox)

### Instructions

Once the desired sources are downloaded, use the [`get-raw-disk-images.sh`](/code/windows/scripts/legacy-software-analysis/get-raw-disk-images.sh) shell script to create raw disk images from the hypervisor vendor-specific disk image formats, or for ISO files keep them as is.

Then, simply open the resulting files in a GUI to access the image contents. Alternatively, proceed manually by creating a loop device and mounting it on the command-line using the `losetup` and `mount` commands, or the `udisksctl` command on systemd machines.

## Operating System SDKs and WDKs

Here are our Windows software development kit (SDK) and Windows driver kit (WDK) sources:

- [Windows 95 and Windows NT 4.0 SDK](https://archive.org/details/nt40_win95_sdk1996)
- [Windows 98 SDK](https://archive.org/details/msdn-development-platform-june-1998-disc-14)
- [Windows 2000 RC2 SDK](https://archive.org/details/ms-win2k-platsdk-rc2)
- [Windows 2000 WDK](https://archive.org/details/msdn-disc7-february-2000-x05-48786)

Microsoft has shuffled around what they call their SDKs and WDKs over time. For instance, a WDK used to be known as a driver development kit (DDK) in pre-Vista Windows versions. We will use the modern names.

## Running 16-bit Windows Apps on a 64-bit System

Microsoft made Windows on Windows (WoW, which later became WoW16 when Microsoft released Wow64) to enable running 16-bit software on 32-bit Windows. This support does not extend to 64-bit Windows. WoW16 is a Windows GUI compatibility layer that depends on the [NT Virtual DOS Machine (NTVDM)](https://learn.microsoft.com/en-us/windows/compatibility/ntvdm-and-16-bit-app-support) for 16-bit DOS program compatibility. Since 64-bit Windows dropped 16-bit application support, we must use an alternative solution to run these applications:

- [WineVDM](https://github.com/otya128/winevdm)
  - Can run 16-bit Windows and DOS applications on 64-bit Windows
- [NTVDMx64](https://github.com/leecher1337/ntvdmx64)
  - Can run DOS applications on 64-bit Windows
  - No longer working on up-to-date Windows 11 builds

Not all 16-bit software will work through the 16-bit compatability layer. Some applications break, other applications can work but pop up error messages that may be okay to ignore.
