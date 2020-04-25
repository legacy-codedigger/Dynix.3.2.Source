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

#ifndef lint
static char rcsid[] = "$Header: sysv.c 1.5 91/02/27 $";
#endif

/*
 * Sequent additions for dual universe support.
 */

#include <sys/param.h>
#include <ufs/quota.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/file.h>
#include <sys/universe.h>

#include <sgtty.h>
#include <utmp.h>
#include <signal.h>
#include <pwd.h>
#include <stdio.h>
#include <lastlog.h>
#include <errno.h>

#include "pathnames.h"

extern int	univ;

extern  struct	utmp utmp;
#define NMAX	sizeof(utmp.ut_name)
struct	passwd nouser = {"", "nope", -1, -1, -1, "", "", "", "" };
char	rusername[NMAX+1], lusername[NMAX+1];
extern	char	term[64];
extern	struct	passwd *pwd;
char	*strcat(), *rindex(), *index(), *sprintf();

#ifdef SCGACCT
#include <local/scgacct.h>
/*
 * get the account for
 * this user, and set it.
 */
initaccount()
{
	register struct scgacct *ap;
	struct scgacct *getacuid();
	register int i, nacct, c;
	char acct;

	if((ap = getacuid(pwd->pw_uid)) == NULL)
	{
		acct = '0'; /* default */
		goto GOTACCT;
	}
	nacct = 0; 
	while(ap->a_id[nacct] != '\0' && nacct < N_SCGACCT) /* '\0' is marker */
	{
		nacct++;
	}
	if(nacct == 1) /* just one account, so skip the interrogation */
	{
		acct = ap->a_id[0];
		goto GOTACCT;
	}

	for(;;) 
	{
		printf("Account: ");
		fflush(stdout);

		for(acct = 0; (c = getchar()) != '\n'; ) 
		{
			if(c <= 0)	
				exit(0);
			if(acct == 0)
				acct = c;
		}

		for(i = 0; i < nacct; i++)
		{
			if(acct == ap->a_id[i])
				goto GOTACCT;
		}
		printf("Legal Accounts\n");
		fflush(stdout);
		for(i = 0; i < nacct; i++)
		{
			printf("%c - %s\n", ap->a_id[i], ap->a_gno[i]);
			fflush(stdout);
		}
	}
GOTACCT:
	if (setacct(acct) < 0)
		perror("setacct");
	utmp.ut_account = acct;  /* for session accounting */
}
#endif

/*
 * Is s1 a substring of s2?
 */
char *
substr(s1, s2)
	register char *s1, *s2;
{
	register n = strlen(s1);
	
	if (n == 0) 
		return (NULL);
	while (s2 && *s2) {
		if (*s1 == *s2 && strncmp(s1, s2, n) == 0)
			return (s2);
		++s2;
	}
	return (NULL);
}

/*
 * Look for universe specification in the GECOS
 * field of the passwd file entry.  Format requires
 * the string of the form "universe(UNIVERSE)"
 * where UNIVERSE is one of "att" or "ucb".
 */

#define	U_MAGIC "universe("

set_universe(p)
	struct passwd *p;
{
	register char *s, *q;
	int n;

	if ((s = substr(U_MAGIC, p->pw_gecos)) != NULL) {
		/* skip to actual universe name */
		s = q = s + strlen(U_MAGIC);
		while (*s && *s != ')')
			++s;
		if (*s == '\0' || s == q) return;
		*s = '\0';
		if (strcmp("ucb", q) == 0)
			n = universe(univ = U_UCB);
		else if (strcmp("att", q) == 0)
			n = universe(univ = U_ATT);
		else {
			printf("login: unknown universe '%s'.\n", q);
			fflush(stdout);
			return;
		}
		if (n < 0)
			perror("login: universe");
	}
}

set_tz(tz)
	char *tz;
{
	struct	timeval tval;
	struct	timezone tzone;
	char   *timezone(), tzh[4];

	gettimeofday(&tval, &tzone);
	if (DST_NONE != tzone.tz_dsttime) {
		(void)sprintf(tz, "%s%d%s", timezone(tzone.tz_minuteswest, 0),
					    tzone.tz_minuteswest/60,
					    timezone(tzone.tz_minuteswest, 1));
					    /* daylight saving */
	} else {
		(void)sprintf(tz, "%s%d", timezone(tzone.tz_minuteswest, 0),
					  tzone.tz_minuteswest/60);
	}
}

set_dual(pwd)
struct	passwd *pwd;
{
	char	timzone[20];
	char	mailbox[128];
	char	*ucbshell;
	char	*attshell;
	char	*path;

	set_tz(timzone);

	(void)setenv("TZ", timzone, 0);
	if (univ == U_UCB) {
		ucbshell = pwd->pw_shell;
		attshell = "/bin/sh";
		path = ":/usr/ucb:/bin:/usr/bin";
	} else {
		ucbshell = "/bin/sh";
		attshell = pwd->pw_shell;
		path = ":/bin:/usr/bin";
	}
	(void)setenv("UCBPATH", ":/usr/ucb:/bin:/usr/bin", 1);
	(void)setenv("ATTPATH", ":/bin:/usr/bin", 1);
	(void)setenv("UCBSHELL", ucbshell ,1);
	(void)setenv("ATTSHELL", attshell, 1);
	(void)setenv("LOGNAME", pwd->pw_name, 0);
	(void)sprintf(mailbox,"%s/%s", _PATH_MAILDIR, pwd->pw_name);
	(void)setenv("MAIL", mailbox, 0);
	(void)setenv("PATH", path, 0);
}
