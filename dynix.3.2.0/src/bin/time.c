/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

#ifndef lint
static char rcsid[] = "$Header: time.c 2.3 1991/07/08 23:59:24 $";
#endif

/*
 * time
 */
#include <stdio.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/dir.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <sys/user.h>
#include <sys/vm.h>
#include <machine/reg.h>
#include <machine/plocal.h>
#include <sys/proc.h>

#define	TRUE	1
#define	FALSE	0

struct process {
	struct process *p_next;
	struct pt_stop	p_s;
	struct rusage	p_ru;
	int		p_parent;
	int		p_stopped;
	struct timeval	p_start;
	struct timeval	p_finish;
	char		p_name[MAXCOMLEN];
};
#define	p_pid		p_s.ps_pid
#define p_reason	p_s.ps_reason

struct	process	proclist;/* process structure pointer */
struct	timeval	before;	/* time command started */
struct	timeval	after;	/* time command finished */
struct	rusage ru;	/* for collecting resource usage */
int	aflag;		/* time all procs */
int	vflag;		/* give verbose fork/exec/exec notification */
int	rflag;		/* give resource usage */
int	nkids;		/* number of children */
int	ndone;		/* waited for children */

double sex();
struct process *findp();
char *malloc();
char *strcpy();
char *rindex();
chldhandler();

main(argc, argv)
	int argc;
	char **argv;
{
	int status;
	register int p;
	char *myname, *m;

	myname = argv[0];
	while (argc > 1 && argv[1][0] == '-') {
		switch(argv[1][1]) {
		case 'a':	/* all procs */
			aflag = TRUE;
			break;
		case 'v':	/* verbose */
			vflag = TRUE;
			break;
		case 'r':	/* give resource usage */
			rflag = TRUE;
			break;
		}
		argc--;
		argv++;
	}
	if (argc<=1)
		exit(0);
	m = rindex(myname, '/');
	if (m != NULL)
		myname = ++m;
	if (strcmp(myname, "ptime") == 0)
		aflag = TRUE;
	if (aflag == TRUE)
		exit(timeall(argv));
	gettimeofday(&before, 0);
	p = fork();
	if (p < 0) {
		perror("time");
		exit(1);
	}
	if (p == 0) {
		execvp(argv[1], &argv[1]);
		perror(argv[1]);
		exit(1);
	}
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	while (wait3(&status, 0, &ru) != p)
		;
	gettimeofday(&after, 0);
	if ((status&0377) != 0)
		fprintf(stderr, "Command terminated abnormally.\n");
	after.tv_sec -= before.tv_sec;
	after.tv_usec -= before.tv_usec;
	if (after.tv_usec < 0)
		after.tv_sec--, after.tv_usec += 1000000;
	printt("real", &after);
	printt("user", &ru.ru_utime);
	printt("sys ", &ru.ru_stime);
	fprintf(stderr, "\n");
	if (rflag)
		printr(&ru);
	exit (status>>8);
}

printt(s, tv)
	char *s;
	struct timeval *tv;
{

	fprintf(stderr, "%9d.%01d %s ", tv->tv_sec, tv->tv_usec/100000, s);
}

timeall(a)
	char **a;
{
	int status;

	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGTERM, SIG_IGN);
	(void) signal(SIGCHLD, chldhandler);
	(void) ptrace(XPT_MPDEBUGGER, 0, 0, 0);

	(void) gettimeofday(&before, 0);
	forkcmd(a);
	waitall();
	(void) gettimeofday(&after, 0);
	printstats();
	exit(status>>8);
}

forkcmd(a)
	char **a;
{
	int p = fork();
	if (p < 0) {
		perror("time: fork");
		exit(1);
	}
	if (p == 0) {
		(void) signal(SIGINT, SIG_DFL);
		(void) signal(SIGTERM, SIG_DFL);
		(void) signal(SIGCHLD, SIG_DFL);
		execvp(a[1], &a[1]);
		perror(a[1]);
		_exit(127);
	}
}

waitall()
{
	int oldmask;

	oldmask = sigblock(sigmask(SIGCHLD));
	while (nkids == 0 || nkids > ndone)
		(void) sigpause(0);
	(void) sigsetmask(oldmask);
}

struct proc proc;

chldhandler()
{
	struct pt_stop	status;
	register struct process *p;
	struct timeval now;

	while(ptrace(XPT_STOPSTAT, 0, (int) &status, 0) != -1) {

		(void) gettimeofday(&now, 0);
		p = findp(status.ps_pid);
		p->p_reason = status.ps_reason;
		p->p_stopped = TRUE;

		if (vflag) {
			char *why;
			switch(p->p_reason) {
			case PTS_FORK:
				why = "fork";
				break;
			case PTS_EXEC:
				why = "exec";
				break;
			case PTS_EXIT:
				why = "exit";
				break;
			default:
				why = "signal";
				break;
			}
			fprintf(stderr, "%d %s\n", p->p_pid, why);
		}

		switch(p->p_reason) {

		case PTS_FORK:
			(void) ptrace(XPT_RPROC, p->p_pid, (int)&proc, 0);
			p->p_parent = proc.p_ppid;
			(void) strcpy(p->p_name, findp(p->p_parent)->p_name);
			(void) ptrace(XPT_SETSIGMASK, p->p_pid, 0, ~0);
			p->p_start = now;
			break;

		case PTS_EXEC:
			getname(p);
			(void) ptrace(XPT_SETSIGMASK, p->p_pid, 0, ~0);
			if (p->p_parent == 0)
				p->p_start = now;
			break;

		case PTS_EXIT:
			p->p_finish = now;
			getru(p);
			ndone++;
			break;

		default:
			fprintf(stderr, "got signal for proc %d\n", p->p_pid);
			break;
		}

		(void) ptrace(XPT_RPROC, p->p_pid, (int)&proc, 0);
		 p->p_parent = proc.p_ppid;

	}

	for (p = proclist.p_next; p != NULL; p = p->p_next) {
		if (p->p_stopped) {
			(void) ptrace(PT_CONTSIG, p->p_pid, 1, 0);
			p->p_stopped = FALSE;
		}
	}
}


struct process *
findp(pid)
	register int pid;
{
	register struct process *p;

	p = &proclist;
	if (p->p_next == NULL) {
nfound:
		p->p_next = (struct process *) malloc(sizeof(struct process));
		p = p->p_next;
		if (p == NULL) {
			fprintf(stderr, "time: out of memory.\n");
			exit(1);
		}
		bzero((char *)p, sizeof(struct process));
		p->p_pid = pid;
		++nkids;
		return p;
	}
	for (;;) {
		p = p->p_next;
		if (p->p_pid == pid)
			return(p);
		if (p->p_next == NULL)
			break;
	}
	goto  nfound;
}

#define RUSIZE (sizeof(struct rusage)/sizeof(int) + 1)

getru(p)
	struct process *p;
{
	int buf[RUSIZE];
	int offset;
	int i;

	offset = (int) &(((struct user *)0)->u_ru);
	for(i = 0; i < RUSIZE; i++) {
		buf[i] = ptrace(PT_RUSER, p->p_pid, offset, 0);
		offset += sizeof(int);
	}
	p->p_ru = *(struct rusage *)buf;
}

getname(p)
	struct process *p;
{
	register int offset;
	register int *buf;
	register char *cp;
	register int i, j;

	offset = (int) (((struct user *)0)->u_comm);
	buf = (int *) p->p_name;
	cp = p->p_name;
	for (i = 0; i < MAXNAMLEN / sizeof(int) + 1; i++) {
		*buf++ = ptrace(PT_RUSER, p->p_pid, offset, 0);
		offset += sizeof(int);
		for (j = 0; j < sizeof(int); j++) {
			if (*cp++ == '\0')
				return;
		}
	}
}

printr(rp)
	struct rusage *rp;
{
	fprintf(stderr, "%6s%9s%9s%7s%7s%7s%5s%7s%7s\n",
		"MaxRSS", "MajorPF", "MinorPF", "Swaps", "blkI", "blkO", "Nsig", "Vcsw", "Icsw");
#define	R(X) (rp->ru_/**/X)
	fprintf(stderr, "%6d %8d %8d %6d %6d %6d %4d %6d %6d\n",
		R(maxrss), R(majflt), R(minflt), R(nswap),
		R(inblock), R(oublock), R(nsignals), R(nvcsw), R(nivcsw));
}

double
sex(tv)
	struct timeval tv;
{
	return(tv.tv_usec / 1e6 + tv.tv_sec);
}

printstats()
{
	struct process *p;
	struct rusage *rp;
	double totu = 0.0, tots = 0.0, tote;
	double u, s, e;

	fprintf(stderr, "%5s %5s %7s %7s %7s %s\n",
		"pid", "ppid", "utime", "stime", "elapsed", "name");
	for (p = proclist.p_next; p != NULL; p = p->p_next) {
		u = sex(p->p_ru.ru_utime);
		s = sex(p->p_ru.ru_stime);
		e = sex(p->p_finish) - sex(p->p_start);
		fprintf(stderr, "%5d %5d %7.1f %7.1f %7.1f %s\n", 
			p->p_pid, p->p_parent, u, s, e, p->p_name);
		totu += u;
		tots += s;
	}
	tote = sex(after) - sex(before);
	fprintf(stderr, "%5s %5s %7.1f %7.1f %7.1f\n", "total", "", totu, tots, tote);
	if (rflag) {
		fprintf(stderr, "\n%5s %5s %6s%9s%9s%7s%7s%7s%5s%7s%7s\n",
			"pid", "ppid", "MaxRSS", "MajorPF", "MinorPF", "Swaps", "blkI", "blkO", "Nsig", "Vcsw", "Icsw");
		for (p = proclist.p_next; p != NULL; p = p->p_next) {
			rp = &p->p_ru;
			fprintf(stderr, "%5d %5d %6d %8d %8d %6d %6d %6d %4d %6d %6d\n",
				p->p_pid, p->p_parent,
				R(maxrss), R(majflt), R(minflt), R(nswap),
				R(inblock), R(oublock), R(nsignals), R(nvcsw), R(nivcsw));
		}
	}
}
