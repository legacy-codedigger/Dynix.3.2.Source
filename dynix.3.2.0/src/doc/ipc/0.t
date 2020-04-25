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
. \" $Header: 0.t 1.3 86/03/06 $
.ds lq ``
.ds rq ''
.de DT
.if t .ta .5i 1.25i 2.5i 3.75i
.\" 3.5i went to 3.8i
.if n .ta .7i 1.75i 3.8i 
..
.bd S B 3
.TL
A 4.2BSD Interprocess Communication Primer
.br
DRAFT of \*(DY
.AU
Samuel J. Leffler
.AU
Robert S. Fabry
.AU
William N. Joy
.AI
Computer Systems Research Group
Department of Electrical Engineering and Computer Science
University of California, Berkeley
Berkeley, California  94720
(415) 642-7780
.de IR
\f2\\$1\fP\\$2
..
.de UX
UNIX\\$1
..
.AB
.PP
.FS
* DEC and VAX are trademarks of
Digital Equipment Corporation.
.FE
.FS
** \s-2UNIX\s0 is a Trademark of Bell Laboratories.
.FE
This document provides an introduction to the interprocess
communication facilities included in the
4.2BSD release of the VAX*
.UX **
system.
.PP
It discusses the overall model for interprocess communication
and introduces the interprocess communication primitives
which have been added to the system.  The majority of the
document considers the use of these primitives in developing
applications.  The reader is expected to be familiar with
the C programming language as all examples are written in C.
.AE
