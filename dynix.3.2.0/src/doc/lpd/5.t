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
. \" $Header: 5.t 1.3 86/03/06 $
.NH 1
Output filter specifications
.PP
The filters supplied with 4.2BSD
handle printing and accounting for most common
line printers, the Benson-Varian, the wide (36") and
narrow (11") Versatec printer/plotters. For other devices or accounting
methods, it may be necessary to create a new filter.
.PP
Filters are spawned by \f2lpd\fP
with their standard input the data to be printed, and standard output
the printer.  The standard error is attached to the
.B lf
file for logging errors.  A filter must return a 0 exit
code if there were no errors, 1 if the job should be reprinted,
and 2 if the job should be thrown away.
When \f2lprm\fP
sends a kill signal to the \f2lpd\fP process controlling
printing, it sends a SIGINT signal 
to all filters and descendents of filters.
This signal can be trapped by filters which need
to perform cleanup operations such as
deleting temporary files.
.PP
Arguments passed to a filter depend on its type.
The
.B of
filter is called with the following arguments.
.DS
\f2ofiler\fP \f3\-w\fPwidth \f3\-l\fPlength
.DE
The \f2width\fP and \f2length\fP values come from the
.B pw
and
.B pl
entries in the printcap database.
The
.B if
filter is passed the following parameters.
.DS
\f2filter\fP [\|\f3\-c\fP\|] \f3\-w\fPwidth \f3\-l\fPlength \f3\-i\fPindent \f3\-n\fP login \f3\-h\fP host accounting_file
.DE
The
.B \-c
flag is optional, and only supplied when control characters
are to be passed uninterpreted to the printer (when the
.B \-l
option of
.I lpr
is used to print the file).
The
.B \-w
and
.B \-l
parameters are the same as for the
.B of
filter.
The
.B \-n
and
.B \-h
parameters specify the login name and host name of the job owner.
The last argument is the name of the accounting file from
.IR printcap .
.PP
All other filters are called with the following arguments:
.DS
\f2filter\fP \f3\-x\fPwidth \f3\-y\fPlength \f3\-n\fP login \f3\-h\fP host accounting_file
.DE
The
.B \-x
and
.B \-y
options specify the horizontal and vertical page
size in pixels (from the
.B px
and
.B py
entries in the printcap file).
The rest of the arguments are the same as for the
.B if
filter.
