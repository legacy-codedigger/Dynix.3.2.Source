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
. \" $Header: 0.t 1.4 86/03/06 $
.\" %M% %I% %E%
.if n .ND
.TL
4.2BSD System Manual\(dd
.FS
\(dd\
Some of the information contained in this document
does not apply directly to
the DYNIX\v'-.4'\s4TM\s0\v'+.4' Operating System
or the Balance\v'-.4'\s4TM\s0\v'+.4' Architecture.
This document has been included as originally published
for historical reference purposes.
.FE
.sp
Revised July, 1983
.AU
William Joy, Eric Cooper, Robert Fabry,
.br
Samuel Leffler, Kirk McKusick and David Mosher
.AI
Computer Systems Research Group
Computer Science Division
Department of Electrical Engineering and Computer Science
University of California, Berkeley
Berkeley, CA  94720

(415) 642-7780
.AB
.FS
* UNIX is a trademark of Bell Laboratories.
.FE
This document summarizes the facilities
provided by the 4.2BSD version of the UNIX operating system.
It does not attempt to act as a tutorial for use of the system
nor does it attempt to explain or justify the design of the
system facilities.
It gives neither motivation nor implementation details,
in favor of brevity.
.PP
The first section describes the basic kernel functions
provided to a UNIX process: process naming and protection,
memory management, software interrupts,
object references (descriptors), time and statistics functions,
and resource controls.
These facilities, as well as facilities for
bootstrap, shutdown and process accounting,
are provided solely by the kernel.
.PP
The second section describes the standard system
abstractions for
files and file systems,
communication,
terminal handling,
and process control and debugging.
These facilities are implemented by the operating system or by
network server processes.
.AE
.LP
.de PT
.lt \\n(LLu
.pc %
.nr PN \\n%
.tl '\\*(LH'\\*(CH'\\*(RH'
.lt \\n(.lu
..
.af PN i
.ds LH 4.2BSD System Manual
.ds RH Contents
.bp 1
.if t .ds CF -- September 1, 1982 --
.if t .ds LF CSRG TR/5
.if t .ds RF "Joy, et. al.
.ft 3
.br
.sv 2
.ce
TABLE OF CONTENTS
.ft 1
.LP
.sp 1
.nf
.B "Introduction."
.LP
.if t .sp .5v
.nf
.B "0. Notation and types"
.LP
.if t .sp .5v
.nf
.B "1. Kernel primitives"
.LP
.if t .sp .5v
.nf
.nf
\f31.1.  Processes and protection\fP
\0\0\0.1.  Host and process identifiers
\0\0\0.2.  Process creation and termination
\0\0\0.3.  User and group ids
\0\0\0.4.  Process groups
.LP
.nf
\f31.2.  Memory management\fP
\0\0\0.1.  Text, data and stack
\0\0\0.2.  Mapping pages
\0\0\0.3.  Page protection control
\0\0\0.4.  Giving and getting advice
.LP
.if t .sp .5v
.nf
\f31.3.  Signals\fP
\0\0\0.1.  Overview
\0\0\0.2.  Signal types
\0\0\0.3.  Signal handlers
\0\0\0.4.  Sending signals
\0\0\0.5.  Protecting critical sections
\0\0\0.6.  Signal stacks
.LP
.if t .sp .5v
.nf
\f31.4.  Timing and statistics\fP
\0\0\0.1.  Real time
\0\0\0.2.  Interval time
.LP
.if t .sp .5v
.nf
\f31.5.  Descriptors\fP
\0\0\0.1.  The reference table
\0\0\0.2.  Descriptor properties
\0\0\0.3.  Managing descriptor references
\0\0\0.4.  Multiplexing requests
\0\0\0.5.  Descriptor wrapping
.LP
.if t .sp .5v
.nf
\f31.6.  Resource controls\fP
\0\0\0.1.  Process priorities
\0\0\0.2.  Resource utilization
\0\0\0.3.  Resource limits
.LP
.if t .sp .5v
.nf
\f31.7.  System operation support\fP
\0\0\0.1.   Bootstrap operations
\0\0\0.2.   Shutdown operations
\0\0\0.3.   Accounting
.bp
.LP
.if t .sp .5v
.sp 1
.nf
\f32.  System facilities\fP
.LP
.if t .sp .5v
.nf
\f32.1.   Generic operations\fP
\0\0\0.1.   Read and write
\0\0\0.2.   Input/output control
\0\0\0.3.   Non-blocking and asynchronous operations
.LP
.if t .sp .5v
.nf
\f32.2.  File system\fP
\0\0\0.1   Overview
\0\0\0.2.  Naming
\0\0\0.3.  Creation and removal
\0\0\0.3.1.  Directory creation and removal
\0\0\0.3.2.  File creation
\0\0\0.3.3.  Creating references to devices
\0\0\0.3.4.  Portal creation
\0\0\0.3.6.  File, device, and portal removal
\0\0\0.4.  Reading and modifying file attributes
\0\0\0.5.  Links and renaming
\0\0\0.6.  Extension and truncation
\0\0\0.7.  Checking accessibility
\0\0\0.8.  Locking
\0\0\0.9.  Disc quotas
.LP
.if t .sp .5v
.nf
\f32.3.  Inteprocess communication\fP
\0\0\0.1.   Interprocess communication primitives
\0\0\0.1.1.\0   Communication domains
\0\0\0.1.2.\0   Socket types and protocols
\0\0\0.1.3.\0   Socket creation, naming and service establishment
\0\0\0.1.4.\0   Accepting connections
\0\0\0.1.5.\0   Making connections
\0\0\0.1.6.\0   Sending and receiving data
\0\0\0.1.7.\0   Scatter/gather and exchanging access rights
\0\0\0.1.8.\0   Using read and write with sockets
\0\0\0.1.9.\0   Shutting down halves of full-duplex connections
\0\0\0.1.10.\0  Socket and protocol options
\0\0\0.2.   UNIX domain
\0\0\0.2.1.    Types of sockets
\0\0\0.2.2.    Naming
\0\0\0.2.3.    Access rights transmission
\0\0\0.3.   INTERNET domain
\0\0\0.3.1.    Socket types and protocols
\0\0\0.3.2.    Socket naming
\0\0\0.3.3.    Access rights transmission
\0\0\0.3.4.    Raw access
.LP
.if t .sp .5v
.nf
\f32.4.  Terminals and devices\fP
\0\0\0.1.   Terminals
\0\0\0.1.1.    Terminal input
\0\0\0.1.1.1     Input modes
\0\0\0.1.1.2     Interrupt characters
\0\0\0.1.1.3     Line editing
\0\0\0.1.2.    Terminal output
\0\0\0.1.3.    Terminal control operations
\0\0\0.1.4.    Terminal hardware support
\0\0\0.2.   Structured devices
\0\0\0.3.   Unstructured devices
.LP
.if t .sp .5v
.nf
\f32.5.  Process control and debugging\fP
.LP
.if t .sp .5v
.nf
\f3I.  Summary of facilities\fP
.LP
.af PN 1
.de sh
.ds RH \\$1
.bp
.NH \\*(ss
\s+2\\$1\s0
.PP
.PP
..
.bp 1
.nr ss 1
.de _d
.if t .ta .6i 2.1i 2.6i
.\" 2.94 went to 2.6, 3.64 to 3.30
.if n .ta .84i 2.6i 3.30i
..
.de _f
.if t .ta .5i 1.25i 2.5i 3.5i
.\" 3.5i went to 3.8i
.if n .ta .7i 1.75i 3.8i 4.8i
..
.nr H1 -1
.sh "Notation and types
.PP
The notation used to describe system calls is a variant of a
C language call, consisting of a prototype call followed by
declaration of parameters and results.
An additional keyword \f3result\fP, not part of the normal C language,
is used to indicate which of the declared entities receive results.
As an example, consider the \f2read\fP call, as described in
section 2.1:
.DS
cc = read(fd, buf, nbytes);
result int cc; int fd; result char *buf; int nbytes;
.DE
The first line shows how the \f2read\fP routine is called, with
three parameters.
As shown on the second line \f2cc\fP is an integer and \f2read\fP also
returns information in the parameter \f2buf\fP.
.PP
Description of all error conditions arising from each system call
is not provided here; they appear in the programmer's manual.
In particular, when accessed from the C language,
many calls return a characteristic \-1 value
when an error occurs, returning the error code in the global variable
\f2errno\fP.
Other languages may present errors in different ways.
.PP
A number of system standard types are defined in the include file <sys/types.h>
and used in the specifications here and in many C programs.
These include \f3caddr_t\fP giving a memory address (typically as
a character pointer), 
\f3off_t\fP giving a file offset (typically as a long integer),
and a set of unsigned types \f3u_char\fP, \f3u_short\fP, \f3u_int\fP
and \f3u_long\fP, shorthand names for \f3unsigned char\fP, \f3unsigned
short\fP, etc.
