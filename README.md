# Novell Netware SMP console screensaver for X11, Gnome, and KDE Desktops

## Description

The _netwaresmp_ screensaver written by Jeffrey Merkey &lt;jeffmerkey@gmail.com&gt; and Cosimo Streppone &lt;cosimo@cpan.org&gt;, based on PopSquares, Deco hacks, loadsnake, and Novell's Netware SMP.  The original Netware SMP screensaver was written by Jeffrey Merkey at Novell in 1994.  This version is a rewrite of the original which functions exactly the same way as it did in Netware SMP.  Each worm represents a running system cpu processor.  As system utilization increases for each cpu, the corresponding worm for that processor gets longer and moves more quicky across the screen.  As overall system load increases, all the worms slightly increase in speed.  This screensaver is very useful for monitoring cpu loading in a system since you can visually see processor utilization and loading by simply observing the worms. 

## Quick Start Guide

You can download precompiled RPM packages which contain the entire XScreensaver program with netwaresmp integrated from
the [release page](https://github.com/jeffmerkey/netware-xscreensaver/releases) for this project, or you can build the 
program from the git sources.  The current netwaresmp xscreensaver module is compiled against the XScreensaver v6.08 base 
and you may have to deinstall any older version of the Xscreensaver program you are running and reinstall with this RPM.  

If you want to build the program from scratch with the git sources rather than use the pre-compiled RPMS packages, then please skip to the section [Building the Screensaver from Source](#building-the-screensaver-from-source) for instructions on how to do this. 
 
Packages are provided in binary and source versions, and can be downloaded and 
installed directly or rebuilt for a different target architecture such as ARM64. Package types are Red Hat Package Manager (RPM) packages for binary installation and Source RPM packages (SRPMS) for source code installation.  

RPM packages for each release include a binary architecture specific package
and a source package which can be downloaded and built/rebuilt and which contains the source code.

For example, the release v6.08-3 contains the following packages in the release section:

### **RPM Based Packages (RedHat, CentOS, SuSe)**

- [xscreensaver-netwaresmp-6.08-3.src.rpm](https://github.com/jeffmerkey/netware-xscreensaver/releases/download/v6.08-3/xscreensaver-netwaresmp-6.08-3.src.rpm)
- [xscreensaver-netwaresmp-6.08-3.x86_64.rpm](https://github.com/jeffmerkey/netware-xscreensaver/releases/download/v6.08-3/xscreensaver-netwaresmp-6.08-3.x86_64.rpm)

#### **Removing a Previous Installation of Outdated Versions of XScreensaver**

Most of the XScreensaver packages shipped or distributed by RedHat, Debian, and many Distros of Linux are quite old
and outdated and do not contain current bug fixes or support and should be upgraded in any event.  

To remove a previously installed or outdated version of XScreensavere with the rpm package manager for x86_64:
```sh
rpm -e xscreensaver
```

### **Installing Binary Packages**

To install the binary package with the rpm package manager for x86_64:
```sh
rpm -i xscreensaver-netwaresmp-6.08-3.x86_64.rpm
```

To deinstall the RPM binary package:
```sh
rpm -e xscreensaver-netwaresmp
```

### **Installing Source Packages**

To install the source package with the rpm package manager:
```sh
rpm -i xscreensaver-netwaresmp-6.08-3.src.rpm
```
*(Note: rpm installs the source code files in /root/rpmbuild/ as top directory for RedHat and CentOS
platforms.  SuSe platforms install the source code files in /usr/src/packages/)*

For building or rebuilding RPMS or DEB Packages after you have installed the associated source packages on your platform, refer to the following:

- [Building the Screensaver as an RPM Package (Redhat/CentOS/SuSe)](#building-as-an-rpm-package)

## netwaresmp Options

```
netwaresmp [-cpus  number of cpus] [-speedup  divisor] [-wormsize  pixels] 
[-display host:display.screen] [-foreground color] [-background color] 
[-window] [-root] [-mono] [-install] [-visual visual] [-delay seconds] 
[-max-depth int] [-min-width int] [-min-height int] [-cycle] [-no-cycle] 
[-cycle-delay]
```

## Command Line Options

_netwaresmp_
accepts the following options:

* **-cpus**  
  Number of cpus to start.  The default is the actual number of cpus detected from /proc/cpuinfo.  If you specify more cpus than are installed in your system, only the worms that are mapped to an actual processor will grow longer and faster when cpu load increases for that processor.  If you specify less cpus than are installed in your system, the program will default the display to the actual detected cpus.  
* **-speedup**  
  speedup divisor to increase worm speed.  functions as powers of 2 i.e. speedup=4 runs at 4 times the speed, etc.  Default value is 1.
* **-wormsize**  
  size of the worm head in pixels in a range between 10 and 80.  Default value is 20.  Can be any value but works best if you use multiples of 2 to map to graphics screen dimensions (which are typically numbers which are multiples of 2 or 10).
* **-window**  
  Draw on a newly-created window.  This is the default.
* **-root**  
  Draw on the root window.
* **-mono**  
  If on a color display, pretend we're on a monochrome display.
* **-install**  
  Install a private colormap for the window.
* **-visual _visual_**  
  Specify which visual to use.  Legal values are the name of a visual class,
  or the id number (decimal or hex) of a specific visual.
* **-delay _seconds_**  
  How long to wait before starting over.  Default 5 seconds.
* **-max-depth _integer_**  
  How deep to subdivide.  Default 12.
* **-min-width _integer_**  
  **-min-height _integer_**
  The size of the smallest rectangle to draw.  Default 20x20.
* **-cycle**  
* **-no-cycle**  
  Whether to do color cycling.  Default False.
* **-cycle-delay _usecs_**  
  If color cycling, how often to change the colors.  Default 1000000,
  or 1 second.

## Environment

* **DISPLAY**  
  to get the default host and display number.
* **XENVIRONMENT**  
  to get the name of a resource file that overrides the global resources
  stored in the RESOURCE_MANAGER property.

## Copyright

Copyright © 1994-2024 by Jamie Zawinski, Jeffrey Merkey, Cosimo Streppone, 
and others.  Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee, provided 
that the above copyright notice appear in all copies and that both that 
copyright notice and this permission notice appear in supporting documentation. No representations are made about the suitability of this software for any 
purpose.  It is provided "as is" without express or implied warranty.

## Authors

Jeffrey Merkey &lt;[jeffmerkey@gmail.com](mailto:jeffmerkey@gmail.com)&gt; and Cosimo Streppone &lt;cosimo@cpan.org&gt;, 
based on PopSquares, Deco hacks, loadsnake, and Novell's Netware SMP.  The 
original Netware SMP screensaver was written by Jeffrey Merkey at Novell in
1994.  

## Building as an RPM Package

In order to build the screensaver as an RPM, the program must be compressed into a tar.gz
file and the tar.gz file named to match the versioning information contained in the 
associated .spec file.  

Spec files are special files which contain instructions on how to build a particular package
from a source code archive.  On Red Hat and CentOS systems, RPMS are built in the /root/rpmbuild/
 top directory.  SuSe systems build RPMS in the /usr/src/packages/ as top directory.  These 
"top directories" will contain BUILD, BUILDROOT, SPECS, RPMS, SRPMS, and SOURCES subdirectories.  

The SPECS directory contains the \*.spec files used to build RPMS and SRPMS packages.  The SOURCES subdirectory will contain the soure code archive file referred to in the \*.spec file used to build the 
RPM package.

See the [Quick Start Guide](#quick-start-guide) on instructions for installing the 
source rpm which installs both the .spec file and source archive file (tar.gz) into 
the rpm build top directory (i.e. /root/rpmbuild/).  You should have previously 
installed the src.rpm file before attempting to build the rpm.  You can also 
manually install the .spec file into the \<top directory\>/SPECS/ directory and 
the source code tarball in the \<top directory\/SOURCES/ directory, then attempt 
to build the rpm package.

To build the Screensaver using the rpm tool, change directories (cd) into the /root/rpmbuild/SPECS/ directory (/usr/src/packages/SPECS/ for SuSe) and enter the following command:

```sh
rpmbuild -ba xscreensaver-6.8.spec <enter>

```
