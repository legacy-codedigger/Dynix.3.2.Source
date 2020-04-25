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
. \" $Header: mail8.nr 1.3 86/03/06 $
.bp
.sh 1 "Glossary"
.pp
This section contains the definitions of a few phrases
peculiar to
.i Mail .
.ip "\f2alias\fP"
An alternative name for a person or list of people.
.ip "\f2flag\fP"
An option, given on the command line of
.i Mail ,
prefaced with a \-.  For example,
.b \-f
is a flag.
.ip "\f2header field\fP"
At the beginning of a message, a line which contains information
that is part of the structure of the message.  Popular header fields
include
.i to ,
.i cc ,
and
.i subject .
.ip "\f2mail\ \ \fP"
A collection of messages.  Often used in the phrase,
.q "Have you read your mail?"
.ip "\f2mailbox\fP"
The place where your mail is stored, typically in the directory
/usr/spool/mail.
.ip "\f2message\fP"
A single letter from someone, initially stored in your
.i mailbox .
.ip "\f2message list\fP"
A string used in
.i Mail
command mode to describe a sequence of messages.
.ip "\f2option\fP"
A piece of special purpose information used to tailor
.i Mail
to your taste.
Options are specified with the
.b set
command.
