.\" $Copyright:	$
.\" Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
.\" Sequent Computer Systems, Inc.   All rights reserved.
.\"  
.\" This software is furnished under a license and may be used
.\" only in accordance with the terms of that license and with the
.\" inclusion of the above copyright notice.   This software may not
.\" be provided or otherwise made available to, or used by, any
.\" other person.  No title to or ownership of the software is
.\" hereby transferred.
...
.\" $Header: 3.t 1.18 87/07/29 $
.\"
.Ct Section 3 "System Building Process"
.Pa
In this section,
we discuss the steps necessary to build a bootable system image.
This procedure is generally the same whether or not your system includes
DYNIX kernel source code.
Source customers should refer to the
documentation provided with the source tape.
.Pa
Under normal circumstances there are six steps in building a system:
.Ls 1
.Li
Create a kernel configuration file for the system
.Li
If you are adding or modifying a device driver,
verify that the files that describe the device configuration
and the device driver are correct
.Li
Make a directory in which the system is to be constructed
.Li
Run
.I config
on the kernel configuration file to generate the files required
to compile and link the system image
.Li
Construct the source code interdependency rules for the
configured system
.Li
Build the system with 
.I make (1)
.Le
.Pa
Steps 1\-3 are usually done only once.
When you changed a system configuration,
you can usually run
.I config
on the modified configuration file, rebuild the source code dependencies,
and remake the system.
However, some configuration dependencies may not be noticed,
in which case it is necessary to clean out the relocatable object files saved
in the system's directory;
this is discussed later.
.Bh "Create a kernel configuration file"
.Pa
Kernel configuration files normally reside in the directory
.I /sys/conf .
A configuration file is most easily constructed by copying an
existing configuration file and modifying it.  The DYNIX distribution
contains the configuration file used to build the kernel
shipped with your system,
.I /sys/conf/DYNIX ,
and a generic configuration file,
.I /sys/conf/GENERIC .
.Pa
The configuration file must have the same name as the directory in
which the configured system is to be built.  
Further,
.I config
assumes this directory is located in the parent directory of
the directory in which it is run.
For example,
the generic system has a configuration file
.I /sys/conf/GENERIC
and an accompanying directory named
.I /sys/GENERIC .
In general,
it is unwise to move your configuration directories out of
.I /sys ,
as most of the system code and the files created by
.I config
use pathnames of the form
.I "\.\./" .
.Pa
When building your configuration file, be sure to include the items
described in Section 2.
In particular,
you must specify the
.I "machine type" ,
.I "CPU type" ,
.I "timezone" ,
.I "system identification" ,
.I "maximum number of users" ,
and
.I "root file system location" .
The specification of the hardware configuration may take a bit of work,
particularly if your hardware is configured at non-standard places,
such as device registers located at site-specific addresses.
Section 4 of this document
gives a detailed description of kernel configuration file syntax,
and Section 5 explains a sample configuration file.
If the devices to be configured are not already
described in one of the existing configuration files, you should check
the manual pages in Section 4 of the
.I "DYNIX Programmer's Manual" .
For each
supported device, the manual page
.B SYNOPSIS
entry gives a sample configuration line.
.Bh "Modify device driver files"
.Pa
In the kernel build process,
.I config
consults the following files in addition to
the kernel configuration file:
.Ls B
.Li
.I /sys/conf/devices.balance
\(em
This file maps each device driver to one or more major device numbers
and specifies the presence or absence of each standard routine, such as
.I open
or
.I close ,
in the device driver.
.I Config
uses this file to build a UNIX-style
.I conf.c
file that specifies the kernel's
.I bdevsw
and
.I cdevsw
structures.
If you are adding a new device driver to the kernel,
you will need to add a line to
.I devices.balance
that describes your device driver.
The format of this file is described in Section 6.
.Li
.I /sys/conf/files
and
.I /sys/conf/files.balance
\(em These files list all the source files that make up the DYNIX kernel.
.I /sys/conf/files
lists files that are machine and architecture independent, and
.I /sys/conf/files.balance
lists files that are specific to the Balance.
If you are adding a device driver,
you will need to list its source file and binary configuration file in
.I files.balance .
The format of this file is described in Section 6.
If the new device driver is intended to be used only with a particular
configuration, such as
.B GRUMPY ,
you can list the device driver's files in a separate
.I files.GRUMPY
file.
For other ways to designate configuration-specific files, refer to  Section 6.
.Pa
.Li
.I /sys/conf/controllers.balance
\(em This file describes all valid controller types to the 
.I config
utility.
It contains the
necessary keyword definitions that are used by
.I config
to determine the validity of a
.I device
specification line.
You do not need to modify this file to add a controller type supported
by Sequent, such as a MULTIBUS adapter board, SCED board,
or dual-channel disk controller.
This file needs to be modified only if you are adding a non-standard
controller to the system bus.
For example, if you built a VME bus
interface controller, you
would need to add an entry.
If you then added a controller to the VME
bus, you would only need to add a
.I device
line to the configuration file.
.Pa
Even if you are not adding a device driver,
you may need to modify a device driver's
binary configuration file to ensure compatibility with
your kernel configuration file.
Binary configuration files reside in the
.I /sys/conf
directory,
and have names of the form
.I conf_xx.c ,
such as
.I conf_sd.c
for the
SCSI disk controller device driver,
.I sd .
Refer to the manual page for the appropriate device driver, such as
.I sd (4),
for information on how and when to modify the binary configuration file.
.Bh "Make directory"
.Pa
Before you run
.I config ,
you must decide within which directory you are going to run it.
If the directory does not exist,
you must create it and change directories to the newly created directory,
as in the following example:
.Ps
% mkdir /sys/GRUMPY
% cd /sys/GRUMPY
.Pe
.Bh "Run config"
.Pa
Once the configuration file is complete, run it through
.I config .
If your system includes the sources for all the files that make up the
kernel, include the
.B -src
option in the
.I config
command line.
As
.I config
executes, watch for any errors.  Never try to use a system about which
.I config
has complained; the results are unpredictable.
For the most part,
.I config 's
error diagnostics are self explanatory.
The line numbers given with the error messages may be off by one.
.Pa
A successful run of
.I config
on your configuration file generates a number of files in
the configuration directory.
These generated files are the following:
.Ls B
.Li
.I Makefile ,
which is used by
.I make (1)
in compiling and linking the system.
.Li
One file for each possible system image for your machine,
which describes where swapping, the root file system, and other
miscellaneous system devices are located.
Each file has a name of the form
.I swapX.c ,
where
.I X
is the name of the system image,
such as
.I swapdynix_zd.c .
.Li
.I ioconf.c ,
which contains the I/O configuration tables used by the system during its
.I autoconfiguration
phase.
.Li
.I conf.c ,
which maps major device numbers to device driver routines.
.Le
Unless you have reason to doubt 
.I config ,
or are curious how the system's autoconfiguration scheme
works, you should never have to look at any of these files.
.Bh "Construct the source code dependencies"
.Pa
When 
.I config
is done generating the files needed to compile and link your system, it
terminates with a message of the following form:
.Ps
Don't forget to run make depend
.Pe
This is a reminder that you should change your current working directory
to the configuration directory for the system just configured and run
.I make
with the target
.B depend ,
such as in the following example:
.Ps
% cd /sys/GRUMPY
% make depend
.Pe
This ensures that any changes to a piece of the system
source code results in the proper modules being recompiled
the next time you run
.I make .
.Pa
This step is particularly important if your site makes changes
to the system include files.  The rules generated specify which source code
files are dependent on which include files.  Without these rules,
.I make
can not recognize when it must rebuild modules due to a system header file
being modified.
.Bh "Build the system"
.Pa
The makefile constructed by
.I config
should allow a new system to be rebuilt by entering:
.Ps
% make \f2image-name\fP
.Pe
For example, if your bootable system image is
.I dynix_zd ,
then
.I "make dynix_zd"
generates the bootable image
.I dynix_zd .
Alternate system image names
are used when the root file system location or swapping configuration
is done in more than one way.  The makefile that
.I config
creates has entry points for each system image defined in
the configuration file.
Thus, if you have configured
.I dynix_sd
to be a system with the root file system on an
.I sd
device and
.I dynix_xp
to be a system with the root file system on an
.I xp
device, then
.I "make dynix_sd dynix_xp"
generates a binary image for each.
You can also enter
.I "make all" ,
which builds all the images listed in the configuration file.
.Pa
Note that the
.I image-name
is different from the
.I "system identification" .
All bootable images are configured for the same system;
only the information about the root file system and paging devices differ.
This is described in more detail in Section 4.
.Pa
The last step in the system building process is to rearrange certain commonly
used symbols in the symbol table of the system image;
the makefile generated by 
.I config
does this automatically for you.
This is advantageous for programs such as
.I ps (1)
and
.I vmstat (1),
which run much faster when the symbols they need are located at
the front of the symbol table.  
Remember also that many programs expect
the currently executing system to be named
.I /dynix .
If you install a new system and name it something other than
.I /dynix ,
many programs are likely to give strange results.
.Tc
