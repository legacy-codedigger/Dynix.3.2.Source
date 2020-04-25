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
. \" $Header: mail9.nr 1.4 90/04/05 $
.bp
.sh 1 "Summary of commands, options, and escapes"
.pp
This section gives a quick summary of the
.i Mail
commands, binary and valued options, and tilde escapes.
.(b
.pp
The following table describes the commands:
.TS
center ;
c ci
lb l.
Command	Description
_
!	Single command escape to shell
-	Back up to previous message
Print	Type message with ignored fields
Reply	Reply to author of message only
Type	Type message with ignored fields
alias	Define an alias as a set of user names
alternates	List other names you are known by
chdir	Change working directory, home by default
copy	Copy a message to a file or folder
delete	Delete a list of messages
dt	Delete current message, type next message
endif	End of conditional statement; see \f3if\fP
edit	Edit a list of messages
else	Start of else part of conditional; see \f3if\fP
exit	Leave mail without changing anything
file	Interrogate/change current mail file
folder	Same as \f3file\fP
folders	List the folders in your folder directory
from	List headers of a list of messages
headers	List current window of messages
help	Print brief summary of \f2Mail\fP commands
hold	Same as \f3preserve\fP
if	Conditional execution of \f2Mail\fP commands
ignore	Set/examine list of ignored header fields
list	List valid \f2Mail\fP commands
local	List other names for the local host
mail	Send mail to specified names
mbox	Arrange to save a list of messages in \f2mbox\fP
next	Go to next message and type it
preserve	Arrange to leave list of messages in system mailbox
quit	Leave \f2Mail\fP; update system mailbox, \f2mbox\fP as appropriate
reply	Compose a reply to a message
save	Append messages, headers included, on a file
set	Set binary or valued options
shell	Invoke an interactive shell
top	Print first so many (5 by default) lines of list of messages
type	Print messages
undelete	Undelete list of messages
unlock	Break read-only lock on a mailbox
unset	Undo the operation of a \f3set\fP
visual	Invoke visual editor on a list of messages
write	Append messages to a file, don't include headers
z	Scroll to next/previous screenful of headers
.TE
.)b
.(b
.pp
The following table describes the options.  Each option is
shown as being either a binary or valued option.
.TS
center;
c ci ci
l ci l.
Option	Type	Description
_
EDITOR	valued	Pathname of editor for ~e and \f3edit\fP
SHELL	valued	Pathname of shell for \f3shell\fP, ~! and \f3!\fP
VISUAL	valued	Pathname of screen editor for ~v, \f3visual\fP
append	binary	Always append messages to end of \f2mbox\fP
ask	binary	Prompt user for Subject: field when sending
askcc	binary	Prompt user for additional Cc's at end of message
autoprint	binary	Print next message after \f3delete\fP
crt	valued	Minimum number of lines before using \f2more\fP
debug	binary	Print out debugging information
dot	binary	Accept . alone on line to terminate message input
escape	valued	Escape character to be used instead of ~
folder	valued	Directory to store folders in
hold	binary	Hold messages in system mailbox by default
ignore	binary	Ignore \s-2RUBOUT\s0 while sending mail
ignoreeof	binary	Don't terminate letters/command input with \f3\(uaD\fP
keep	binary	Don't unlink system mailbox when empty
keepsave	binary	Don't delete \f3save\fPd messages by default
metoo	binary	Include sending user in aliases
noheader	binary	Suppress initial printing of version and headers
nosave	binary	Don't save partial letter in \f2dead.letter\fP
quiet	binary	Suppress printing of \f2Mail\fP version and message numbers
record	valued	File to save all outgoing mail in
screen	valued	Size of window of message headers for \f3z\fP, etc.
sendmail	valued	Choose alternate mail delivery system
toplines	valued	Number of lines to print in \f3top\fP
verbose	binary	Invoke sendmail with the \f3\-v\fP flag
.TE
.)b
.(b
.pp
The following table summarizes the tilde escapes available
while sending mail.
.TS
center;
c ci ci
l li l.
Escape	Arguments	Description
_
~!	command	Execute shell command
~c	name ...	Add names to Cc: field
~d		Read \f2dead.letter\fP into message
~e		Invoke text editor on partial message
~f	messages	Read named messages
~h		Edit the header fields
~m	messages	Read named messages, right shift by tab
~p		Print message entered so far
~q		Abort entry of letter; like \s-2RUBOUT\s0
~r	filename	Read file into message
~s	string	Set Subject: field to \f2string\fP
~t	name ...	Add names to To: field
~v		Invoke screen editor on message
~w	filename	Write message on file
~|	command	Pipe message through \f2command\fP
~~	string	Quote a ~ in front of \f2string\fP
.TE
.)b
.(b
.pp
The following table shows the command line flags that
.i Mail
accepts:
.TS
center;
c c
l a.
Flag	Description
_
\-N	Suppress the initial printing of headers
\-T \f2file\fP	Article-id's of read/deleted messages to \f2file\fP
\-d	Turn on debugging
\-f \f2file\fP	Show messages in \f2file\fP or \f2~/mbox\fP
\-h \f2number\fP	Pass on hop count for mail forwarding
\-i	Ignore tty interrupt signals
\-n	Inhibit reading of /usr/lib/Mail.rc
\-r \f2name\fP	Pass on \f2name\fP for mail forwarding
\-s \f2string\fP	Use \f2string\fP as subject in outgoing mail
\-u \f2name\fP	Read \f2name's\fP mail instead of your own
\-v 	Invoke sendmail with the \f3\-v\fP flag
.TE
.)b
.lp
Notes:
.b \-T ,
.b \-d ,
.b \-h ,
and
.b \-r
are not for human use.
