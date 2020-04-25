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

.\" $Header: release3.ms 2.0 86/01/28 $
.PP
The major differences between release 2 and release 3 of RCS are:
.IP ci: \w'rcsmerge'u+.2i
Release 3 ci determines whether the file to be checked in is
different from the previous revision. If it is not different, ci
asks whether to do a checkin anyway, or, if -q is present, ci
suppresses the checkin. This feature avoids redundant checkins. A
checkin can be forced with the new option -f.
.IP
The option -l on release 3 ci now works properly: After the
checkin, an implicit checkout with locking occurs. The keywords
are updated. A new option, -u, also performs an implicit
checkout, but does not lock.
.IP
The option -k looks through the working file to pick up keyword
values for the revision number, date, author and state, and
assigns them to the checked-in revision, rather then computing
them from existing locks, the clock, etc. This is useful for
software distribution: Suppose a file is maintained in RCS format
at several sites. If an update is sent to these sites and checked
in with the -k option, then the original revision number, date,
author, and state are preserved.
.IP co:
Generates full path names for RCS files during the keyword
expansion. (Determining the full path causes a noticeable
slowdown of co; this can be mitigated by checking out several
files in a single command.)
.IP
A new keyword, $Locker:  $, expands to the id of the user
currently holding a lock on the revision.
.IP rlog:
The option -L omits all files that have no locks set. The option
-R prints only the RCS file name. Try rlog -L -R or rlog -L -h.
.IP rcsdiff:
(New operation) Runs diff on a checked-out file and a revision in
an RCS file. This is useful for figuring out what modifications
were made since the last ci. Rcsdiff can also run diff on 2
revisions in an RCS file.
.IP rcsmerge: (New operation) Merges the changes between 2 revisions in
an RCS file into the checked out revision.
.IP merge:    3-way file merge. Merge now has an option to print the result to
stdout (and a manual page).
.IP File naming conventions:
Release 3 RCS no longer removes suffixes of working files.
In addition, the suffix for RCS files is now ",v" instead of
".v". Thus, a working file of the form "f.c" is stored into
"f.c,v".
.IP
All you have to do is to rename your existing ".v"-files. Don't
forget to add the suffix of the working file, if it was stripped
off.
.IP
Note that this change restricts the length of working file names
to 12 characters (RCS detects violations reliably). In 4.2bsd, this
restriction will be removed. The ",v" was necessary to keep MAKE
happy. A new version of MAKE that knows about RCS files in
subdirectories /RCS will be released soon.
.IP File modes:
During the initial checkin, the RCS file inherits the read and
execute permission from the working file. During subsequent
checkouts, the working file inherits the read and execute
permission from the RCS file. Thus, an executable file containing
a shell program will still be executable after a ci-co cycle.
.IP
The working file is normally generated with write permission for
the owner. An exception is if locking is set to strict, and
checkout is without locking. Then the working file is generated
without write permission, resulting in an error if one tries to
edit it.
.IP Portability:
Release 3 is portable. I have tested it on a VAX-11/780 (Unix 4.1
bsd), a PDP-11/70, and a PDP-11/45 (Unix 2.8 bsd), and it runs on
these machines without change. Porting RCS to Berkeley Unix 4.2
is trivial by changing one macro. I've also included the
modifications that were necessary to run release 2 on the BBN-C70
(BBN's C-machine), IBM 4341 with VM/UTS, M68000, Intel 86/330
with Xenix-86, Onyx with V7 Unix, VAX/VMS/Eunice 2.2. However, it
has not been tested on these systems. Currently. RCS is being
ported to the DEC-20.
.PP
Numerous minor problems have been fixed. RCS now dies gracefully in case
the file system fills up, or if there are other read/write errors.
(Gracefully in this case means that RCS files are not mutilated.)
RCS operations can no longer be interrupted during the renaming of RCS files
(and thus will no longer through away RCS files if interrupted).
There were some problems with nil-revision numbers and with printing of
nil-strings; these have all been fixed. If stdin is not a terminal, ci and
rcs now suppress the prompts for the log message and the descriptive text.
Calls to getlogin() have been replaced with getpwuid(getuid()). The default
for overwriting working files by co has been changed to not overwriting. Co
does overwrite without asking if the file is read-only (generated by
unlocking checkout, but with locking set to strict.) A serious, but
extremely rare problem with the regeneration of older revision has been
fixed. The comment-leader for .h-files is now initially set to " * ".
.PP
Lots of fixes were necessary to make RCS portable. These include
sign-extension bugs, long identifiers, conflicting structure members, and
expression overflows in older C-compilers. One person reported that %02d in
printf doesn't work on his USG system; I now have a macro DATEFORM which
either uses %02d or %.2d.