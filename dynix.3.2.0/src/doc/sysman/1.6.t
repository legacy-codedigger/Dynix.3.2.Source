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
. \" $Header: 1.6.t 1.3 86/03/06 $
.\" %M% %I% %E%
.sh "Resource controls
.NH 3
Process priorities
.PP
The system gives CPU scheduling priority to processes that have not used
CPU time recently.  This tends to favor interactive processes and
processes that execute only for short periods.
It is possible to determine the priority currently
assigned to a process, process group, or the processes of a specified user,
or to alter this priority using the calls:
.DS
._d
#define	PRIO_PROCESS	0	/* process */
#define	PRIO_PGRP	1	/* process group */
#define	PRIO_USER	2	/* user id */

prio = getpriority(which, who);
result int prio; int which, who;

setpriority(which, who, prio);
int which, who, prio;
.DE
The value \f2prio\fP is in the range \-20 to 20.
The default priority is 0; lower priorities cause more
favorable execution.
The \f2getpriority\fP call returns the highest priority (lowest numerical value)
enjoyed by any of the specified processes.
The \f2setpriority\fP call sets the priorities of all of the
specified processes to the specified value.
Only the super-user may lower priorities.
.NH 3
Resource utilization
.PP
The resources used by a process are returned by a \f2getrusage\fP call,
returning information in a structure defined in <sys/resource.h>:
.DS
._d
#define	RUSAGE_SELF	0		/* usage by this process */
#define	RUSAGE_CHILDREN	-1		/* usage by all children */

getrusage(who, rusage)
int who; result struct rusage *rusage;

._f
struct rusage {
	struct	timeval ru_utime;	/* user time used */
	struct	timeval ru_stime;	/* system time used */
	int	ru_maxrss;	/* maximum core resident set size: kbytes */
	int	ru_ixrss;	/* integral shared memory size (kbytes*sec) */
	int	ru_idrss;	/* unshared data " */
	int	ru_isrss;	/* unshared stack " */
	int	ru_minflt;	/* page-reclaims */
	int	ru_majflt;	/* page faults */
	int	ru_nswap;	/* swaps */
	int	ru_inblock;	/* block input operations */
	int	ru_oublock;	/* block output " */
	int	ru_msgsnd;	/* messages sent */
	int	ru_msgrcv;	/* messages received */
	int	ru_nsignals;	/* signals received */
	int	ru_nvcsw;	/* voluntary context switches */
	int	ru_nivcsw;	/* involuntary " */
};
.DE
The \f2who\fP parameter specifies whose resource usage is to be returned.
The resources used by the current process, or by all
the terminated children of the current process may be requested.
.NH 3
Resource limits
.PP
The resources of a process for which limits are controlled by the
kernel are defined in <sys/resource.h>, and controlled by the
\f2getrlimit\fP and \f2setrlimit\fP calls:
.DS
._d
#define	RLIMIT_CPU	0	/* cpu time in milliseconds */
#define	RLIMIT_FSIZE	1	/* maximum file size */
#define	RLIMIT_DATA	2	/* maximum data segment size */
#define	RLIMIT_STACK	3	/* maximum stack segment size */
#define	RLIMIT_CORE	4	/* maximum core file size */
#define	RLIMIT_RSS	5	/* maximum resident set size */

#define	RLIM_NLIMITS	6

#define	RLIM_INFINITY	0x7f\&f\&f\&f\&f\&f\&f

._f
struct rlimit {
	int	rlim_cur;	/* current (soft) limit */
	int	rlim_max;	/* hard limit */
};

getrlimit(resource, rlp)
int resource; result struct rlimit *rlp;

setrlimit(resource, rlp)
int resource; struct rlimit *rlp;
.DE
.PP
Only the super-user can raise the maximum limits.
Other users may only
alter \f2rlim_cur\fP within the range from 0 to \f2rlim_max\fP
or (irreversibly) lower \f2rlim_max\fP.
