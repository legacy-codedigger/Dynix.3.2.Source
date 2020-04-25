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
. \" $Header: 2.t 1.19 87/07/29 $
.Ct Section 2 "Configuration File Contents"
.Pa
A system configuration must include at least the following information:
.Ls B
.Li
Machine type
.Li
System identification
.Li
CPU type
.Li
Timezone
.Li
Maximum number of users
.Li
Location of the root file system
.Li
Available hardware
.Le
.Pa
.I Config
allows multiple system images to be generated from a single
configuration description file,
.I /sys/conf/DYNIX *.
.FS
* For further information on
.I /sys/conf/DYNIX ,
refer to Section 4,
.B "Configuration File Syntax" ,
Section 5,
.B "Sample Configuration File" ,
and Appendix C
.B "Sample Configuration File Listing" .
.FE
Each system image is configured
for identical hardware, but may have different locations for the root
file system and other system devices.
.I Config
is also useful for
tailoring system software to a set of user-defined peripheral and operating
characteristics.
.Bh "Machine type"
.Pa
The 
.I "machine type"
indicates on which architecture type DYNIX operates.
At this time,
the only architecture type is
.B balance .
.I Config
uses
.I "machine type"
to locate data files that are machine-specific
and to select machine-specific rules to construct configuration files.
.Bh "System identification"
.Pa
The
.I "system identification"
is the name attached to the system, and must be
.B DYNIX .
For example, your installation you might have
machines named
.B DOC ,
.B GRUMPY ,
and
.B BASHFUL ,
but the
.I "system identification"
for
.I config
must be
.B DYNIX .
.I Config
uses
.I "system identification"
to create a global C
.B #define
to isolate system-dependent code in the kernel.
For example,
a device driver
might require some special code.
The special code can be preceded by
.B "#ifdef DYNIX"
and superseded by
.B "#endif" ,
which includes the code only if
.B DYNIX
is defined.
In the kernel configuration file distributed with the DYNIX system,
the
.I "system identification"
is
.B DYNIX .
.Pa
The system identifier
.B GENERIC
is given to a system that runs on any machine with any peripheral
configuration that the machine supports; it should not
otherwise be used for a
.I "system identification" .
.Bh "CPU type"
.Pa
The
.I "CPU type"
indicates on which CPU type the system is to operate.
Currently, the only
\f2CPU type\fPs
for a DYNIX system are
.B NS32000
and
.B i386 .
Specifying more than one
.I "CPU type"
implies that the system should be configured to run
on any of the CPUs specified.  For some types of machines this is not
possible, and 
.I config
prints a diagnostic indicating such.
.Bh "Timezone"
.Pa
.I Config
uses
.I "timezone"
to define the information returned by the
.I gettimeofday (2)
system call.
This value is specified as the number of hours east or west of Greenwich
Mean Time (GMT).
Negative numbers indicate a value east of GMT.
The
.I "timezone"
specification may also indicate the
type of daylight savings time rules to be applied.
.Bh "Maximum number of users"
.Pa
The system allocates many system data structures at boot time
based on the
.I "maximum number of users"
configured.
This number is normally at least 8, and depending on whether the
COMMERCIAL, PARALLEL, or TIMESHARE option is set,
should not exceed 512, 32, or 64, respectively.
Within that range,
you are constrained only by the 16-Mbyte address space.
The rules used to size system data structures are discussed in Appendix D.
Note that this is not the maximum number of users on the system at
any one time,
but the maximum number of active users on the system at any one time.
Typically only about one-third of logged on users are active at any one time.
.Bh "Root file system location"
.Pa
When the system boots, it must know the
.I "root file system location" .
This location and the part(s) of the disk(s) to be used
for paging and swapping must be specified in order to create
a complete configuration description.  
.I Config
uses the rules described in Appendix B to 
calculate locations for the system identification
.B DYNIX .
.Pa
When a generic system is configured, the
.I "root file system location"
and swapping area are left undefined until the system is booted.
In this case, the
.I "root file system location"
is specified in the boot command issued to the power-up monitor,
such as in the following example:
.DS
*
b 2 zd(0,0)gendynix zd0a
.DE
.Bh "Hardware devices"
.Pa
When the system boots,
it goes through an
.I autoconfiguration
phase.  During this phase, the system searches for the hardware devices
the system builder has indicated might be present.  This probing
sequence requires certain pieces of information, such as register
addresses or bus interconnects.  A system's hardware may be configured
in a very flexible manner or be specified without any flexibility
whatsoever.
Typically, you do not configure hardware devices into the
system unless they are on the machine, you expect
them to be put on the machine in the near future, or you are guarding
against a hardware failure somewhere else at the site.
(It is prudent to configure in
extra disks in case an emergency requires moving one from a machine that
has hardware problems.)
.Pa
The specification of hardware devices constitutes the majority of
most configuration files; therefore,
much of this document is dedicated to those specifications.
.Bh "Optional items"
.Pa
Other than the mandatory pieces of information described earlier,
it is also possible to include various optional system facilities.
Any optional facilities to be configured into
the system are specified in the configuration file.  The resultant
files generated by
.I config
then automatically include the necessary pieces of the system.
Rules used to size system data structures are discussed in the
.B options
descriptions in Section 4 and in Appendix D.
.Tc
