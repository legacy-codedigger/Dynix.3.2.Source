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
. \" $Header: 1.1.t 1.3 86/03/06 $
.\" %M% %I% %E%
.sh "Processes and protection
.NH 3
Host and process identifiers
.PP
Each UNIX host has associated with it a 32-bit host id, and a host
name of up to 255 characters.  These are set (by a privileged user)
and returned by the calls:
.DS
sethostid(hostid)
long hostid;

hostid = gethostid();
result long hostid;

sethostname(name, len)
char *name; int len;

len = gethostname(buf, buflen)
result int len; result char *buf; int buflen;
.DE
On each host runs a set of \f2processes\fP.
Each process is largely independent of other processes,
having its own protection domain, address space, timers, and
an independent set of references to system or user implemented objects.
.PP
Each process in a host is named by an integer
called the \f2process id\fP.  This number is
in the range 1-30000
and is returned by
the \f2getpid\fP routine:
.DS
pid = getpid();
result int pid;
.DE
On each UNIX host this identifier is guaranteed to be unique;
in a multi-host environment, the (hostid, process id) pairs are
guaranteed unique.
.NH 3
Process creation and termination
.PP
A new process is created by making a logical duplicate of an
existing process:
.DS
pid = fork();
result int pid;
.DE
The \f2fork\fP call returns twice, once in the parent process, where
\f2pid\fP is the process identifier of the child,
and once in the child process where \f2pid\fP is 0.
The parent-child relationship induces a hierarchical structure on
the set of processes in the system.
.PP
A process may terminate by executing an \f2exit\fP call:
.DS
exit(status)
int status;
.DE
returning 8 bits of exit status to its parent.
.PP
When a child process exits or
terminates abnormally, the parent process receives
information about any
event which caused termination of the child process.  A
second call provides a non-blocking interface and may also be used
to retrieve information about resources consumed by the process during its
lifetime.
.DS
#include <sys/wait.h>

pid = wait(astatus);
result int pid; result union wait *astatus;

pid = wait3(astatus, options, arusage);
result int pid; result union waitstatus *astatus;
int options; result struct rusage *arusage;
.DE
.PP
A process can overlay itself with the memory image of another process,
passing the newly created process a set of parameters, using the call:
.DS
execve(name, argv, envp)
char *name, **argv, **envp;
.DE
The specified \f2name\fP must be a file which is in a format recognized
by the system, either a binary executable file or a file which causes
the execution of a specified interpreter program to process its contents.
.NH 3
User and group ids
.PP
Each process in the system has associated with it two user-id's:
a \f2real user id\fP and a \f2effective user id\fP, both non-negative 16 bit
integers.
Each process has an \f2real accounting group id\fP and an \f2effective
accounting group id\fP and a set of
\f2access group id's\fP.  The group id's are non-negative 16 bit integers.
Each process may be in several different access groups, with the maximum
concurrent number of access groups a system compilation parameter,
the constant NGROUPS in the file <sys/param.h>, guaranteed to be at least 8.
.PP
The real and effective user ids associated with a process are returned by:
.DS
ruid = getuid();
result int ruid;

euid = geteuid();
result int euid;
.DE
the real and effective accounting group ids by:
.DS
rgid = getgid();
result int rgid;

egid = getegid();
result int egid;
.DE
and the access group id set is returned by a \f2getgroups\fP call:
.DS
ngroups = getgroups(gidsetsize, gidset);
result int ngroups; int gidsetsize; result int gidset[gidsetsize];
.DE
.PP
The user and group id's
are assigned at login time using the \f2setreuid\fP, \f2setregid\fP,
and \f2setgroups\fP calls:
.DS
setreuid(ruid, euid);
int ruid, euid;

setregid(rgid, egid);
int rgid, egid;

setgroups(gidsetsize, gidset)
int gidsetsize; int gidset[gidsetsize];
.DE
The \f2setreuid\fP call sets both the real and effective user-id's,
while the \f2setregid\fP call sets both the real
and effective accounting group id's.
Unless the caller is the super-user, \f2ruid\fP
must be equal to either the current real or effective user-id,
and \f2rgid\fP equal to either the current real or effective
accounting group id.  The \f2setgroups\fP call is restricted
to the super-user.
.NH 3
Process groups
.PP
Each process in the system is also normally associated with a \f2process
group\fP.  The group of processes in a process group is sometimes
referred to as a \f2job\fP and manipulated by high-level system
software (such as the shell).
The current process group of a process is returned by the
\f2getpgrp\fP call:
.DS
pgrp = getpgrp(pid);
result int pgrp; int pid;
.DE
When a process is in a specific process group it may receive
software interrupts affecting the group, causing the group to
suspend or resume execution or to be interrupted or terminated.
In particular, a system terminal has a process group and only processes
which are in the process group of the terminal may read from the
terminal, allowing arbitration of terminals among several different jobs.
.PP
The process group associated with a process may be changed by
the \f2setpgrp\fP call:
.DS
setpgrp(pid, pgrp);
int pid, pgrp;
.DE
Newly created processes are assigned process id's distinct from all
processes and process groups, and the same process group as their
parent.  A normal (unprivileged) process may set its process group equal
to its process id.  A privileged process may set the process group of any
process to any value.
