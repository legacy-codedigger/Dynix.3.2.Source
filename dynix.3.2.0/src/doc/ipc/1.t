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
.ds LH "4.2BSD IPC Primer
.ds RH Introduction
.LP
.nr H1 1
.bp
.ds RF "Leffler/Fabry/Joy
.ds LF "DRAFT of \*(DY
.ds CF "
.LG
.B
.ce
1. INTRODUCTION
.sp 2
.R
.NL
One of the most important parts of 4.2BSD is the interprocess
communication facilities.  These facilities are the result of
more than two years of discussion and research.  The facilities
provided in 4.2BSD incorporate many of the ideas from current
research, while trying to maintain the UNIX philosophy of
simplicity and conciseness.  It is hoped that
the interprocess communication
facilities included in 4.2BSD will establish a
standard for UNIX.  From the response to the design,
it appears many organizations carrying out
work with UNIX are adopting it.
.PP
UNIX has previously been very weak in the area of interprocess
communication.  Prior to the 4.2BSD facilities, the only
standard mechanism which allowed two processes to communicate were
pipes (the mpx files which were part of Version 7 were
experimental).  Unfortunately, pipes are very restrictive
in that
the two communicating processes must be related through a
common ancestor.
Further, the semantics of pipes makes them almost impossible
to maintain in a distributed environment. 
.PP
Earlier attempts at extending the ipc facilities of UNIX have
met with mixed reaction.  The majority of the problems have
been related to the fact these facilities have been tied to
the UNIX file system; either through naming, or implementation.
Consequently, the ipc facilities provided in 4.2BSD have been
designed as a totally independent subsystem.  The 4.2BSD ipc
allows processes to rendezvous in many ways. 
Processes may rendezvous through a UNIX file system-like
name space (a space where all names are path names)
as well as through a
network name space.  In fact, new name spaces may
be added at a future time with only minor changes visible
to users.  Further, the communication facilities 
have been extended to included more than the simple byte stream
provided by a pipe-like entity.  These extensions have resulted
in a completely new part of the system which users will need
time to familiarize themselves with.  It is likely that as
more use is made of these facilities they will be refined;
only time will tell.
.PP
The remainder of this document is organized in four sections.  
Section 2 introduces the new system calls and the basic model
of communication.  Section 3 describes some of the supporting
library routines users may find useful in constructing distributed
applications.  Section 4 is concerned with the client/server model
used in developing applications and includes examples of the
two major types of servers.  Section 5 delves into advanced topics
which sophisticated users are likely to encounter when using
the ipc facilities.  
