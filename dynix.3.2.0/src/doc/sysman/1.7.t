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
. \" $Header: 1.7.t 1.3 86/03/06 $
.\" %M% %I% %E%
.sh "System operation support
.PP
Unless noted otherwise,
the calls in this section are permitted only to a privileged user.
.NH 3
Bootstrap operations
.PP
The call
.DS
mount(blkdev, dir, ronly);
char *blkdev, *dir; int ronly;
.DE
extends the UNIX name space.  The \f2mount\fP call specifies
a block device \f2blkdev\fP containing a UNIX file system
to be made available starting at \f2dir\fP.  If \f2ronly\fP is
set then the file system is read-only; writes to the file system
will not be permitted and access times will not be updated
when files are referenced.
\f2Dir\fP is normally a name in the root directory.
.PP
The call
.DS
swapon(blkdev, size);
char *blkdev; int size;
.DE
specifies a device to be made available for paging and swapping.
.PP
.NH 3
Shutdown operations
.PP
The call
.DS
unmount(dir);
char *dir;
.DE
unmounts the file system mounted on \f2dir\fP.
This call will succeed only if the file system is
not currently being used.
.PP
The call
.DS
sync();
.DE
schedules input/output to clean all system buffer caches.
(This call does not require priveleged status.)
.PP
The call
.DS
reboot(how)
int how;
.DE
causes a machine halt or reboot.  The call may request a reboot
by specifying \f2how\fP as RB_AUTOBOOT, or that the machine be halted
with RB_HALT.  These constants are defined in <sys/reboot.h>.
.NH 3
Accounting
.PP
The system optionally keeps an accounting record in a file
for each process that exits on the system.
The format of this record is beyond the scope of this document.
The accounting may be enabled to a file \f2name\fP by doing
.DS
acct(path);
char *path;
.DE
If \f2path\fP is null, then accounting is disabled.  Otherwise,
the named file becomes the accounting file.
