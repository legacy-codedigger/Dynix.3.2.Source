/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
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
static char rcsid[] = "$Header: init.c 2.19 90/10/25 $";
#endif

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utmp.h>
#include <setjmp.h>
#include <sys/reboot.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <pwd.h>
#ifdef SCGACCT
#include <local/scgacct.h>
#endif

#define	LINSIZ	sizeof(wtmp.ut_line)
#define	ALL	p = itab; p ; p = p->next
#define	EVER	;;
#define SCPYN(a, b)	strncpy(a, b, sizeof(a))
#define SCMPN(a, b)	strncmp(a, b, sizeof(a))
#define	mask(s)	(1 << ((s)-1))
#define LEGAL_MAGIC	(0x12210000)		/* must not have top bit set! */

char	shell[]	= "/bin/sh";
char	getty[]	 = "/etc/getty";
char	minus[]	= "-";
char	runs[] 	= "/etc/rc.single";
char	runb[]	= "/etc/rc.boot";
char	runc[]	= "/etc/rc";
char	ifile[]	= "/etc/ttys";
char	utmpf[]	= "/etc/utmp";
char	wtmpf[]	= "/usr/adm/wtmp";
char	ctty[]	= "/dev/console";
char	dev[]	= "/dev/";
char	nusersfailm[] = "init: can't figure out the number of users (1 assumed)\n";
char	nuserswarnm[] = "init: a reboot is required to notice new user limit\n";

struct utmp wtmp;
struct
{
	char	line[LINSIZ];
	char	comn;
	char	flag;
} line;
struct	tab
{
	char	line[LINSIZ];
	char	comn;
	char	xflag;
	char	lflag;		/* first col. of ttys file */
	int	pid;
	time_t	gettytime;
	int	gettycnt;
	struct	tab *next;
} *itab;

int	fi;
int	mergflag;
char	tty[20];
jmp_buf	sjbuf, shutpass;
time_t	time0;
unsigned int systemid;
int	initusers;
int	warned;
int	shuttingdown;
off_t	utmp_size;

int	reset();
int	idle();
char	*strcpy(), *strcat(), *crypt();
long	lseek();
struct	passwd *getpwnam();

struct	sigvec rvec = { reset, mask(SIGHUP), 0 };

main()
{
#if ns32000
	register int r7;	/* Boot flags, passed thru from boot */
	register int r6,r5;	/* Trashed by crt0 */
	register int r4;	/* System ID */
#endif
#if i386
	register int edi;	/* Boot flags, passed thru from boot */
	register int esi;	/* System ID */
#endif
	int howto, oldhowto;

#if ns32000
	howto = r7;
	systemid = r4;
#endif
#if i386
	howto = edi;
	asm("	movl	%ecx, %esi ");	/* workaround for lack of registers */
	systemid = esi;
#endif
	time0 = time(0);

	sigvec(SIGTERM, &rvec, (struct sigvec *)0);
	signal(SIGTSTP, idle);
#ifdef  SCGACCT
	setacct('0');		/* default account */
        wtmp.ut_account = '0';
#endif
	signal(SIGSTOP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);

	initusers = legal_users();
	if (initusers <= 0) {
		cttywarn(nusersfailm);
		initusers = 1;
	}

	/*
	 * Send a specific message to kernel to verify
	 * we are the *REAL* init, not a imitator
	 * and tell kernel how many users are allowed.
	 * This implements licensing restrictions.
	 */
	kill(1, LEGAL_MAGIC|initusers);

	runboot();
	(void) setjmp(sjbuf);
	for (EVER) {
		oldhowto = howto;
		howto = RB_SINGLE;
		if (setjmp(shutpass) == 0)
			shutdown();
		shuttingdown = warned = 0;
		if (oldhowto & RB_SINGLE)
			single();
		if (initusers == 0) {
			cttywarn(nuserswarnm);
			continue;
		}
		if (runcom(oldhowto) == 0)
			continue;
		merge();
		multiple();
	}
}

cttywarn(s)
{
	int ct = open(ctty, 1);
	if (ct >= 0) {
		write(ct, s, strlen(s));
		sleep(3);
		close(ct);
		if ((ct = open("/dev/tty", O_RDWR)) >= 0) {
			ioctl(ct, TIOCNOTTY, 0);
			close(ct);
		}
	}
}

int	shutreset();

shutdown()
{
	register i;
	register struct tab *p, *p1;

	close(creat(utmpf, 0644));
	utmp_size = 0;
	shuttingdown = 1;
	signal(SIGHUP, SIG_IGN);
	for (p = itab; p; ) {
		term(p);
		p1 = p->next;
		free(p);
		p = p1;
	}
	itab = (struct tab *)0;
	signal(SIGALRM, shutreset);
	alarm(30);
	for (i = 0; i < 5; i++)
		kill(-1, SIGKILL);
	while (wait((int *)0) != -1)
		;
	alarm(0);
	shutend();
}

char shutfailm[] = "WARNING: Something is hung (wont die); ps axl advised\n";

shutreset()
{
	int status;

	if (fork() == 0) {
		int ct = open(ctty, 1);
		write(ct, shutfailm, sizeof (shutfailm));
		sleep(5);
		exit(1);
	}
	sleep(5);
	shutend();
	longjmp(shutpass, 1);
}

shutend()
{
	register i, f;

	acct(0);
	signal(SIGALRM, SIG_DFL);
	for (i = 0; i < 10; i++)
		close(i);
	f = open(wtmpf, O_WRONLY|O_APPEND);
	if (f >= 0) {
		SCPYN(wtmp.ut_line, "~");
		SCPYN(wtmp.ut_name, "shutdown");
		SCPYN(wtmp.ut_host, "");
		time(&wtmp.ut_time);
		write(f, (char *)&wtmp, sizeof(wtmp));
		close(f);
	}
	return (1);
}

single()
{
	register pid;
	register xpid;
	extern	errno;

	/*
	 * exec a process to do /etc/rc.single.  We don't care if it
	 * fails or not.  Wait for it so that we don't get output
	 * from rc.single after the shell is forked.  Only do this if
	 * /etc/rc.single exists.
	 */
	if (0 == access(runs, F_OK)) {
		pid = fork();
		if (pid == 0) {
			signal(SIGQUIT, SIG_DFL);
			signal(SIGTERM, SIG_DFL);
			signal(SIGHUP, SIG_DFL);
			signal(SIGALRM, SIG_DFL);
			(void) open(ctty, O_RDWR);
			dup2(0, 1);
			dup2(0, 2);
			execl(shell, shell, runs, (char *)0);
			exit(0);
		}
		while (wait((int *)0) != pid);
	}
	do {
		pid = fork();
		if (pid == 0) {
			signal(SIGQUIT, SIG_DFL);
			signal(SIGTERM, SIG_DFL);
			signal(SIGHUP, SIG_DFL);
			signal(SIGALRM, SIG_DFL);
			(void) open(ctty, O_RDWR);
			dup2(0, 1);
			dup2(0, 2);
			execl(shell, minus, (char *)0);
			exit(0);
		}
		while ((xpid = wait((int *)0)) != pid)
			if (xpid == -1 && errno == ECHILD)
				break;
	} while (xpid == -1);
}

runboot()
{
	register pid, f;
	int status;

	pid = fork();
	if (pid == 0) {
		(void) open("/", O_RDONLY);
		dup2(0, 1);
		dup2(0, 2);
		execl(shell, shell, runb, (char *)0);
		exit(1);
	}
	while (wait(&status) != pid)
		;
}

runcom(oldhowto)
	int oldhowto;
{
	register pid, f;
	int status;

	pid = fork();
	if (pid == 0) {
		(void) open("/", O_RDONLY);
		dup2(0, 1);
		dup2(0, 2);
		/*
		 * put root in its set of groups, so things like rshd,
		 * rlogind, and mountd have a fighting chance to access
		 * directories remotely mounted, since root is "nobody"
		 * on remote files.
		 */
		if (initgroups("root", 0) != 0) {
			cttywarn("failed to initialize group list before running /etc/rc\n");
			exit(1);
		}
		if (oldhowto & RB_SINGLE)
			execl(shell, shell, runc, (char *)0);
		else
			execl(shell, shell, runc, "autoboot", (char *)0);
		exit(1);
	}
	while (wait(&status) != pid)
		;
	if (status)
		return (0);
	f = open(wtmpf, O_WRONLY|O_APPEND);
	if (f >= 0) {
		SCPYN(wtmp.ut_line, "~");
		SCPYN(wtmp.ut_name, "reboot");
		SCPYN(wtmp.ut_host, "");
		if (time0) {
			wtmp.ut_time = time0;
			time0 = 0;
		} else
			time(&wtmp.ut_time);
		write(f, (char *)&wtmp, sizeof(wtmp));
		close(f);
	}
	return (1);
}

struct	sigvec	mvec = { merge, mask(SIGTERM), 0 };
/*
 * Multi-user.  Listen for users leaving, SIGHUP's
 * which indicate ttys has changed, and SIGTERM's which
 * are used to shutdown the system.
 */
multiple()
{
	register struct tab *p;
	register pid;

	sigvec(SIGHUP, &mvec, (struct sigvec *)0);
	for (EVER) {
		pid = wait((int *)0);
		if (pid == -1)
			return;
		for (ALL)
			if (p->pid == pid || p->pid == -1) {
				rmut(p);
				dfork(p);
			}
	}
}

/*
 * Merge current contents of ttys file
 * into in-core table of configured tty lines.
 * Entered as signal handler for SIGHUP.
 */
#define	FOUND	1
#define	CHANGE	2

merge()
{
	register struct tab *p;
	register struct tab *p1;

	fi = open(ifile, 0);
	if (fi < 0)
		return;
	for (ALL)
		p->xflag = 0;
	while (rline()) {
		for (ALL) {
			if (SCMPN(p->line, line.line))
				continue;
			p->xflag |= FOUND;
			p->lflag = line.flag;
			if (line.comn != p->comn) {
				p->xflag |= CHANGE;
				p->comn = line.comn;
			}
			goto contin1;
		}

		/*
		 * Make space for a new one.
		 */
		p1 = (struct tab *)calloc(1, sizeof(*p1));
		if (!p1) {
			int f;
			f = open("/dev/console", O_WRONLY);
			write(f, "init: no space for ", 19);
			write(f, line.line, strlen(line.line));
			write(f, "\n\r", 2);
			close(f);
			goto contin1;
		}
		/*
		 * Put new terminal at the end of the linked list.
		 */
		if (itab) {
			for (p = itab; p->next; p = p->next)
				continue;
			p->next = p1;
		} else
			itab = p1;

		p = p1;
		SCPYN(p->line, line.line);
		p->xflag |= FOUND|CHANGE;
		p->comn = line.comn;
		p->lflag = line.flag;

	contin1:
		;
	}
	close(fi);
	p1 = (struct tab *)0;
	for (ALL) {
		if ((p->xflag&FOUND) == 0) {
			term(p);
			if (p1)
				p1->next = p->next;
			else
				itab = p->next;
			free(p);
			p = p1 ? p1 : itab;
		}
		if (p->xflag&CHANGE) {
			term(p);
			dfork(p);
		}
		p1 = p;
	}
}

term(p)
	register struct tab *p;
{

	if (p->pid != 0) {
		rmut(p);
		kill(p->pid, SIGKILL);
	}
	p->pid = 0;
}

rline()
{
	register c, i;

loop:
	c = get();
	if (c < 0)
		return(0);
	if (c == 0)
		goto loop;
	line.flag = c;
	c = get();
	if (c <= 0)
		goto loop;
	line.comn = c;
	SCPYN(line.line, "");
	for (i = 0; i < LINSIZ; i++) {
		c = get();
		if (c <= 0)
			break;
		line.line[i] = c;
	}
	while (c > 0)
		c = get();
	if (line.line[0] == 0)
		goto loop;
	if (line.flag == '0')
		goto loop;
	strcpy(tty, dev);
	strncat(tty, line.line, LINSIZ);
	if (access(tty, 06) < 0)
		goto loop;
	return (1);
}

get()
{
	char b;

	if (read(fi, &b, 1) != 1)
		return (-1);
	if (b == '\n')
		return (0);
	return (b);
}

dfork(p)
	struct tab *p;
{
	register pid;
	time_t t;
	int dowait = 0;
	extern char *sys_errlist[];

	time(&t);
	p->gettycnt++;
	if ((t - p->gettytime) >= 60) {
		p->gettytime = t;
		p->gettycnt = 1;
	} else {
		if (p->gettycnt >= 5) {
			dowait = 1;
			p->gettytime = t;
			p->gettycnt = 1;
		}
	}
	pid = fork();
	if (pid == 0) {
		int oerrno, f;
		extern int errno;

		signal(SIGTERM, SIG_DFL);
		signal(SIGHUP, SIG_IGN);
		strcpy(tty, dev);
		strncat(tty, p->line, LINSIZ);
		if (dowait) {
			f = open("/dev/console", O_WRONLY);
			write(f, "init: ", 6);
			write(f, tty, strlen(tty));
			write(f, ": getty failing, sleeping\n\r", 27);
			close(f);
			sleep(30);
			if ((f = open("/dev/tty", O_RDWR)) >= 0) {
				ioctl(f, TIOCNOTTY, 0);
				close(f);
			}
		}
		chown(tty, 0, 0);
		chmod(tty, 0622);
		if (open(tty, O_RDWR) < 0) {
			int repcnt = 0;
			do {
				oerrno = errno;
				if (repcnt % 10 == 0) {
					f = open("/dev/console", O_WRONLY);
					write(f, "init: ", 6);
					write(f, tty, strlen(tty));
					write(f, ": ", 2);
					write(f, sys_errlist[oerrno],
						strlen(sys_errlist[oerrno]));
					write(f, "\n", 1);
					close(f);
					if ((f = open("/dev/tty", 2)) >= 0) {
						ioctl(f, TIOCNOTTY, 0);
						close(f);
					}
				}
				repcnt++;
				sleep(60);
			} while (open(tty, O_RDWR) < 0);
			exit(0);	/* have wrong control tty, start over */
		}
		vhangup();
		signal(SIGHUP, SIG_DFL);
		(void) open(tty, O_RDWR);
		if (p->lflag == '2') {
			/*
		 	 * Ignore SIGHUP, force DTR off and then on to make sure
		 	 * that any modem connected to this port really knows 
		 	 * that we have gone away and are about to come back... 
		 	 */
			signal(SIGHUP, SIG_IGN);
			ioctl(1, TIOCCDTR, (int *)0);	/* drop dtr */
			sleep(2);
			ioctl(1, TIOCSDTR, (int *)0);  /* set dtr */
			signal(SIGHUP, SIG_DFL);
		}
		close(0);
		dup(1);
		dup(0);
		tty[0] = p->comn;
		tty[1] = 0;
		execl(getty, minus, tty, (char *)0);
		exit(0);
	}
	p->pid = pid;
}

struct stat statbuf;

/*
 * Remove utmp entry.
 */
rmut(p)
	register struct tab *p;
{
	register f;
	register count = 0;
	int found = 0;

	f = open(utmpf, O_RDWR);
	if (f >= 0) {
		count = getnumusers(f, 0);
		while (read(f, (char *)&wtmp, sizeof(wtmp)) == sizeof(wtmp)) {
			if (SCMPN(wtmp.ut_line, p->line) || wtmp.ut_name[0]==0)
				continue;
			lseek(f, -(long)sizeof(wtmp), 1);
			SCPYN(wtmp.ut_name, "");
			SCPYN(wtmp.ut_host, "");
			time(&wtmp.ut_time);
			write(f, (char *)&wtmp, sizeof(wtmp));
			found++;
		}
		if (!shuttingdown && fstat(f, &statbuf) == 0) {
			if (utmp_size <= statbuf.st_size)
				utmp_size = statbuf.st_size;
			else  if (warned == 0) {
				cttywarn("init: /etc/utmp tampered with.\n");
				++warned;
				/* so we never go multi-users again! */
				initusers = 0;	
				kill(1, SIGTERM);
			}
		}
		close(f);
	}
	if (found) {
		f = open(wtmpf, O_WRONLY|O_APPEND);
		if (f >= 0) {
			SCPYN(wtmp.ut_line, p->line);
			SCPYN(wtmp.ut_name, "");
			SCPYN(wtmp.ut_host, "");
			time(&wtmp.ut_time);
			write(f, (char *)&wtmp, sizeof(wtmp));
			close(f);
		}
	}
	/*
	 * Detect if user limit has been bypassed
	 * by replacing /bin/login or if kernel notion of
	 * nuser has been fussed with.
	 */
	if (count > initusers || initusers != getmaxusers()) {
		if (warned == 0) {
			cttywarn("init: illegal number of users detected.\n");
			++warned;
			initusers = 0;	/* so we never go multi-users again! */
			kill(1, SIGTERM);
		}
	}
}

reset()
{

	longjmp(sjbuf, 1);
}

jmp_buf	idlebuf;

idlehup()
{

	longjmp(idlebuf, 1);
}

idle()
{
	register struct tab *p;
	register pid;

	signal(SIGHUP, idlehup);
	for (EVER) {
		if (setjmp(idlebuf))
			return;
		pid = wait((int *) 0);
		if (pid == -1) {
			sigpause(0);
			continue;
		}
		for (ALL)
			if (p->pid == pid) {
				rmut(p);
				p->pid = -1;
			}
	}
}

/*
 * legal_users:  Determine the legal number of users
 *  	allowed on a system.  Requires that the passwd file
 *	contain an entry for special user "usrlimit".  Information 
 *	is then passed to login via getty.
 *
 * RETURN VALUES:
 *	 0 = allow no users, error has occured.
 *   1 - N = allow N users
 */
#define	NKEYS	64

static	char saltc[2+1];
static  char copyright[] = 
  "DYNIX 3.0, Copyright 1984, 1985, 1986, 1987 Sequent Computer Systems";
static	char key1[2*NKEYS];
static	int nusers[] = { 
	1,		/* 1 user */
	2,		/* 2 users */
	4,		/* 4 users */
	16,		/* 16 users */
	32,		/* 32 users */
	64,		/* 64 users */
	1000, 		/* 1000 users */
	1000000,	/* infinite (or close enough) */
	0,		/* end marker */
};

static
legal_users()
{
	register i;
	register struct passwd *p;

	if (systemid == 0 || (p = getpwnam("usrlimit")) == 0)
		return(0);
	for (i=0; nusers[i]; i++) {
		makesalt(systemid, nusers[i]);
		if (strncmp(p->pw_passwd, saltc, 2))
			continue;
		permute(systemid, nusers[i]);
		if (strcmp(p->pw_passwd, crypt(key1, saltc)) == 0)
			return(nusers[i]);
	}
	return (0);
}

static
permute(ether, users)
{
	register int n, i, j;

	bzero(key1, sizeof(key1));
	bcopy(copyright, key1, NKEYS);
	for (i=0; i < NKEYS; i++) {
		for (j=0; j < NKEYS-3; j++) {
			n = *(int *)&key1[j];
#define	FUNC(a, b, c) (((a) * (c) * 1103515245) + 12345 + (b)) & 0x7fffffff;
			n = FUNC(ether, users, n);
			*(int *)&key1[j] = n;
		}
	}
}

static
makesalt(ether, users)
{
	register salt, i, c;

	/* 24 bits of ether address */
	salt = ((((ether & 0x7777) + users) + 
		(ether >> 12)) * 1103515245) + 12345;
	saltc[0] = (salt & 077);
	saltc[1] = ((salt>>6) & 077);
	for (i = 0; i < 2; i++) {
		c = saltc[i] + '.';
		if (c > '9') c += 7;
		if (c > 'Z') c += 6;
		saltc[i] = c;
	}
}
