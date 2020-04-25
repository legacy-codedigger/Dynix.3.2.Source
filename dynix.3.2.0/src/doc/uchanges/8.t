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
. \" $Header: 8.t 1.2 86/03/06 $
.SH
.LG
.ce
Section 7
.SM
.sp
.BP hier
Has been updated to reflect the reorganization
to the user and system source.
.BP mailaddr
Is a new entry describing mail addressing syntax
under sendmail (possibly too Berkeley specific).
.BP ms
The \-ms macros have been extended to allow automatic
creation of a table of contents.  Support for the
refer preprocessor is improved.  Several bugs related
to multi-column output and floating keeps have been
fixed.  Extensions to the accent mark string set are
available by including the .AM macro.  Footnotes
can now be automatically numbered (in superscript)
by \-ms and referenced in the text with a \e** string
register.  The manual page includes a summary of important
number and string registers.  A new document
\&``Changes to \-ms'' is included in Volume 2C of
the programmer's manual.
