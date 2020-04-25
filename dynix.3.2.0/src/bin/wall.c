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
static char rcsid[] = "$Header: wall.c 2.0 86/01/28 $";
#endif

/*
 * wall.c - Broadcast a message to all users.
 *
 * This program is not related to David Wall, whose Stanford Ph.D. thesis
 * is entitled "Mechanisms for Broadcast and Selective Broadcast".
 */

#include <stdio.h>
#include <utmp.h>
#include <signal.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define IGNOREUSER	"sleeper"

char	hostname[32];
char	mesg[3000];
int	msize,sline;
struct	utmp *utmp;
char	*strcpy();
char	*strcat();
char	*malloc();
char	who[9] = "???";
long	clock, time();
struct tm *localtime();
struct tm *localclock;

main(argc, argv)
char *argv[];
{
	register int i, c;
	register struct utmp *p;
	int f;
	struct stat statb;
	FILE *mf;

	(void) gethostname(hostname, sizeof (hostname));
	if ((f = open("/etc/utmp", O_RDONLY, 0)) < 0) {
		fprintf(stderr, "Cannot open /etc/utmp\n");
		exit(1);
	}
	clock = time( 0 );
	localclock = localtime( &clock );
	mf = stdin;
	if(argc >= 2) {
		/* take message from unix file instead of standard input */
		if((mf = fopen(argv[1], "r")) == NULL) {
			fprintf(stderr,"Cannot open %s\n", argv[1]);
			exit(1);
		}
	}
	while((i = getc(mf)) != EOF) {
		if (msize >= sizeof mesg) {
			fprintf(stderr, "Message too long\n");
			exit(1);
		}
		mesg[msize++] = i;
	}
	fclose(mf);
	sline = ttyslot();	/* 'utmp' slot no. of sender */
	(void) fstat(f, &statb);
	utmp = (struct utmp *)malloc(statb.st_size);
	c = read(f, (char *)utmp, statb.st_size);
	(void) close(f);
	c /= sizeof(struct utmp);
	if (sline)
		strncpy(who, utmp[sline].ut_name, sizeof(utmp[sline].ut_name));
	for (i=0; i<c; i++) {
		p = &utmp[i];
		if ((p->ut_name[0] == 0) ||
		    (strncmp (p->ut_name, IGNOREUSER, sizeof(p->ut_name)) == 0))
			continue;
		sendmes(p->ut_line);
	}
	exit(0);
}

sendmes(tty)
char *tty;
{
	register i;
	char t[50], buf[BUFSIZ];
	register char *cp;
	register int c, ch;
	FILE *f;

	while ((i = fork()) == -1)
		if (wait((int *)0) == -1) {
			fprintf(stderr, "Try again\n");
			return;
		}
	if(i)
		return;
	strcpy(t, "/dev/");
	strcat(t, tty);

	signal(SIGALRM, SIG_DFL);	/* blow away if open hangs */
	alarm(10);

	if((f = fopen(t, "w")) == NULL) {
		fprintf(stderr,"cannot open %s\n", t);
		exit(1);
	}
	setbuf(f, buf);
	fprintf(f,
	    "\nBroadcast Message from %s!%s (%.*s) at %d:%02d ...\r\n\n"
		, hostname
		, who
		, sizeof(utmp[sline].ut_line)
		, utmp[sline].ut_line
		, localclock -> tm_hour
		, localclock -> tm_min
	);
	/* fwrite(mesg, msize, 1, f); */
	for (cp = mesg, c = msize; c-- > 0; cp++) {
		ch = *cp;
		if (ch == '\n')
			putc('\r', f);
		putc(ch, f);
	}

	exit(0);
}
