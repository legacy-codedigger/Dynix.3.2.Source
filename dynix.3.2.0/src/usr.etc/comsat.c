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
static char rcsid[] = "$Header: comsat.c 2.4 90/06/13 $";
#endif

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)comsat.c	5.16 (Berkeley) 5/9/90";
#endif /* not lint */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>

#include <netinet/in.h>

#include <stdio.h>
#include <sgtty.h>
#include <utmp.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <syslog.h>
#include <strings.h>
#include <ctype.h>

#define _PATH_DEV	"/dev/"
#define _PATH_MAIL	"/usr/spool/mail"
#define _PATH_UTMP	"/etc/utmp"

/*
 * comsat
 */
int	debug = 0;
#define	dsyslog	if (debug) syslog

#define MAXIDLE	120

char	hostname[MAXHOSTNAMELEN];
struct	utmp *utmp = NULL;
time_t	lastmsgtime, time();
int	nutmp, uf;

/* ARGSUSED */
main(argc, argv)
	int argc;
	char **argv;
{
	extern int errno;
	extern char *sys_errlist[];
	register int cc;
	char msgbuf[100];
	struct sockaddr_in from;
	int fromlen;
	void onalrm(), reapchildren();

	/* verify proper invocation */
	fromlen = sizeof(from);
	if (getsockname(0, &from, &fromlen) < 0) {
		(void)fprintf(stderr,
		    "comsat: getsockname: %s.\n", sys_errlist[errno]);
		exit(1);
	}
	openlog("comsat", LOG_PID, LOG_DAEMON);
	if (chdir(_PATH_MAIL)) {
		syslog(LOG_ERR, "chdir: %s: %m", _PATH_MAIL);
		exit(1);
	}
	if ((uf = open(_PATH_UTMP, O_RDONLY, 0)) < 0) {
		syslog(LOG_ERR, ".main: %s: %m", _PATH_UTMP);
		(void) recv(0, msgbuf, sizeof (msgbuf) - 1, 0);
		exit(1);
	}
	(void)time(&lastmsgtime);
	(void)gethostname(hostname, sizeof (hostname));
	onalrm();
	(void)signal(SIGALRM, onalrm);
	(void)signal(SIGTTOU, SIG_IGN);
	(void)signal(SIGCHLD, reapchildren);
	for (;;) {
		cc = recv(0, msgbuf, sizeof (msgbuf) - 1, 0);
		if (cc <= 0) {
			if (errno != EINTR)
				sleep(1);
			errno = 0;
			continue;
		}
		if (!nutmp)		/* no one has logged in yet */
			continue;
		sigblock(sigmask(SIGALRM));
		msgbuf[cc] = 0;
		(void)time(&lastmsgtime);
		mailfor(msgbuf);
		sigsetmask(0L);
	}
}

void
reapchildren()
{
	while (wait3((union wait *)NULL, WNOHANG, (struct rusage *)NULL) > 0);
}

void
onalrm()
{
	static u_int utmpsize;		/* last malloced size for utmp */
	static u_int utmpmtime;		/* last modification time for utmp */
	struct stat statbf;
	off_t lseek();
	char *malloc(), *realloc();

	if (time((time_t *)NULL) - lastmsgtime >= MAXIDLE)
		exit(0);
	(void)alarm((u_int)15);
	(void)fstat(uf, &statbf);
	if (statbf.st_mtime > utmpmtime) {
		utmpmtime = statbf.st_mtime;
		if (statbf.st_size > utmpsize) {
			utmpsize = statbf.st_size + 10 * sizeof(struct utmp);
			if (utmp)
				utmp = (struct utmp *)realloc((char *)utmp, utmpsize);
			else
				utmp = (struct utmp *)malloc(utmpsize);
			if (!utmp) {
				syslog(LOG_ERR, "malloc failed");
				exit(1);
			}
		}
		(void)lseek(uf, 0L, L_SET);
		nutmp = read(uf, utmp, (int)statbf.st_size)/sizeof(struct utmp);
	}
}

mailfor(name)
	char *name;
{
	register struct utmp *utp = &utmp[nutmp];
	register char *cp;
	off_t offset, atol();

	if (!(cp = index(name, '@')))
		return;
	*cp = '\0';
	offset = atoi(cp + 1);
	while (--utp >= utmp)
		if (!strncmp(utp->ut_name, name, sizeof(utmp[0].ut_name)))
			notify(utp, offset);
}

static char *cr;

notify(utp, offset)
	register struct utmp *utp;
	off_t offset;
{
	static char tty[20] = _PATH_DEV;
	struct sgttyb gttybuf;
	FILE *tp;
	char name[sizeof (utmp[0].ut_name) + 1];
	struct stat stb;

	(void)strncpy(tty + 5, utp->ut_line, sizeof(utp->ut_line));
	if (stat(tty, &stb) || !(stb.st_mode & S_IEXEC)) {
		dsyslog(LOG_DEBUG, "%s: wrong mode on %s", utp->ut_name, tty);
		return;
	}
	dsyslog(LOG_DEBUG, "notify %s on %s\n", utp->ut_name, tty);
	if (fork())
		return;
	(void)signal(SIGALRM, SIG_DFL);
	(void)alarm((u_int)30);
	(void)close(0);		/* close socket in child */
	(void)close(uf);	/* close utmp in child */
	if ((tp = fopen(tty, "w")) == NULL) {
		dsyslog(LOG_ERR, "fopen of tty %s failed", tty);
		_exit(-1);
	}
	(void)ioctl(fileno(tp), TIOCGETP, &gttybuf);
	cr = (gttybuf.sg_flags&CRMOD) && !(gttybuf.sg_flags&RAW) ?
	    "\n" : "\n\r";
	(void)strncpy(name, utp->ut_name, sizeof (utp->ut_name));
	name[sizeof (name) - 1] = '\0';
	(void)fprintf(tp, "%s\007New mail for %s@%.*s\007 has arrived:%s----%s",
	    cr, name, sizeof(hostname), hostname, cr, cr);
	jkfprintf(tp, name, offset);
	(void)fclose(tp);
	_exit(0);
}

jkfprintf(tp, name, offset)
	register FILE *tp;
	char name[];
	off_t offset;
{
	register char *cp, ch;
	register FILE *fi;
	register int linecnt, charcnt, inheader;
	char line[BUFSIZ];
	off_t fseek();

	if ((fi = fopen(name, "r")) == NULL)
		return;
	(void)fseek(fi, offset, L_SET);
	/* 
	 * Print the first 7 lines or 560 characters of the new mail
	 * (whichever comes first).  Skip header crap other than
	 * From, Subject, To, and Date.
	 */
	linecnt = 7;
	charcnt = 560;
	inheader = 1;
	while (fgets(line, sizeof (line), fi) != NULL) {
		if (inheader) {
			if (line[0] == '\n') {
				inheader = 0;
				continue;
			}
			if (line[0] == ' ' || line[0] == '\t' ||
			    strncmp(line, "From:", 5) &&
			    strncmp(line, "Subject:", 8))
				continue;
		}
		if (linecnt <= 0 || charcnt <= 0) {
			(void)fprintf(tp, "...more...%s", cr);
			return;
		}
		/* strip weird stuff so can't trojan horse stupid terminals */
		for (cp = line; (ch = *cp) && ch != '\n'; ++cp, --charcnt) {
			ch &= 0x7f;
			if (ch == '\f' || ch == '\v' || 
			    !isprint(ch) && !isspace(ch)) {
				ch |= 0x40;
				(void)fputc('^', tp);
			}
			(void)fputc(ch, tp);
		}
		(void)fputs(cr, tp);
		--linecnt;
	}
	(void)fprintf(tp, "----%s\n", cr);
}
