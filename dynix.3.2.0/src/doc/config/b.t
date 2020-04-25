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
. \" $Header: b.t 1.15 87/07/29 $
.Ct APPENDIX B "Rules For Defaulting System Devices"
.Pa
When
.I config
processes a
.I config
rule that does not fully specify the location of the root file system and
paging area(s),
it applies a set of rules to
define those values left unspecified.  The following
rules are used in defaulting system devices:
.Ls B
.Li
If a root device is not specified, the swap
specification must indicate a
.I generic
system is to be built
.Li
If the root device does not specify a unit number, it
defaults to unit 0
.Li
If the root device does not include a partition specification,
it defaults to the
.B a
partition
.Li
If no swap area is specified, it defaults to the
.B b
partition of the root device
.Le
.Bh "Multiple swap/paging areas"
.Pa
When multiple swap partitions are specified, the system treats the
first specified as a
.I primary
swap area, which is always used.
The remaining partitions are then interleaved into the paging
system at the time a
.I swapon (2)
system call is made.  This is normally done at boot time with
a call to
.I swapon (8)
from the file
.I /etc/rc .
.Tc
