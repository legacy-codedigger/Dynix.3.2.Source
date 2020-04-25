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
. \" $Header: a.t 1.15 87/07/29 $
.Ct APPENDIX A "Configuration File Grammar"
.Pa
The following grammar is a simplified form of the
.I yacc (1)
grammar used by
.I config
to parse configuration files.
Terminal symbols are shown all in UPPER CASE, literals
are
.B emboldened ;
optional clauses are enclosed in [brackets];
zero or more instantiations are denoted with
.B * .
.Ns N
In the current version of DYNIX \f1config\fP,
the parsing routine accepts certain key words and constructs
found in the UNIX 4.2bsd version,
such as
.B nexus ,
.B master ,
or
.B uba ,
that apply to other machine architectures.
However, these key words and constructs are omitted from this grammar,
because the functionality that they imply is not supported.
.Ne
.Ps
Configuration ::=  [ Spec \f3;\fP ]*
.sp
Spec ::= Config_spec
	| Device_spec
	| \f3trace\fP
	| /* lambda */
.Pe
.Ps
/* configuration specifications */
.sp
Config_spec ::=  \f3machine\fP ID
	| \f3cpu\fP ID
	| \f3options\fP Opt_list
	| \f3ident\fP ID
	| System_spec
	| \f3timezone\fP [ \f3\-\fP ] NUMBER [ \f3dst\fP [ NUMBER ] ]
	| \f3timezone\fP [ \f3\-\fP ] FPNUMBER [ \f3dst\fP [ NUMBER ] ]
	| \f3maxusers\fP NUMBER
.Pe
.Ps
/* system configuration specifications */
.sp
System_spec ::= \f3config\fP ID System_parameter [ System_parameter ]*
.sp
System_parameter ::=  swap_spec | root_spec | arg_spec
.sp
swap_spec ::=  \f3swap\fP [ \f3on\fP ] swap_dev [ \f3and\fP swap_dev ]*
.sp
swap_dev ::=  dev_spec [ \f3size\fP NUMBER ]
.sp
root_spec ::=  \f3root\fP [ \f3on\fP ] dev_spec
.sp
arg_spec ::=  \f3args\fP [ \f3on\fP ] dev_spec
.sp
dev_spec ::=  dev_name | major_minor
.sp
major_minor ::=  \f3major\fP NUMBER \f3minor\fP NUMBER
.sp
dev_name ::=  ID [ NUMBER [ ID ] ]
.Pe
.Ps
/* option specifications */
.sp
Opt_list ::=  Option [ \f3,\fP Option ]*
.sp
Option ::=  ID [ \f3=\fP Opt_value ]
.sp
Opt_value ::=  ID | NUMBER
.Pe
.Ps
/* device specifications */
.sp
Device_spec ::= \f3device\fP Dev_name Dev_info [ Int_spec ]
	| \f3controller\fP Dev_name Dev_info
	| \f3pseudo-device\fP Dev [ NUMBER ]
.sp
Dev_name ::=  Dev NUMBER
.sp
Dev ::= Controller_name | Device_name
.sp
Controller_name ::= \f3sec\fP | \f3mbad\fP | \f3zdc\fP
/* or any other controller listed in \f2/sys/conf/controllers.balance\fP */
.sp
Device_name ::= ID   /* e.g., \f3xy\fP, \f3st\fP, \f3ts\fP, \f3zd\fP */
.sp
Dev_info ::=  Con_info [ Info ]*
.sp
Con_info ::=  \f3at\fP Controller_name NUMBER
	| \f3at\fP \f3slot\fP NUMBER
.sp
Info ::=  \f3bin\fP NUMBER | Param_name NUMBER
.sp
Param_name ::= \f3csr\fP | \f3maps\fP | \f3flags\fP
	| \f3target\fP | \f3req\fP | \f3doneq\fP | \f3unit\fP
	| \f3drive\fP | \f3drive_type\fP
/* or any other parameters listed in \f2/sys/conf/controllers.balance\fP */
.sp
Int_spec ::=  \f3intr\fP NUMBER
.Pe
.Bh "Lexical Conventions"
.Pa
The terminal symbols are loosely defined as the following:
.Ls H
.Li "ID"
One or more letters, either upper- or lower-case, and underscore,
.B _ .
.Li "NUMBER"
Approximately the C language specification for an integer number.
That is, a leading
.B 0x
indicates a hexadecimal value,
and a leading
.B 0
indicates an octal value; otherwise the number is
expected to be a decimal value.  Hexadecimal numbers may use either
upper-case or lower-case letters.
.Li "FPNUMBER"
A floating point number without an exponent.  That is a number of the
form
.I nnn.ddd ,
where the fractional component is optional.
.Le
.Pa
In special instances, you can substitute a wildcard question mark (?)
for a NUMBER token.
The question mark matches any single character or digit.
.Pa
A
.B #
character at the beginning of the line indicates a comment;
.I config
ignores the rest of the line.
.Pa
A specification is interpreted as a continuation of the previous line
if the first character of the line is tab.
.Tc
