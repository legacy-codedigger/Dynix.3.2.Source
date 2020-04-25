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
. \" $Header: 1.t 1.14 87/07/29 $
.\"
.Ct Section 1 "Introduction"
.Pa
.I Config
is a tool used in building DYNIX system images.
It takes as input a file describing a system's tunable parameters and
hardware support, and generates a collection
of files which are then used to build a copy of DYNIX appropriate
to that configuration.
.I Config
simplifies system maintenance by isolating system dependencies
in a single, easy-to-understand file,
.I /usr/sys/conf/DYNIX .
.Pa
The contents of the document are as follows:
.Ls B
.Li
Section 2 describes the contents of the configuration file
.Li
Section 3 discusses the steps necessary to build a bootable system image
.Li
Section 4 describes the configuration file syntax
.Li
Section 5 describes a typical configuration file
.Li
Section 6 provides guidelines for adding new system software,
such as device drivers
.Li
Appendix A describes a simplified form of the
.I yacc
grammar
.I config
uses to parse the configuration file
.Li
Appendix B describes the rules used to set default system devices
.Li
Appendix C contains a listing of the sample configuration file
described in Section five
.Li
Appendix D summarizes the rules
used in calculating important system data structures,
and indicates some inherent system data structure size
limitations and how to modify them.
.Le
.Tc
