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
. \" $Header: 6.t 1.23 87/07/31 $
.Ct Section 6 "Adding New System Software"
.Pa
This section provides miscellaneous information for
people who intend to install new device drivers or
other system facilities.
This section is broken into two parts:
.Ls B
.Li
General guidelines to be followed in modifying system code
.Li
Explanations of two files used by
.I config
that you must modify when adding new device driver software
.Le
.Pa
The first part is for use principally
with systems that include source files for the DYNIX kernel
and utilities.
.Bh "Modifying system code"
.Pa
If you wish to make site-specific modifications to the system,
it is best to bracket them with the following pre-processor directives:
.Ps
#ifdef SITENAME
\&...
#endif
.Pe
.Pa
This allows your source to be easily distributed to others,
and simplifies
.I diff (1)
listings.
Whether or not you use a source code control system,
such as RCS,
we recommend you save the old code with something of the form:
.Ps
#ifndef SITENAME
\&...
#endif
.Pe
.Pa
We recommend that you
try to isolate your site-dependent code in separate files.
These files may be configured with pseudo-device specifications.
.Pa
Use
.I lint
periodically if you make changes to the system.
The following is an example of how to lint the kernel:
.Ps
$ cd /sys/DYNIX
$ make \-k depend lint > lint.out 2>&1 &
.Pe
.Pa
The following example is for
.I csh (1)
users:
.Ps
% cd /sys/DYNIX
% make \-k depend lint >& lint.out &
.Pe
.Pa
This procedure is well worth the time invested if
.I lint
turns up a potential problem.
.Ns N
This feature is supported for source customers only.
.Ne
.Bh "Adding device drivers to DYNIX"
.Pa
This section describes the formats of two files used by
.I config
that you must modify when you add a new device driver.
In addition,
the
.I "Balance Guide to Writing Device Drivers"
provides further information for the experienced device driver writer.
.Ch "/sys/conf/files.balance"
.Pa
The file
.I files.balance
lists those kernel source files,
including device driver source files,
that are largely dependent on the Balance architecture,
which includes the Symmetry series machines.
A typical device driver is represented by two entries in
.I files.balance :
one for the device driver source file
and one for the binary configuration file.
For example,
the following are the entries for the
.B sd
device driver:
.ta \w'conf/conf_sd.c\0\0\0\0\0'u
.Ps
sec/sd.c	optional sedit sd device-driver
conf/conf_sd.c	optional sd src
.Pe
.Pa
The meaning of these entries is thus:
.Ls B
.Li
In accordance with the naming policies described in the
.I "Balance Guide to Writing Device Drivers" ,
the device driver source file is named
.I sd.c
and resides with other SCSI device drivers in the directory
.I /sys/sec .
The binary configuration file is
.I /sys/conf/conf_sd.c .
Notice that all pathnames are relative to the
.I /sys
directory.
.Li
The attribute
.B "optional sd"
specifies that the files are to be linked into the kernel only if
.I sd
devices are specified in the kernel configuration file.
.\" Note that other flags may be specified between the ``optional'' flag
.\" and the name of the device.
Note that the device driver name that appears in the name
of a source file need not match the device name specified by the
.B optional
flag.
The
.B xp
driver supports
.B xy
disk controllers as in the following example:
.Le
.ta \w'conf/conf_sd.c\0\0\0\0\0'u
.Ps
mbad/xp.c	optional sedit xy device-driver
conf/conf_xp.c	optional xy src
.Pe
.Ls B
.Li
The
.B device-driver
flag specifies that
.I sd.c
is to be compiled with the
.B \-i
option of the
.I cc
command.
This option suppresses potentially hazardous optimizations
in modules that access addresses outside primary memory.
.Li
The
.B sedit
flag causes the Makefile created by
.I config
to run a
.I sed
script on the compiled code before assembling it.
We recommend that you use the
.I sed
script because it improves the performance of the code.
.Li
The
.B src
flag indicates that the
.I conf_sd.c
source file is present,
rather than just the corresponding object file.
For new device drivers, both files should be tagged with the
.B src
flag.
.Le
.\" If your system includes source files for the entire kernel,
.\" you invoke
.\" .I config
.\" with the
.\" .B -src
.\" option, and the ``src'' attribute is assumed for all files.
.Pa
.I Config
also recognizes the following flags:
.Ls B
.Li
.B standard
\(em
The file is linked into the kernel for all configurations.
Files that are not
.B optional
must be declared
.B standard .
.Li
.B fastobj
\(em
Fast execution of the code in this file is critical for
optimal system performance.
The code is placed in each CPU's 8-Kbyte processor-local RAM,
if there is room.
We recommend against using this attribute for device driver code.
.Li
.B config-dependent
\(em
The file is compiled with
.B \-D
options that define the global configuration parameters,
such as
.B timezone
and
.B maxusers .
In particular,
.I /sys/conf/param.c
is compiled using these
.B \-D
options.
.Le
.Pa
These flags should appear in the following order:
.Ls 1
.Li
.B standard
or
.B optional
.Li
any combination of
.B src ,
.B sedit ,
.B fastobj ,
and
.B config-dependent
.Li
the
.I "device name" ,
if
.B optional
is specified
.Li
.B device-driver ,
if required
.Le
.Ch "/sys/conf/devices.balance"
.Pa
This file maps each device driver to one or more major device numbers
and specifies the presence or absence of each standard routine
such as 
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
you will need to add one or more lines to
.I devices.balance
that describe your device driver.
.Pa
The
.I devices.balance
file contains at least one entry for each device driver.
Block device drivers that include a raw interface
have two entries in
.I devices.balance
\(em one block and one character.
If we assume that the
.B nu
device driver is a block driver which also includes the
routines necessary to perform raw I/O, the entries in
.I devices.balance
might look like the ones in the following example:
.KS
.ta \w'\0\0\0M\0\0strategy\0\0\0'u +\w'W\0\0write\0\0\0'u
.Ps
#
# Bdevsw	Cdevsw	Pseudo
# O open	O open	Z zero
# C close	C close	N nodev
# S strategy	R read	U nulldev
# M minphys	W write	T seltrue
# P size	I ioctl
# F flags	S stop
# L select
# M mmap
#
.Pe
.KE
.ta \w'# Name\0\0\0'u +\w'b|c\0\0\0'u +\w'Major#\0\0\0'u
.Ps
## Block Device Specification ##
#
# Name	b\||\|c	Major#	Functions [ optional flags ]
 :
   nu	  b	  7	OCSMNZ
.Pe
.Ps
## Character Device Specification ##
#
# Name	b\||\|c	Major#	Functions
 :
   nu	  c	 15	OCRWISLN
.Pe
The
.I Name
field takes one of three forms:
.Ls B
.Li
A single name,
such as
.B nu
\(em
This is the name of the device driver;
it is found in the
names of the major driver routines and source files.
The device driver is included in the device table only if
a device by that same name appears in the kernel configuration file.
This is the normal situation for SCSI device drivers.
.Li
.I "name/\(**"
\(em
This construct indicates that the named device driver is
always included in the device table.
.Li
.I name1/name2 ,
such as
.B xp/xy
\(em
This construct indicates that the device driver whose name is
.I name1
is included in the device table only if device
.I name2
appears in the kernel configuration file.
In the case of
.B xp/xy ,
the
.B xp
driver is included only if the kernel configuration file lists one or more
.B xy
controllers.
.Le
.Pa
The
.B b
or
.B c
indicates whether the entry describes a block or character device,
respectively.
.Pa
The
.B Major#
field is the major device number.
Usually you will assign the next
available major device number to your new device type to
avoid allocating unused space in the the device tables.
The device tables are only as large as they need
to be to accommodate the highest major device number.
.I Config
generates null entries for major device
numbers that are skipped in the
.I devices.balance
list.
.Pa
The
.B Functions
field specifies what functions the device driver supports.
The first five characters in the
.B Functions
field for block devices indicate the presence (or absence)
of the
.I open (),
.I close (),
.I strat (),
.I minphys (),
and
.I size ()
routines in that order.
The letter representing the driver routine
supporting each function is listed in the slot for that
function.
If the driver does not support a function,
either an
.B N
or a
.B U
is listed in the slot.
An
.B N
causes
.I config
to generate a
.B nodev
entry for this function in the device table.
A
.B nodev
entry means that the device does not support
the function and that attempts to perform the function
on that device result in an error \(em
for example, reading from a line printer.
A
.B U
causes
.I config
to generate a
.B nulldev
entry for this function in the device table.
A
.B nulldev
entry means that the device does not take any
action, but the function does not cause an error,
such as a
.I stop ()
on a tape device.
.Pa
The last character in the
.B Functions
field for block devices indicates whether
any flags can be passed to the driver via the
.I bdevsw
structure.
A
.B Z
indicates that there are no flags,
and an
.B F
indicates that there are one or more flags.
The flags are listed at the end of the
.B Functions
field,
separated by spaces.
Note that these flags are different from the flags that
can be specified in the
.B flags
field of a
.I device
entry in the kernel configuration file.
Very few device drivers make use of the optional
.B flags
field in
.I devices.balance .
.Pa
The characters in the
.B Functions
field for character devices indicate the presence (or absence)
of the
.I open (),
.I close (),
.I read (),
.I write (),
.I ioctl (),
.I stop (),
.I select (),
and
.I mmap ()
routines in that order.
As with block devices, the letter representing the driver routine
supporting each function is listed in the slot for that
function.
.B N
and
.B U
are used for functions that the
driver does not support.
If the device always returns true for a
.I select ()
system call,
you can specify a
.I T
as the last character in the
.B Functions
field instead of writing a driver
.I select ()
routine that always returns true.
.Pa
An entry in
.I devices.balance
can also contain the word
.B mono-processor
following the
.B Functions
field.
This word indicates that the device driver
is designed for a mono-processor system,
such as a driver ported from UNIX 4.2BSD.
The driver is guaranteed to run on a single CPU
selected by the kernel at boot time,
and UNIX-style
.I sleep ()
and
.I wakeup()
operations are supported.
For MULTIBUS devices, the driver must also set the
MBD_MONOP flag in the
.I mbd_flags
field of the driver's
.I mbad_driver
structure.
.Ch "/sys/conf/controllers.balance"
.Pa
This file describes all valid controller types to the 
.I config
utility.
.I Config
uses the information in this file to parse the
.I device
configurable data in the kernel configuration file.
This data is used to construct the data
structure initializations in
.I ioconf.c
for the peripherals attached to that controller type.
You do not need to modify this file to add a controller
type supported by Sequent, 
such as a MULTIBUS adapter,
SCED board,
or dual-channel disk controller.
This file needs to be modified only if you are adding a non-standard
controller to the system bus.
.Tc
