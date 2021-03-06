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
. \" $Header: 7.t 1.3 86/03/06 $
.NH 1
Troubleshooting
.PP
There are a number of messages which may be generated by the
the line printer system.  This section
categorizes the most common and explains the cause
for their generation.  Where the message indicates a failure,
directions are given to remedy the problem.
.PP
In the examples below, the name
.I printer
is the name of the printer. 
This would be one of the names from the
.I printcap
database.
.NH 2
LPR
.SH
lpr: \f2printer\fP\|: unknown printer
.IP
The
.I printer
was not found in the
.I printcap
database.  Usually this is a typing mistake; however, it may indicate
a missing or incorrect entry in the /etc/printcap file.
.SH
lpr: \f2printer\fP\|: jobs queued, but cannot start daemon.
.IP
The connection to 
.I lpd
on the local machine failed. 
This usually means the printer server started at
boot time has died or is hung.  Check the local socket
/dev/printer to be sure it still exists (if it does not exist,
there is no 
.I lpd
process running).  Use
.DS
% ps ax | fgrep lpd
.DE
to get a list of process identifiers of running lpd's.
The \f2lpd\fP to kill is the one which is not listed in any
of the ``lock" files (the lock file is contained in the spool directory of
each printer).
Kill the master daemon using the following command.
.DS
% kill \f2pid\fP
.DE
Then remove /dev/printer and restart the daemon (and printer)
with the following commands.
.DS
% rm /dev/printer
% /usr/lib/lpd
.DE
.IP
Another possibility is that the
.I lpr
program is not setuid \f2root\fP, setgid \f2spooling\fP.
This can be checked with
.DS
% ls \-lg /usr/ucb/lpr
.DE
.SH
lpr: \f2printer\fP\|: printer queue is disabled
.IP
This means the queue was turned off with
.DS
% lpc disable \f2printer\fP
.DE
to prevent 
.I lpr
from putting files in the queue.  This is normally
done by the system manager when a printer is
going to be down for a long time.  The
printer can be turned back on by a super-user with
.IR lpc .
.NH 2
LPQ
.SH
waiting for \f2printer\fP to become ready (offline ?)
.IP
The printer device could not be opened by the daemon. 
This can happen for a number of reasons,
the most common being that the printer is turned off-line.
This message can also be generated if the printer is out
of paper, the paper is jammed, etc.
The actual reason is dependent on the meaning
of error codes returned by system device driver. 
Not all printers supply sufficient information 
to distinguish when a printer is off-line or having
trouble (e.g. a printer connected through a serial line). 
Another possible cause of this message is
some other process, such as an output filter,
has an exclusive open on the device.  Your only recourse
here is to kill off the offending program(s) and
restart the printer with
.IR lpc .
.SH
\f2printer\fP is ready and printing
.IP
The
.I lpq
program checks to see if a daemon process exists for
.I printer
and prints the file
.IR status .
If the daemon is hung, a super user can use
.I lpc
to abort the current daemon and start a new one.
.SH
waiting for \f2host\fP to come up
.IP
This indicates there is a daemon trying to connect to the remote
machine named
.I host
in order to send the files in the local queue. 
If the remote machine is up,
.I lpd
on the remote machine is probably dead or
hung and should be restarted as mentioned for
.IR lpr .
.SH
sending to \f2host\fP
.IP
The files should be in the process of being transferred to the remote
.IR host .
If not, the local daemon should be aborted and started with
.IR lpc .
.SH
Warning: \f2printer\fP is down
.IP
The printer has been marked as being unavailable with
.IR lpc .
.SH
Warning: no daemon present
.IP
The \f2lpd\fP process overseeing
the spooling queue, as indicated in the ``lock'' file
in that directory, does not exist.  This normally occurs
only when the daemon has unexpectedly died.
The error log file for the printer should be checked for a
diagnostic from the deceased process.
To restart an \f2lpd\fP, use
.DS
% lpc restart \f2printer\fP
.DE
.NH 2
LPRM
.SH
lprm: \f2printer\fP\|: cannot restart printer daemon
.IP
This case is the same as when
.I lpr
prints that the daemon cannot be started.
.NH 2
LPD
.PP
The
.I lpd
program can write many different messages to the error log file
(the file specified in the 
.B lf
entry in
.IR printcap ).
Most of these messages are about files which can not
be opened and usually indicate the
.I printcap
file or the protection modes of the files are not
correct.   Files may also be inaccessible if people
manually manipulate the line printer system (i.e. they
bypass the
.I lpr
program). 
.PP
In addition to messages generated by 
.IR lpd ,
any of the filters that
.I lpd
spawns may also log messages to this file.
.NH 2
LPC
.PP
.SH
could't start printer
.IP
This case is the same as when
.I lpr
reports that the daemon cannot be started.
.SH
cannot examine spool directory
.IP
Error messages beginning with ``cannot ...'' are usually due to
incorrect ownership and/or protection mode of the lock file, spooling
directory or the
.I lpc
program.
