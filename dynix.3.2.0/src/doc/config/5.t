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
. \" $Header: 5.t 1.16 87/07/29 $
.Ct Section 5 "Sample Configuration File"
.Pa
In this section, we discuss how to configure a
sample Sequent system \(em in this case, a B8.
Our hypothetical system is housed entirely in a standard system enclosure,
and contains the peripheral controllers and devices
listed in the following table:
.KS
.TS
center box tab (!) ;
l | l | l | l .
Item!Connection!Name!Reference
_
MULTIBUS adapter board!slot ?!mbad0
SCED board!slot ?!sec0
Terminal multiplexor!mbad0!st0!st(4)
Terminal multiplexor!mbad0!st1
T{
SCSI disk
and target adapter
T}!sec0!sd0!sd(4)
SCSI disk!sec0!sd1
T{
\(14-inch tape
and target adapter
T}!sec0!ts0!ts(4)
.sp .5
.TE
.KE
.LP
We must call the machine
.B DYNIX .
.Pa
The first step is to fill in the global configuration parameters.
The machine is a B8,
so the
.I "machine type"
is
.B balance
and the
.I "CPU type"
is
.B NS32000 .
We may want to connect our system to an Ethernet and use commands such as
.I rlogin
and
.I rsh
that use the Internet networking protocols,
so we need to include the
.B INET
option.
The system identifier, as mentioned earlier,
is
.B DYNIX ,
and the maximum number of users we plan to support is 16,
since we have only 140 Mbytes of disk space.
Finally, we live in the Pacific time zone
(8 dst).
Thus, the beginning of the configuration file may look like
the following excerpt:
.ta \w'maxusers\0\0\0'u
.Ps
#
# B8: starter system
#
machine	balance
cpu	"NS32000"
ident	DYNIX
timezone	8 dst
maxusers	16
options	INET
options TIMESHARE
.Pe
.Pa
To these parameters we must then add the specifications for two
system images.
The first is our standard system
with swap space interleaved between the two disk drives.
The second is a generic system,
to allow us to boot off either disk drive:
.Ps
% config dynix_sd root on sd0a swap on sd0 and sd1
% config gendynix swap generic
.Pe
.Pa
Finally, the hardware must be specified.
First, let's just
transcribe the information from our table of peripherals at the beginning
of this section:
.ta \w'controller\0\0\0\0\0'u +\w'mbad()\0\0'u
.Ps
controller	mbad0	at slot ?
controller	sec0	at slot ?
device	st0	at mbad0 csr 0x200 maps  0 bin 4 intr 3
device	st1	at mbad0 csr 0x210 maps  0 bin 4 intr 4
device	sd0	at sec0 req 4 doneq 4 bin 5 unit 0 target 6
device	sd1	at sec0 req 4 doneq 4 bin 5 unit 1 target 6
device	ts0	at sec0 req 4 doneq 4 bin 5 unit 0 target 4
.Pe
.Pa
That is sufficient for our current configuration.
However, we may eventually want to add more SCED boards
or MULTIBUS adapter boards,
so let's allow our terminal multiplexors, disk units,
and tape unit to be connected to any of them.
To do this, we wildcard the
.B sec
and
.B mbad
connections:
.Ps
device	st0	at mbad? csr 0x200 maps  0 bin 4 intr 3
device	st1	at mbad? csr 0x210 maps  0 bin 4 intr 4
device	sd0	at sec? req 4 doneq 4 bin 5 unit 0 target 6
device	sd1	at sec? req 4 doneq 4 bin 5 unit 1 target 6
device	ts0	at sec? req 4 doneq 4 bin 5 unit 0 target 4
.Pe
.Pa
The SCED board also has an Ethernet port,
.B se0 ,
and two console ports,
.B co0
and
.B co1 ,
plus some battery-backed RAM,
.B sm0 ,
that DYNIX needs to access in order retrieve boot flags
and console parameters from the power-up monitor.
These devices are accessed in much the same way as SCSI devices,
and are described in the following table:
.Ps
device	se0	at sec0 req 25 doneq 25 bin 6 unit 0	# input
device	se0	at sec0 req 10 doneq  10 bin 6 unit 1	# output
device	co0	at sec0 req 4 doneq 4 bin 4 unit 0	# input
device	co0	at sec0 req 4 doneq 4 bin 4 unit 1	# output
device	co1	at sec0 req 4 doneq 4 bin 4 unit 2	# input
device	co1	at sec0 req 4 doneq 4 bin 4 unit 3	# output
device	sm0	at sec0 req  3 doneq  3 bin 4 unit 0
.Pe
.Pa
Note that the Ethernet and console device drivers both require
two
.I device
entries for each port.
.Pa
Finally, we add in the
.I pseudo-devices
required for various functions.
Pseudo terminals,
.B pty ,
are needed to allow users to log in across the network.
The
.B pci
.I pseudo-device
supports BSS/PC-Vdisk communications.
The
.B pmap
.I pseudo-device
is required to support the mapping of physical addresses into
a process's virtual address space.  It is used to access
Atomic Lock Memory and custom memory-mapped devices.
The additional device specifications are shown in the following table:
.Ps
pseudo-device	pty
pseudo-device	pci	8	# 8 PCI devices
pseudo-device	pmap		# phys-map driver
.Pe
.Pa
The completed kernel configuration file for
.B DYNIX
is shown in Appendix C.
.Tc
