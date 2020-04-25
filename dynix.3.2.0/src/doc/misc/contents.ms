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
. \" $Header: contents.ms 1.11 90/02/23 $
.TL
DYNIX Programmer's Manual
.sp
Volume 2 \- Supplementary Documents
.PP
This volume contains documents which supplement the information in
Volume 1 of the
.I "DYNIX Programmer's Manual" .
This volume is derived from Volumes 2a and 2b
as provided by Bell Laboratories for
.UX
Version 7,
as well as Volume 2c as provided by the University of California at Berkeley
for their 4.2bsd version of
.UX .
The original ordering and numbering of these documents has been preserved,
although the inapplicable documents have been deleted from this distribution.
.PP
Unless otherwise noted, all references to
.UX
apply to DYNIX.
.PP
Volume II is bound in two parts.
The first part contains articles 2 through 38,
while the second part contains articles from 39 upward.
.DS C
.B "Volume 2, Part 1"
.DE
.SH
General Works
.IP 2.
The UNIX Time-Sharing System.  D. M. Ritchie and K. Thompson.
.in +5n
The original UNIX paper, reprinted from CACM.
.in -5n
.SH
Getting Started
.IP 4.
A Tutorial Introduction to the UNIX Text Editor.  B. W. Kernighan.
.in +5n
An easy way to get started with the editor.
.in -5n
.IP 5.
Advanced Editing on UNIX.  B. W. Kernighan.
.in +5n
The next step.
.in -5n
.IP 6.
An Introduction to the UNIX Shell.  S. R. Bourne.
.in +5n
An introduction to the capabilities of the command interpreter, the shell.
.in -5n
.IP 7.
Learn \(em Computer Aided Instruction on UNIX.  M. E. Lesk and B. W. Kernighan.
.in +5n
Describes a computer-aided instruction program that walks new users through
the basics of files, the editor, and document preparation software.
.in -5n
.SH
Document Preparation
.IP 8.
Typing Documents on the UNIX System.  M. E. Lesk.
.in +5n
Describes the basic use of the formatting tools.
Also describes ``\-ms'', a standardized package of formatting requests
that can be used to lay out most documents
(including those in this volume).
.in -5n
.IP 9.
A System for Typesetting Mathematics.  B. W. Kernighan and L. L. Cherry.
.in +5n
Describes EQN, an easy-to-learn language for doing high-quality mathematical
typesetting.
.in -5n
.IP 10.
TBL \(em A Program to Format Tables.  M. E. Lesk.
.in +5n
A program to permit easy specification of tabular material for typesetting.
Again, easy to learn and use.
.in -5n
.IP 11.
Some Applications of Inverted Indexes on the UNIX System.  M. E. Lesk.
.in +5n
Describes, among other things, the program REFER which fills in bibliographic
citations from a data base automatically.
.in -5n
.IP 12.
NROFF/TROFF User's Manual.  J. F. Ossanna.
.in +5n
The basic formatting program.
.in -5n
.IP 13.
A TROFF Tutorial.  B. W. Kernighan.
.in +5n
An introduction to TROFF for those who really want to know such things.
.in -5n
.SH
Programming
.IP 15.
Lint, A C Program Checker.  S. C. Johnson.
.in +5n
Checks C programs for syntax errors, type violations, portability problems, and
a variety of probable errors.
.in -5n
.IP 16.
Make \(em A Program for Maintaining Computer Programs.  S. I. Feldman.
.in +5n
Indespensible tool for making sure that large programs are properly compiled
with minimal effort.
.in -5n
.IP 17.
UNIX Programming.  B. W. Kernighan and D. M. Ritchie.
.in +5n
Describes the programming interface to the operating system and the standard
I/O library.
.in -5n
.SH
Supporting Tools and Languages
.IP 19.
YACC: Yet Another Compiler-Compiler.  S. C. Johnson.
.in +5n
Converts a BNF specification of a language and semantic actions written in C
into a compiler for the language.
.in -5n
.IP 20.
LEX \(em A Lexical Analyzer Generator.  M. E. Lesk and E. Schmidt.
.in +5n
Creates a recognizer for a set of regular expressions;
each regular expression can be followed by arbitrary C code which will be
executed when the regular expression is found.
.in -5n
.IP 23.
The M4 Macro Processor.  B. W. Kernighan and D. M. Ritchie.
.in +5n
M4 is a macro processor useful as a front end for C, Ratfor, Cobol, and in its
own right.
.in -5n
.IP 24.
SED \(em A Non-interactive Text Editor.  L. E. McMahon.
.in +5n
A variant of the editor for processing large inputs.
.in -5n
.IP 25.
AWK \(em A Pattern Scanning and Processing Language.
A. V. Aho, B. W. Kernighan, and P. J. Weinberger.
.in +5n
Makes it easy to specify many data transformation and selection operations.
.in -5n
.IP 26.
DC \(em An Interactive Desk Calculator.  R. H. Morris and L. L. Cherry.
.in +5n
A super HP calculator, if you don't need floating point.
.in -5n
.IP 27.
BC \(em An Arbitrary Precision Desk-Calculator Language.  L. L. Cherry \
and R. H. Morris.
.in +5n
A front end for DC that provides infix notation, control flow, and built-in
functions.
.in -5n
.SH
Implementation, Maintenance, and Miscellaneous
.IP 31.
UNIX Implementation.  K. Thompson.
.in +5n
How the system actually works inside.
.in -5n
.IP 32.
The UNIX I/O System.  D. M. Ritchie.
.in +5n
How the I/O system really works.
.in -5n
.IP 35.
A Dial-Up Network of UNIX Systems.  D. A. Nowitz and M. E. Lesk.
.in +5n
Describes UUCP, a program for communicating files between UNIX systems.
.in -5n
.IP 36.
Installation and Operation of UUCP.  D. A. Nowitz.
.in +5n
How UUCP works, and how to administer it.
.in -5n
.IP 37.
On the Security of UNIX.  D. M. Ritchie.
.in +5n
Hints on how to break UNIX, and how to avoid doing so.
.in -5n
.IP 38.
Password Security: A Case History.  R. H. Morris and K. Thompson.
.in +5n
How the bad guys used to be able to break the password algorithm,
and why they can't now, at least not so easily.
.in -5n
.IP 38a.
Name Server Operations for BIND.  Kevin J. Dunlap
.in +5n
How to handle name server operations for the Berkeley Internet Name Domain.
.in -5n
. \" begin vol 2c stuff here
.DS C
.B "Volume 2, Part 2"
.DE
.SH
General Works
.IP 39.
Bug Fixes and changes in 4.2BSD.
.in +5n
A brief discussion of the major user-visible changes made to the
system since the last release.
.in -5n
.SH
Getting Started
.IP 40.
An introduction to the C shell
.in +5n
Introducing a popular command interpreter and many of the commonly used
commands, assuming little prior knowledge of
.UX .
.in -5n
.IP 41.
An Introduction to Display Editing with Vi
.in +5n
The document to read to learn to use the \f2vi\f1 screen editor.
.in -5n
.IP 42.
Edit: A tutorial (Revised)
.in +5n
For those who prefer line oriented editing, an introduction assuming
no previous knowledge of
.UX
or of text editing.
.in -5n
.IP 43.
Ex Reference Manual (Version 3.1 \- Oct. 1980)
.in +5n
The final reference for the \f2ex\f1 editor, which underlies both \f2edit\f1
and \f2vi\f1.
.in -5n
.IP 44.
Ex Changes \- Version 3.1 to 3.5
.in +5n
A quick guide to what is new in version 3.5 of \f2ex\f1 and \f2vi\f1, for those
who have used version 3.1.
.in -5n
.IP 45.
Mail Reference Manual (Revised)
.in +5n
Complete details on the mail processing program.
.in -5n
.IP 46.
A Guide to the Dungeons of Doom (Revised)
.in +5n
An introduction to the popular game of \f2rogue\fP.
.in -5n
.SH
Languages
.IP 47.
The FRANZ LISP Manual
.in +5n
A dialect of LISP, largely compatible with MACLISP.
.in -5n
.SH
Document preparation
.IP 53.
Writing Papers with \f2nroff\f1 using \-me
.in +5n
A popular macro package for
.I nroff.
.in -5n
.IP 54.
\-me Reference Manual
.in +5n
The final word on \-me.
.in -5n
.IP 56.
Writing tools \- the Style and Diction Programs
.in +5n
Description of programs which help you understand and improve
your writing style.
.in -5n
.IP 57.
Refer \- A Bibliography System
.in +5n
An introduction to the tools used to maintain bibliographic
databases.  The major program, \f2refer\fP, is used to automatically
retrieve and format references based on document citations.
.in -5n
.IP 58.
A Revised Version of \-ms
.in +5n
A quick description of the revisions made to the \-ms formatting
macros for nroff and troff.
.in -5n
.SH
Programming
.IP 60.
Screen Updating and Cursor Movement Optimization
.in +5n
An aide for writing screen-oriented, terminal independant programs.
.in -5n
.IP 60a.
Disk Quota Notes for Programmers.
.in +5n
What programmers need to know about disk quotas.
.in -5n
.SH
System Installation and Administration
.IP 65.
4.2BSD Line Printer Spooler Manual
.in +5n
This document describes the structure and installation procedure
for the line printer spooling system.  
.in -5n
.IP 66.
Fsck \- The UNIX File System Check Program
.in +5n
A reference document for use with the \f2fsck\fP program during
times of file system distress.
.in -5n
.IP 67.
Sendmail Installation and Operation Guide
.in +5n
The last word in installing and operating the \f2sendmail\fP program.
.in -5n
.SH
Supporting Documentation
.IP 68.
4.2BSD System Manual
.in +5n
A concise, though terse, description of the system call interface
provided in 4.2BSD.  This will never be a best seller.
.in -5n
.IP 69.
A Fast File System for UNIX
.in +5n
A description of the new file system organization design
and implementation.
.in -5n
.IP 71.
Sendmail \- An Internetwork Mail Router
.in +5n
An overview document on the design and implementation of \f2sendmail\fP.
.in -5n
.IP 72.
Mail Systems and Addressing in 4.2BSD
.in +5n
A general introduction to the mail facilities in 4.2BSD.
.in -5n
.IP 72a.
Disk Quota Operations for Users.
.in +5n
What users need to know about disk quotas.
.in -5n
.SH
Miscellaneous
.IP 73.
Gprof: A Call Graph Execution Profiler
.in +5n
Describes the
.I gprof
program.
.in -5n
.IP 74.
A 4.2BSD Interprocess Communications Primer
.in +5n
Describes the facilities available under 4.2bsd to communicate
between processes.
.in -5n
.IP 75.
Star Trek Flight Officer's Manual
.in +5n
The instructions for the 4.2bsd version of
that ultimate universal consumption of computer resources.
.in -5n
.IP 76.
Remote Procedure Call Programming
.in +5n
Programming with Sun's RPC library.
.in -5n
.IP 77.
XDR Protocol Specification
.in +5n
Library routines to describe arbitrary,
machine-independent data structures.
.in -5n
.IP 78.
RPC Protocol Specification
.in +5n
Describes the message protocol used in the external data representation
language.
.in -5n
