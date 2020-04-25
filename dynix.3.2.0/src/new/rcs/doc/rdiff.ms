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

.\" $Header: rdiff.ms 2.0 86/01/28 $
.TL
Changes to diff
.PP
The options -e and -f generate edit scripts that can be used to
regenerate one file from the other. In these scripts, groups of text lines
following the a (add) and c (change) commands are terminated with a
line consisting of a single '.'. This is a botch. I added a new option, -n,
which works like -f, except that groups of text lines are not terminated
with '.'. Instead, the number of text lines is given in the command itself.
The commands are generated in order of increasing line numbers.
Option -h does not support -n.

.nf
Commands generated by diff -n

d l n      delete lines l..l+n-1
a l n      add the following n lines after line l.
....
....