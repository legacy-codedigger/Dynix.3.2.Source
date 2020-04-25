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
. \" $Header: 4.t 1.23 87/07/29 $
.\"
.Ct Section 4 "Configuration File Syntax"
.Pa
In this section, we consider the specific rules used in writing
a configuration file.  A complete grammar for the input language
can be found in Appendix A and may be of use if you encounter
syntax errors.
.Pa
A configuration file consists of three types of parameters and specifications:
.Ls B
.Li
Configuration parameters global to all system images 
specified in the configuration file
.Li
Parameters specific to each
system image to be generated
.Li
Device specifications
.Le
.Bh "Global configuration parameters"
.Pa
The global configuration parameters are the machine type, CPU type,
options, timezone, system identification, and maximum number of users.
Each is specified with a separate line in the configuration file.
.Ls H
.Li "\f3machine\fP \f2type\fP"
The system runs on the machine type specified.
The machine type is based on the architecture of a system.
Only one machine type can appear in the configuration file.
Currently, the only legal machine type is
.B balance .
.Li "\f3cpu\fP \f2type\fP"
The system runs on the CPU type specified.
More than one CPU type specification
can appear in a configuration file.
Currently, the only legal CPU types are
.B NS32000
for
.B Balance
machines and
.B i386
for
.B Symmetry
machines.
.Li "\f3options\fP \f2optionlist\fP"
Compile the system with the listed options.
Options in this list are separated by commas.
Certain options, such as
.B MMU_MBUG ,
allow the kernel to work around known bugs in the processor chip set.
Other options can be used to adjust system tuning parameters from
their default values.
For a list of these tuning parameters and their default values, refer to
Appendix D.
A line of the form
.B "options FUNNY,HAHA"
generates the global definitions
.B "#define \-DFUNNY"
.B "#define \-DHAHA"
in the resultant makefile.
An option may be given a value by following its name with
.B =
(optionally enclosed in double quotes).
There may be additional options which are associated with certain
peripheral devices; those are listed in the
.B SYNOPSIS
section of the manual page for the device. 
.Li "\f3maxusers\fP \f2number\fP"
The maximum expected number of simultaneously active users on this system is
.I number .
This number, which should be in the range from eight to 512, 32, or 64,
depending on whether the COMMERCIAL, PARALLEL, or TIMESHARE option is set,
respectively,
is used to size several system data structures,
as described in Appendix D.
.Li "\f3timezone\fP \f2number\fP [ \f3dst\fP [ \f2number\fP ] ]"
Specifies within which timezone you are.
This is based on the number of hours west of Greenwich Mean Time (GMT)
your timezone is.  
Eastern Standard Time (EST) is 5 hours west of GMT;
Pacific Standard Time (PST) is 8 hours west of GMT.
Negative numbers indicate hours east of GMT. If you specify
.B dst ,
the system operates under daylight savings time.
You can include an optional integer or floating point number
to specify a particular daylight savings time correction algorithm;
the default value is 1, indicating the United States.
Other values are:
0 (no correction),
2 (Australia),
3 (Western Europe),
4 (Middle Europe),
5 (Eastern Europe),
6 (Canada),
7 (Great Britain),
8 (Rumania),
9 (Turkey),
and
10 (Australia with 1986 shift).
Refer to
.I gettimeofday (2)
and
.I ctime (3)
for further information.
.Li "\f3ident\fP \f2name\fP"
This system is to be known as
.I name .
This must be
.B DYNIX .
.Le
.Bh "System image parameters"
.Pa
Multiple bootable images may be specified in a single configuration
file.  The systems will have the same global configuration parameters
and devices, but the location of the root file system and other
system specific devices may be different.  A system image is specified
with a
.I config
line:
.Ps
config \f2sysname\fP\ \f2config-clauses\fP
.Pe
.Pa
The
.I sysname
field is the name given to the loaded system image,
such as
.B dynix_sd ,
.B dynix_xp ,
or
.B dynix_zd .
The configuration clauses
are one or more specifications indicating where the root file system
is located, how many paging devices there are, and where they go.
.Pa
A configuration clause must be in one of the following forms:
.Ps
root [ on ] \f2root-device\fP
swap [ on ] \f2swap-device\fP [ and \f2swap-device\fP ]
.Pe
.Pa
The
.B on
is optional.
Multiple configuration clauses are separated by white space; 
.I config
allows specifications to be continued across multiple lines
by beginning the continuation line with a tab character.
The
.B root
clause specifies where the root file system is located,
and the
.B swap
clause indicates swapping and paging area(s).
.Pa
The device names supplied in the clauses may be fully specified
as a device, unit, and file system partition; or underspecified,
in which case
.I config
uses built-in rules to select default unit numbers and file system partitions.
The defaulting rules are complicated as they depend
on the overall system configuration.
For example, the swap area need not be specified at all if 
the root device is specified; in this case the swap area is
placed in the
.B b
partition of the same disk where the root file system is located.
Appendix B contains a complete list
of the defaulting rules used in selecting system configuration devices.
.Pa
The device names are translated to the appropriate major and minor device
numbers on a per-machine basis.
The file
.I /sys/conf/devices.balance
is used to map a device name to its major block device number.
The minor device number is calculated using the standard 
disk partitioning rules: on unit 0, partition
.B a
is minor device 0, partition
.B b
is minor device 1, and so on;
for units other than 0,
add 8 times the unit number to get the minor device.
Therefore,
minor device number 35 is found on unit 4, partition c (4 \(mu 8 + 3 = 35).
.Pa
If the default mapping of device name to major/minor device
number is incorrect for your configuration, it can be replaced
by an explicit specification of the major/minor device,
such as in the following example:
.Ps
config dynix_xx root on major 99 minor 1
.Pe
.Pa
Normally, the areas configured for swap space are sized by the system
at boot time.  If a non-standard partition size is to be used for one
or more swap areas, this can also be specified.  To do this, the
device name specified for a swap area should have a
.B size
specification appended,
such as in the following example:
.Ps
config dynix_sd root on sd0 swap on sd0b size 1200
.Pe
.Pa
This forces swapping to be done in partition
.B b
of
.B sd0
and sets the swap partition size to 1200 sectors.
A swap area sized larger than the associated disk partition is trimmed to the
partition size at boot time.
.Ch "Generic configurations"
.Pa
A generic configuration is a kernel that can be run from one of
the devices listed in
.I /sys/conf/conf_generic.c .
To create a generic configuration,
specify the clause
.B "swap generic"
only;
otherwise,
an error is generated.
.Bh "Device specifications"
.Pa
You must specify each device attached to a machine
to
.I config
so the system generated knows to probe for the device during
the autoconfiguration process carried out at boot time.
Hardware specified in the configuration need not be present on
the machine where the generated system is to be run.
Only the hardware found at boot time is used by the system.
.Pa
The specification of hardware devices in the configuration file
parallels the interconnection hierarchy of the machine to be
configured.
A configuration file must indicate which
.I controllers
are present and in which slots they reside.
.LP
A
.I controller
is a peripheral controller that connects directly to the system bus,
such as a SCED board, MULTIBUS adapter board, or dual-channel disk controller
(DCC).
All other peripherals,
such as SCSI disk and tape drives,
MULTIBUS disk and tape controllers, DCC disk drives, and terminal multiplexors,
are
.I devices .
.LP
Similarly,
you must indicate which devices could be connected to one or more controllers.
A device description may provide a
complete definition of the possible configuration parameters
or it may leave certain parameters undefined and make the system
probe for all of the possible values.  The latter allows a single
device configuration list to match many possible physical
configurations.  For example, a disk may be indicated as present
at SCED board 0, or at any SCED board that the system
finds at boot time.  The latter scheme, termed 
.I wildcarding ,
allows more flexibility in the physical configuration of a system;
if a disk must be moved around for some reason, the system can
still find it at the alternative location.
.Pa
A device specification takes one of the following forms:
.Ps
controller \f2controller-name\fP at slot \f2n\fP
device \f2device-name\fP at \f2controller-name device-info\fP
.Pe
.Pa
A
.I controller-name
takes the form
\f3sec\fP\f2n\fP
for SCED boards,
\f3mbad\fP\f2n\fP
for MULTIBUS adapter boards,
or
\f3zdc\fP\f2n\fP
for DCCs,
where
.I n
is a number in the range of 0 to 3, or
.B ? .
In
.I controller
entries,
controllers of a particular type are assigned
consecutive numbers starting at 0,
with 0 specifying the leftmost controller in the Balance card cage
(as you face the rear of the machine).
Wildcard controller names,
which end in
.B ? ,
are used only in
.I device
entries to indicate that the device may be connected to any of two or more
controllers.  The controller name consists of two parts:  the type
of controller as specified in the
.I controllers.balance
file,
and a unit number as described earlier.
.Pa
The
.I controller
entries for a system that may contain up to
two SCED boards, three MULTIBUS adapter boards, and two DCCs would be:
.ta \w'controller\0\0\0'u +\w'mbad2\0\0\0'u
.Ps
controller sec0	at slot ?
controller sec1	at slot ?
controller mbad0	at slot ?
controller mbad1	at slot ?
controller mbad2	at slot ?
controller zdc0	at slot ?
controller zdc1	at slot ?
.Pe
.Pa
In the current version of
.I config ,
all slot numbers must be wildcarded.
.Pa
A
.I device-name
consists of a standard device name,
as found in Section 4 of the
.I "DYNIX Programmer's Manual" ,
concatenated with the logical unit number of the device.
Typically, multiple devices of the same type are assigned
consecutive unit numbers starting at 0.
However, the number in a
.I device-name
does not correspond to any index or other value that
is passed to the device driver.
.Pa
The name used in the kernel configuration file
may be different from the name commonly used in referring to
the device driver itself.
For example, the MULTIBUS disk controller is controlled by
the
.B xp
device driver,
so called because
.B xp
is used in the names of the driver's source files and routines.
However, an entry in the kernel configuration file
for a disk controller driven by this driver uses device names
such as
.B xy0
and
.B xy1 .
In this case, it is
necessary to distinguish between disk controllers,
such as
.B xy0
or
.B xy1 ,
in
.I device
entries
and individual disks,
such as
.B xp0
or
.B xp1 ,
in
.I config
rules.
The links between the driver name,
such as
.B xp ,
and the device name used in the kernel configuration file,
such as
.B xy ,
are established in the files
.I devices.balance
and
.I files.balance ,
as explained in Section 6.
.Ch "MULTIBUS devices"
.Pa
.I Device
entries for MULTIBUS devices should reflect only devices that are
connected directly to the MULTIBUS.
For example, to represent a MULTIBUS disk controller
that can support up to four drives,
you need only one
.I device
entry for the disk controller itself.
The configuration of the individual drives must be specified
in the binary configuration file for the device driver.
.Pa
A
.I device
entry for a MULTIBUS device should specify the following:
.Ls B
.Li
The name of the device, as described earlier.
.Li
The MULTIBUS adapter board to which the device connects,
such as
.B mbad0
or
.B mbad .
.Li
The address of the control status register,
which is the
.I csr
field of the
.I device
entry.
This value must be unique for each MULTIBUS,
and it is recommended that the addresses be unique across MULTIBUSes.
The peripheral address is set on the peripheral hardware using jumpers
or switches (consult the hardware manual associated with the device).
.Li
The number of MULTIBUS mapping registers required by this device
for DMA into Balance system memory,
which is the
.I maps
field.
Each register maps 1 Kbyte of memory;
a total of 256 registers can be allocated for each MULTIBUS.
.Li
The SLIC interrupt bin (0-7) that is to be used when the
device interrupts the Balance CPUs,
which is the
.I bin
field.
Interrupts in higher bin numbers are given higher priority.
When selecting a bin number for a device,
consider the real-time needs of the device and use
the following table as a guide:
.Le
.KS
.TS
center box tab (!) ;
c | c
c | l .
Bin!Interrupt Source
.sp .5
=
7!T{
Reserved for SLIC clock,
other high-priority kernel activities
T}
.sp .5
_
6!Ethernet
.sp .5
_
5!Disk and tape devices
.sp .5
_
4!T{
Terminal multiplexors,
line printers, parallel interfaces,
other slow devices
T}
.sp .5
_
3-1!Unused
.sp .5
_
0!Software
.sp .5
.TE
.KE
.Ls B
.Li
The MULTIBUS interrupt line (0-7) on which the device interrupts,
which is the
.I intr
field.
This value must be unique for each MULTIBUS,
and can usually be set on the peripheral hardware using jumpers
or switches.
Among MULTIBUS interrupt lines,
Sequent CPUs give the highest priority to line 0,
and lowest priority to line 7.
If your device doesn't use interrupts,
specify an
.I intr
number of 255 (0xff);
for such devices, the
.I bin
number is ignored.
.Pa
The entry may also specify a 32-bit
.I flags
value,
to be used as described in the manual page for the device driver.
The following are sample entries for two MULTIBUS controllers:
.Ps
device st0 at mbad? csr 0x200 maps 0 bin 4 intr 3
device xy0 at mbad? csr 0x100 maps 34 bin 5 intr 0 
.Pe
.Pa
The information in each MULTIBUS
.I device
entry is passed to the
.I boot ()
routine of the appropriate device driver in an
.I mbad_dev
structure.
.Ch "SCSI devices"
.Pa
In contrast with the conventions described
for MULTIBUS devices,
SCSI peripheral controllers (target adapters)
do not require
.I device
entries.
Instead, each individual unit,
such as a disk or tape drive,
should have a
.I device
entry that provides the following information:
.Ls B
.Li
The name of the device, as specified earlier.
.Li
The SCED board to which the device connects,
such as
.B sec0
or
.B sec? .
.Li
The target adapter (0-7) to which the unit is connected,
which is the
.I target
field of the
.I device
entry.
.Li
The SCSI logical unit number (0-7) that distinguishes
this unit from other units connected to the same target adapter,
which is the
.I unit
field.
Since the embedded SCSI disks can have only one unit per target adapter,
the SCSI logical unit number is always zero (0) for embedded SCSI disks.
.Li
The SLIC interrupt bin to be used,
as described for MULTIBUS devices.
.Li
The number of elements in the device driver's device input queue
and output queue that must be allocated for this unit,
which are the
.I req
and
.I doneq
fields.
.Le
.Pa
The entry may also specify a 32-bit
.I flags
value,
to be used as described in the manual page for the device driver.
The following are sample entries for two SCSI disk units on a single
target adapter:
.Ps
device sd0 at sec? req 50 doneq 50 bin 5 unit 0 target 6
device sd1 at sec? req 50 doneq 50 bin 5 unit 1 target 6
.Pe
.Pa
The information in each SCSI
.I device
entry is passed to the
.I boot ()
routine of the appropriate device driver in a
.I sec_dev
structure.
.Pa
The
.I target
and
.I unit
numbers may be wildcards,
in which case the autoconfiguration routine probes
for the unit at all indicated device addresses until it is found.
.Ns N
We recommend you do not wildcard the target adapter number.\"Why?
.Ne
.Ch "Disks connected to a DCC"
.Pa
A
.I device
entry for a disk that is connected to a dual-channel
disk controller should specify the following parameters:
.Ls B
.Li
The name of the disk,
such as
.B zd0
or
.B zd31
.Li
The DCC to which the disk connects,
such as
.B zdc0
.Li
The disk unit number (0-7) \(em the
.I drive
field
.Li
The type of disk: 0 for Sequent's 264-Mbyte (Swallow 3) disk,
1 for Sequent's 396-Mbyte (Eagle) disk,
or 2 for Sequent's 540-Mbyte (Swallow 4) disk \(em the
.I drive_type
field
.Le
.Pa
The following are sample entries for two DCC disks:
.Ps
device zd0 at zdc0 drive 0 drive_type ?
device zd1 at zdc0 drive 1 drive_type ?
.Pe
.Pa
The
.I zdc ,
.I drive ,
and
.I drive_type
numbers may all be wildcards.
If the location of a disk is explicitly specified
(anchored) in the kernel configuration file,
as shown in the example earlier,
and there are no wildcarded disks configured before the anchored disk,
the anchored disk can be spun up after the system is booted.
The
.B zd
driver reassesses the status of the disk when the device is first opened.
.Ch "Miscellaneous guidelines"
.Pa
All entries for a particular device type must be contiguous;
for example,
an entry for a
.I ts
device cannot be placed among entries for
.I sd
devices.
.Pa
As dictated by the
.I /sys/conf/controllers.balance
file,
all MULTIBUS devices are probed first, then all SCSI devices,
then all DCC devices.
For each controller type,
devices are probed in the order in which they appear in
the kernel configuration file.
.Pa
When a wildcard SCSI target adapter number is given,
the probe starts at target adapter 7 and works down to 0,
unless the device at target adapter 0 is an embedded SCSI disk,
in which case the probe starts at target adapter 0 and
works up to 7.
When using wildcard controller, target adapter, or unit numbers,
consider the consequences of a device that is present but
fails to respond to probes.
For example,
suppose that you have two
.I sd
disks,
connected as units 0 and 1 to target adapter 6 on SCED board 0.
Now suppose that your
.I device
entries for these disks use wildcard unit numbers as in the following
excerpt from your
.I /sys/conf/DYNIX
file:
.Ps
device sd0 at sec0 req 50 doneq 50 bin 5 unit ? target 6
device sd1 at sec0 req 50 doneq 50 bin 5 unit ? target 6
.Pe
.Pa
If both disks respond to a probe,
.B sd0
will be found at unit 0 and
.B sd1
will be found at unit 1.
Now consider what happens if the disk at unit 0 fails to respond to its probe:
.B sd0
will still be found,
but this time at unit 1.
.Pa
Note that in the case of SCSI devices,
the system autoconfigures sooner if there are no wildcards used in the
.I device
entries of the
.I config
file,
since less searching is required to find the correct devices.
.Bh "Pseudo-devices"
.Pa
A number of drivers and software subsystems
are treated like device drivers without any associated hardware.
To include any of these pieces, a
.I pseudo-device
specification must be used.
A
.I pseudo-device
specification takes the form
.Ps
pseudo-device \f2device-name\fP [ \f2howmany\fP ]
.Pe
.Pa
Examples of pseudo-devices are
.B pty ,
which is the pseudo-terminal driver,
.B pci ,
which is used for Sequent BSS/PC-Vdisk communications,
and
.B pmap ,
which is used to access Atomic Lock Memory and custom memory-mapped
devices.
.Pa
The pseudo-devices
.B ether ,
.B inet ,
and
.B loop
have no corresponding drivers,
but must be included if you are building your kernel
from source files and you want to configure in the indicated facilities.
The pseudo-device
.B inet
is the DARPA Internet protocols.
You must also specify INET in
.B options .
The pseudo-device
.B loop
is the software loopback interface;
the pseudo-device
.B ether
is used by the Address Resolution Protocol on Ethernets.
.Pa
The
.I howmany
parameter is interpreted differently by different
pseudo-devices and is ignored by some.
Its default value is 32 for all pseudo-devices.
Refer to Section 4 of Volume I of the
.I "DYNIX Programmer's Manual"
for descriptions of the pseudo-device drivers.
.Tc
