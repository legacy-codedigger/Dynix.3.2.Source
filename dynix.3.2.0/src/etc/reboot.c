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
static char rcsid[] = "$Header: reboot.c 2.4 1991/08/06 21:13:54 $";
#endif

/*
 * Reboot
 */
#include <stdio.h>
#include <sys/reboot.h>
#include <errno.h>
#include <signal.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/syslog.h>


time_t time();

main(argc, argv)
	int argc;
	char **argv;
{
	int howto;
	register i;
	register qflag = 0;
	int needlog = 1;
	char *user, *getlogin();
	struct passwd *pw, *getpwuid();
	char rcbuf[1024];
	char rcargs[1024];

	*rcargs = '\0';
	openlog("reboot", 0, LOG_AUTH);
	argc--, argv++;
	howto = 0;
	while (argc > 0) {
		if (!strcmp(*argv, "-q"))
			qflag++;
		else if (!strcmp(*argv, "-n"))
			howto |= RB_NOSYNC;
		else if (!strcmp(*argv, "-l"))
			needlog = 0;
		/* 
		 * Unknown args may be recognized by rc.shutdown
		 * so we don't flag them as errors.
		 */

		/*
		 * pass all arguments to rc.shutdown
		 */
		(void)strcat(rcargs, " ");
		(void)strcat(rcargs, *argv);

		argc--, argv++;
	}

	if (needlog) {
		user = getlogin();
		if (user == (char *)0 && (pw = getpwuid(getuid())))
			user = pw->pw_name;
		if (user == (char *)0)
			user = "root";
		syslog(LOG_CRIT, "rebooted by %s", user);
	}

	(void)signal(SIGHUP, SIG_IGN);	/* for remote connections */

	if( ! offline_all() ) {
		fprintf(stderr, "reboot: unable to off-line all processors !!\n");
		exit(1);
	}

	if (kill(1, SIGTSTP) == -1) {
		fprintf(stderr, "reboot: can't idle init\n");
		exit(1);
	}
	sleep(1);
	(void) kill(-1, SIGTERM);	/* one chance to catch it */
	sleep(5);

	if (!qflag) for (i = 1; ; i++) {
		if (kill(-1, SIGKILL) == -1) {
			extern int errno;

			if (errno == ESRCH)
				break;

			perror("reboot: kill");
			(void)kill(1, SIGHUP);
			exit(1);
		}
		if (i > 5) {
	fprintf(stderr, "CAUTION: some process(es) wouldn't die\n");
			break;
		}
		setalarm(2 * i);
		(void)pause();
	}

	(void)chdir("/");		  /* out of all mount points */
	if (!qflag) {
		if (!(howto & RB_NOSYNC)) {
			markdown();
		}
		sprintf(rcbuf, "exec sh /etc/rc.shutdown reboot %s",
			rcargs);
		(void)system(rcbuf);
		if (!(howto & RB_NOSYNC)) {
			sync();
			setalarm(5);
			(void)pause();
		}
	}
	reboot(howto);
	perror("reboot");
	(void)kill(1, SIGHUP);
	exit(1);
}

dingdong()
{
	/* RRRIIINNNGGG RRRIIINNNGGG */
}

setalarm(n)
{
	(void)signal(SIGALRM, dingdong);
	(void)alarm((unsigned)n);
}

#include <utmp.h>
#define SCPYN(a, b)	((void)strncpy(a, b, sizeof(a)))
char	wtmpf[]	= "/usr/adm/wtmp";
struct utmp wtmp;

markdown()
{
	register f = open(wtmpf, 1);
	if (f >= 0) {
		(void)lseek(f, (off_t)0, 2);
		SCPYN(wtmp.ut_line, "~");
		SCPYN(wtmp.ut_name, "shutdown");
		SCPYN(wtmp.ut_host, "");
		(void)time(&wtmp.ut_time);
		(void)write(f, (char *)&wtmp, sizeof(wtmp));
		(void)close(f);
	}
}

