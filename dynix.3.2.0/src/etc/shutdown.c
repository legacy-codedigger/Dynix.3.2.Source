/* $Copyright:	$
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
/*
 * Copyright (c) 1983,1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
/*
char copyright[] =
"@(#) Copyright (c) 1983,1986 Regents of the University of California.\n\
 All rights reserved.\n";
static char sccsid[] = "@(#)shutdown.c	5.7 (Berkeley) 12/26/87";
*/
#endif not lint

#ifndef	lint
static char rcsid[] = "$Header: shutdown.c 2.6 91/04/03 $";
#endif

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <setjmp.h>
#include <utmp.h>
#include <pwd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/param.h>
#include <sys/syslog.h>

#include <errno.h>
#include <strings.h>
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <nfs/nfs.h>
#include <rpcsvc/mount.h>
#include <rpcsvc/rwall.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/stat.h>

/*
 *	/etc/shutdown when [messages]
 *
 *	allow super users to tell users and remind users
 *	of iminent shutdown of unix
 *	and shut it down automatically
 *	and even reboot or halt the machine if they desire
 */

#define	REBOOT	"/etc/reboot"
#define	HALT	"/etc/halt"
#define NFSD	"/etc/nfsd"
#define MAXINTS 20
#define	HOURS	*3600
#define MINUTES	*60
#define SECONDS
#define NLOG		600		/* no of bytes possible for message */
#define	NOLOGTIME	5 MINUTES
#define IGNOREUSER	"sleeper"

struct hostlist {
    char *host;
    struct hostlist *nxt;
} *hostlist;

char	hostname[MAXHOSTNAMELEN];
char	buf[BUFSIZ];

int	timeout();
time_t	getsdt();
void	finish();

extern	char *ctime();
extern	struct tm *localtime();
extern	long time();

extern	char *strcpy();
extern	char *strncat();
extern	off_t lseek();

struct	utmp utmp;
int	sint;
int	stogo;
char	tpath[] =	"/dev/";
int	nlflag = 1;		/* nolog yet to be done */
int	killflg = 1;
int	doreboot = 0;
int	halt = 0;
int     fast = 0;
char    *nosync = NULL;
char    nosyncflag[] = "-n";
char	term[sizeof tpath + sizeof utmp.ut_line];
char	tbuf[BUFSIZ];
char	nolog1[] = "\n\nNO LOGINS: System going down at %5.5s\n\n";
char	nolog2[NLOG+1];
#ifdef	DEBUG
char	nologin[] = "nologin";
char    fastboot[] = "fastboot";
#else
char	nologin[] = "/etc/nologin";
char	fastboot[] = "/fastboot";
#endif
time_t	nowtime;
time_t	warntime;
time_t	toolong = (60 * 5); /* abort shutdown if you wait this long to write
			       to a tty */
jmp_buf	alarmbuf;

struct interval {
	int stogo;
	int sint;
} interval[] = {
	4 HOURS,	1 HOURS,
	2 HOURS,	30 MINUTES,
	1 HOURS,	15 MINUTES,
	30 MINUTES,	10 MINUTES,
	15 MINUTES,	5 MINUTES,
	10 MINUTES,	5 MINUTES,
	5 MINUTES,	3 MINUTES,
	2 MINUTES,	1 MINUTES,
	1 MINUTES,	30 SECONDS,
	0 SECONDS,	0 SECONDS
};

char *shutter, *getlogin();

main(argc,argv)
	int argc;
	char **argv;
{
	register i, ufd;
	register char *f;
	char *ts;
	time_t sdt;
	int h, m;
	int first;
	FILE *termf;
	struct passwd *pw, *getpwuid();
	extern char *strcat();
	extern uid_t geteuid();
	char *name;
	struct hostlist *hl;
	char rcbuf[1024];
	char rcargs[1024];

	*rcargs = '\0';
	name = argv[0];
	shutter = getlogin();
	if (shutter == 0 && (pw = getpwuid(getuid())))
		shutter = pw->pw_name;
	if (shutter == 0)
		shutter = "???";
	(void) gethostname(hostname, sizeof (hostname));
	openlog("shutdown", 0, LOG_AUTH);
	argc--, argv++;
	while (argc > 0 && (f = argv[0], *f++ == '-')) {
		while (i = *f++) switch (i) {
		case 'k':
			killflg = 0;
			continue;
		case 'n':
			nosync = nosyncflag;
			continue;
		case 'f':
			fast = 1;
			continue;
		case 'r':
			doreboot = 1;
			continue;
		case 'h':
			halt = 1;
			continue;
		default:
			/*
			 * Unknown args may be recognized by rc.shutdown
			 * so we don't flag them as errors.
			 */
			continue;
		}
		/*
		 * Pass all arguments to rc.shutdown
		 */
		(void)strcat(rcargs, " ");
		(void)strcat(rcargs, *argv);

		argc--, argv++;
	}
	if (argc < 1) {
		printf("Usage: %s [ -krhfn ] shutdowntime [ message ]\n", name);
		finish();
	}
	if (fast && (nosync == nosyncflag)) {
	        printf ("shutdown: Incompatible switches 'fast' & 'nosync'\n");
		finish();
	}
	if (geteuid()) {
		fprintf(stderr, "NOT super-user\n");
		finish();
	}
	gethostlist();
	nowtime = time((long *)0);
	sdt = getsdt(argv[0]);
	argc--, argv++;
	nolog2[0] = '\0';
	while (argc-- > 0) {
		(void) strcat(nolog2, " ");
		(void) strcat(nolog2, *argv++);
	}
	m = ((stogo = sdt - nowtime) + 30)/60;
	h = m/60; 
	m %= 60;
	ts = ctime(&sdt);
	printf("Shutdown at %5.5s (in ", ts+11);
	if (h > 0)
		printf("%d hour%s ", h, h != 1 ? "s" : "");
	printf("%d minute%s) ", m, m != 1 ? "s" : "");
#ifndef DEBUG
	(void) signal(SIGHUP, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	(void) signal(SIGINT, SIG_IGN);
#endif
	(void) signal(SIGTTOU, SIG_IGN);
	(void) signal(SIGTERM, finish);
	(void) signal(SIGALRM, timeout);
	(void) setpriority(PRIO_PROCESS, 0, PRIO_MIN);
	(void) fflush(stdout);
#ifndef DEBUG
	if (i = fork()) {
		printf("[pid %d]\n", i);
		exit(0);
	}
#else
	(void) putc('\n', stdout);
#endif
	sint = 1 HOURS;
	f = "";
	ufd = open("/etc/utmp",0);
	if (ufd < 0) {
		perror("shutdown: /etc/utmp");
		exit(1);
	}
	first = 1;
	for (;;) {
		for (i = 0; stogo <= interval[i].stogo && interval[i].sint; i++)
			sint = interval[i].sint;
		if (stogo > 0 && (stogo-sint) < interval[i].stogo)
			sint = stogo - interval[i].stogo;
		if (stogo <= NOLOGTIME && nlflag) {
			nlflag = 0;
			nolog(sdt);
		}
		if (sint >= stogo || sint == 0)
			f = "FINAL ";
		nowtime = time((long *)0);
		(void) lseek(ufd, 0L, 0);
		while (read(ufd,(char *)&utmp,sizeof utmp)==sizeof utmp)
		if (utmp.ut_name[0] &&
		    strncmp(utmp.ut_name, IGNOREUSER, sizeof(utmp.ut_name))) {
			if (setjmp(alarmbuf))
				continue;
			(void) strcpy(term, tpath);
			(void) strncat(term, utmp.ut_line, sizeof utmp.ut_line);
			warntime = time((time_t *)0);
			(void) alarm(3);
#ifdef DEBUG
			if ((termf = stdout) != NULL)
#else
			if ((termf = fopen(term, "w")) != NULL)
#endif
			{
				(void) alarm(0);
				setbuf(termf, tbuf);
				fprintf(termf, "\n\r\n");
				warn(termf, sdt, nowtime, f);
				if (first || sdt - nowtime > 1 MINUTES) {
					if (*nolog2)
						fprintf(termf, "\t...%s", nolog2);
				}
				(void) fputc('\r', termf);
				(void) fputc('\n', termf);
				(void) alarm(5);
#ifdef DEBUG
				(void) fflush(termf);
#else
				(void) fclose(termf);
#endif
				(void) alarm(0);
			}
			if ((warntime - time((time_t *)0)) > toolong) {
				printf("shutdown aborted due to waiting too long to write to dev \"%s\"\n", term);
				exit(1);
			}
		}
		for (hl = hostlist; hl != NULL; hl = hl->nxt) {
			rwarn(sdt, nowtime, f, hl->host);
			if (first || sdt - nowtime > 1 MINUTES) {
				if (*nolog2) {
					sprintf(buf, "\t...%s", nolog2);
					rprintf(hl->host, buf);
				}
			}
		}
		if (stogo <= 0) {
			printf("\n\007\007System shutdown time has arrived\007\007\n");
			syslog(LOG_CRIT, "%s by %s: %s",
			    doreboot ? "reboot" : halt ? "halt" : "shutdown",
			    shutter, nolog2);
			sleep(2);
			(void) unlink(nologin);
			if (!killflg) {
				printf("but you'll have to do it yourself\n");
				finish();
			}
			if (fast)
				doitfast();
#ifndef DEBUG
			(void)chdir("/"); /* out of all mount points */

			sprintf(rcbuf, "exec sh /etc/rc.shutdown warn %s",
				rcargs);
			(void)system(rcbuf);

			/* Stop init from spawning more processes as we kill */
			(void)kill( 1, SIGTSTP);

			if (doreboot)
				execle(REBOOT, "reboot", "-l", nosync, 0, 0);
			if (halt)
				execle(HALT, "halt", "-l", nosync, 0, 0);
			(void) kill(-1, SIGTERM);	/* kill all */

			sleep(5);
			sprintf(rcbuf, "exec sh /etc/rc.shutdown shutdown %s",
				rcargs);
			(void)system(rcbuf);
			(void) kill(1, SIGTERM);	/* sync */
			(void) kill(1, SIGTERM);	/* to single user */
#else
			if (doreboot)
				printf("REBOOT");
			if (halt)
				printf(" HALT");
			if (fast)
				printf(" -l %s (without fsck's)\n", nosync);
			else
				printf(" -l %s\n", nosync);
			else
				printf("kill -HUP 1\n");

#endif
			finish();
		}
		stogo = sdt - time((long *) 0);
		if (stogo > 0 && sint > 0)
			sleep((unsigned)(sint<stogo ? sint : stogo));
		stogo -= sint;
		first = 0;
	}
}

time_t
getsdt(s)
	register char *s;
{
	time_t t, t1, tim;
	register char c;
	struct tm *lt;

	if (strcmp(s, "now") == 0)
		return(nowtime);
	if (*s == '+') {
		++s; 
		t = 0;
		for (;;) {
			c = *s++;
			if (!isdigit(c))
				break;
			t = t * 10 + c - '0';
		}
		if (t <= 0)
			t = 5;
		t *= 60;
		tim = time((long *) 0) + t;
		return(tim);
	}
	t = 0;
	while (strlen(s) > 2 && isdigit(*s))
		t = t * 10 + *s++ - '0';
	if (*s == ':')
		s++;
	if (t > 23)
		goto badform;
	tim = t*60;
	t = 0;
	while (isdigit(*s))
		t = t * 10 + *s++ - '0';
	if (t > 59)
		goto badform;
	tim += t; 
	tim *= 60;
	t1 = time((long *) 0);
	lt = localtime(&t1);
	t = lt->tm_sec + lt->tm_min*60 + lt->tm_hour*3600;
	if (tim < t || tim >= (24*3600)) {
		/* before now or after midnight */
		printf("That must be tomorrow\nCan't you wait till then?\n");
		finish();
	}
	return (t1 + tim - t);
badform:
	printf("Bad time format\n");
	finish();
	/*NOTREACHED*/
}

warn(term, sdt, now, type)
	FILE *term;
	time_t sdt, now;
	char *type;
{
	char *ts;
	register delay = sdt - now;

	if (delay > 8)
		while (delay % 5)
			delay++;

	fprintf(term,
	    "\007\007\t*** %sSystem shutdown message from %s@%s ***\r\n\n",
		    type, shutter, hostname);

	ts = ctime(&sdt);
	if (delay > 10 MINUTES)
		fprintf(term, "System going down at %5.5s\r\n", ts+11);
	else if (delay > 95 SECONDS) {
		fprintf(term, "System going down in %d minute%s\r\n",
		    (delay+30)/60, (delay+30)/60 != 1 ? "s" : "");
	} else if (delay > 0) {
		fprintf(term, "System going down in %d second%s\r\n",
		    delay, delay != 1 ? "s" : "");
	} else
		fprintf(term, "System going down IMMEDIATELY\r\n");
}

doitfast()
{
	FILE *fastd;

	if ((fastd = fopen(fastboot, "w")) != NULL) {
		putc('\n', fastd);
		(void) fclose(fastd);
	}
}

nolog(sdt)
	time_t sdt;
{
	FILE *nologf;

	(void) unlink(nologin);			/* in case linked to std file */
	if ((nologf = fopen(nologin, "w")) != NULL) {
		fprintf(nologf, nolog1, (ctime(&sdt)) + 11);
		if (*nolog2)
			fprintf(nologf, "\t%s\n", nolog2 + 1);
		(void) fclose(nologf);
	}
}

void
finish()
{
	(void) signal(SIGTERM, SIG_IGN);
	(void) unlink(nologin);
	exit(0);
}

timeout()
{
	longjmp(alarmbuf, 1);
}

rwarn(sdt, now, type, host)
	time_t sdt, now;
	char *type;
	char *host;
{
	char *ts;
	register delay = sdt - now;
	char *bufp;

	if (delay > 8)
		while (delay % 5)
			delay++;

	(void)sprintf(buf,
	    "\007\007\t*** %sShutdown message for %s from %s@%s ***\r\n\n",
		    type, hostname, shutter, hostname);
	ts = ctime(&sdt);
	bufp = buf + strlen(buf);
	if (delay > 10 MINUTES) {
		(void)sprintf(bufp,
			      "%s going down at %5.5s\r\n", hostname, ts+11);
	}
	else if (delay > 95 SECONDS) {
		(void)sprintf(bufp, "%s going down in %d minute%s\r\n",
		    hostname, (delay+30)/60, (delay+30)/60 != 1 ? "s" : "");
	} else if (delay > 0) {
		(void)sprintf(bufp, "%s going down in %d second%s\r\n",
		    hostname, delay, delay != 1 ? "s" : "");
	} else {
		(void)sprintf(bufp, "%s going down IMMEDIATELY\r\n", hostname);
	}
	rprintf(host, buf);
}

rprintf(host, buf)
	char *host, *buf;
{
#ifdef DEBUG
	int err;
	
		fprintf(stderr, "about to call %s\n", host);
	if (err = callrpcfast(host, (int)WALLPROG, WALLVERS, WALLPROC_WALL,
	    xdr_path, &buf, xdr_void, (char *)NULL)) {
		fprintf(stderr, "couldn't make rpc call ");
		clnt_perrno(err);
		fprintf(stderr, "\n");
	    }
#else /* DEBUG */
	(void)callrpcfast(host, (int)WALLPROG, WALLVERS, WALLPROC_WALL,
			  xdr_path, &buf, xdr_void, (char *)NULL);
#endif
}

gethostlist()
{
 	int s, err;
	char host[256];
	struct mountlist *ml;
	struct hostlist *hl;
	struct sockaddr_in addr;
	struct stat statb;

	/*
	 * if NFS is not installed, don't bother the user about the
	 * mountd failure.
	 */
	if (stat(NFSD, &statb) < 0)
		return;
    
	/* 
	 * check for portmapper
	 */
	get_myaddress(&addr);
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		return;
	if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		return;
	(void)close(s);

	(void)gethostname(host, sizeof(host));
	ml = NULL;
	if (err = callrpc(host, MOUNTPROG, MOUNTVERS, MOUNTPROC_DUMP,
	    xdr_void, 0, xdr_mountlist, &ml)) {
		fprintf(stderr, "shutdown: callrpc ");
		clnt_perrno(err);
		fprintf(stderr, "\n");
		return;
	}
	for (; ml != NULL; ml = ml->ml_nxt) {
		for (hl = hostlist; hl != NULL; hl = hl->nxt)
			if (strcmp(ml->ml_name, hl->host) == 0)
				goto again;
		hl = (struct hostlist *)malloc(sizeof(struct hostlist));
		hl->host = ml->ml_name;
		hl->nxt = hostlist;
		hostlist = hl;
	   again:;
	}
}

/* 
 * Don't want to wait for usual portmapper timeout you get with
 * callrpc or clnt_call, so use rmtcall instead.  Use timeout
 * of 8 secs, based on the per try timeout of 3 secs for rmtcall 
 */
callrpcfast(host, prognum, versnum, procnum, inproc, in, outproc, out)
	char *host;
	xdrproc_t inproc, outproc;
	char **in, *out;
{
	struct sockaddr_in server_addr;
	struct hostent *hp;
	struct timeval timeout;
	int port;

	if ((hp = gethostbyname(host)) == NULL)
		return ((int) RPC_UNKNOWNHOST);
	bcopy(hp->h_addr, (caddr_t)&server_addr.sin_addr,
	      (unsigned)hp->h_length);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port =  0;
	timeout.tv_sec = 8;
	timeout.tv_usec = 0;
	return( (int)pmap_rmtcall(&server_addr, prognum, versnum, procnum,
            inproc, (caddr_t)in, outproc, (caddr_t)out, timeout, &port) );
}
