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
. \" $Header: 6.t 1.3 86/03/06 $
.NH 1
Line printer Administration
.PP
The
.I lpc
program provides local control over line printer activity.
The major commands and their intended use will be described.
The command format and remaining commands are described in
.IR lpc (8).
.LP
\f3abort\fP and \f3start\fP
.IP
.I Abort
terminates an active spooling daemon on the local host immediately and
then disables printing (preventing new daemons from being started by
.IR lpr ).
This is normally used to forciblly restart a hung line printer daemon
(i.e., \f2lpq\fP reports that there is a daemon present but nothing is
happening).  It does not remove any jobs from the queue
(use the \f2lprm\fP command instead).
.I Start
enables printing and requests \f2lpd\fP to start printing jobs.
.LP
\f3enable\fP and \f3disable\fP
.IP
\f2Enable\fP and \f2disable\fP allow spooling in the local queue to be
turned on/off.
This will allow/prevent
.I lpr
from putting new jobs in the spool queue.  It is frequently convenient
to turn spooling off while testing new line printer filters since the
.I root
user can still use
.I lpr
to put jobs in the queue but no one else can.
The other main use is to prevent users from putting jobs in the queue
when the printer is expected to be unavailable for a long time.
.LP
\f3restart\fP
.IP
.I Restart
allows ordinary users to restart printer daemons when
.I lpq
reports that there is no daemon present.
.LP
\f3stop\fP
.IP
.I Stop
is used to halt a spooling daemon after the current job completes;
this also disables printing.  This is a clean way to shutdown a
printer in order to perform
maintenence, etc.  Note that users can still enter jobs in a
spool queue while a printer is
.IR stopped .
.LP
\f3topq\fP
.IP
.I Topq
places jobs at the top of a printer queue.  This can be used
to reorder high priority jobs since
.I lpr
only only provides first-come-first-serve ordering of jobs.
