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
. \" $Header: 1.t 1.2 86/03/06 $
.NH 1
Overview
.PP
The line printer system supports:
.IP \(bu 3
multiple printers,
.IP \(bu 3
multiple spooling queues,
.IP \(bu 3
both local and remote
printers, and
.IP \(bu 3
printers attached via serial lines which require
line initialization such as the baud rate.
.LP
Raster output devices
such as a Varian or Versatec, and laser printers such as an Imagen,
are also supported by the line printer system.
.PP
The line printer system consists mainly of the
following files and commands:
.DS
.TS
l l.
/etc/printcap	printer configuration and capability data base
/usr/lib/lpd	line printer daemon, does all the real work
/usr/ucb/lpr	program to enter a job in a printer queue
/usr/ucb/lpq	spooling queue examination program
/usr/ucb/lprm	program to delete jobs from a queue
/etc/lpc	program to administer printers and spooling queues
/dev/printer	socket on which lpd listens
.TE
.DE
The file /etc/printcap is a master data base describing line
printers directly attached to a machine and, also, printers
accessible across a network. The manual page entry
.IR printcap (5)
provides the ultimate definition of
the format of this data base, as well as
indicating default values for important items
such as the directory in which spooling is performed.
This document highlights the important
information which may be placed
.IR printcap .
