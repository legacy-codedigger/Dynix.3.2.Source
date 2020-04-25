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
. \" $Header: 3.t 1.3 86/03/06 $
.NH 1
Access control
.PP
The printer system maintains protected spooling areas so that
users cannot circumvent printer accounting or
remove files other than their own.
The strategy used to maintain protected
spooling areas is as follows:
.IP \(bu 3
The spooling area is writable only by a \f2daemon\fP user
and \f2spooling\fP group.
.IP \(bu 3
The \f2lpr\fP program runs setuid \f2root\fP and
setgid \f2spooling\fP.  The \f2root\fP access is used to
read any file required, verifying accessibility
with an \f2access\fP\|(2) call.  The group ID
is used in setting up proper ownership of files
in the spooling area for \f2lprm\fP.
.IP \(bu 3
Control files in a spooling area are made with \f2daemon\fP
ownership and group ownership \f2spooling\fP.  Their mode is 0660.
This insures control files are not modified by a user
and that no user can remove files except through \f2lprm\fP.
.IP \(bu 3
The spooling programs,
\f2lpd\fP, \f2lpq\fP, and \f2lprm\fP run setuid \f2root\fP
and setgid \f2spooling\fP to access spool files and printers.
.IP \(bu 3
The printer server, \f2lpd\fP,
uses the same verification procedures as \f2rshd\fP\|(8C)
in authenticating remote clients.  The host on which a client
resides must be present in the file /etc/hosts.equiv, used
to create clusters of machines under a single administration. 
.PP
In practice, none of \f2lpd\fP, \f2lpq\fP, or
\f2lprm\fP would have to run as user \f2root\fP if remote
spooling were not supported.  In previous incarnations of
the printer system \f2lpd\fP ran setuid \f2daemon\fP,
setgid \f2spooling\fP, and \f2lpq\fP and \f2lprm\fP ran
setgid \f2spooling\fP.
