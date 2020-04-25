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
.\" $Header: 0.t 1.18 87/10/29 $
.\"
.af % i
.nr nH 0
.nr iB 0
.nr iT 0
.Mt "Building DYNIX Systems with Config"
.DS C
.ps 18
.B "Building DYNIX Systems with Config"
.sp 2
.I ABSTRACT
.ps
.DE
.Pa
This document describes the use of
.I config
to configure and create bootable DYNIX system images.
It discusses the structure of kernel configuration files and how to configure
systems with non-standard hardware configurations.
Appendix D contains a summary of the rules used by the system
in calculating the size of system data structures,
indicates some of the standard system size limitations,
and gives you hints on how to change them.
.Pa
