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
static char rcsid[] = "$Header: su.c 2.6 91/03/20 $";
#endif

#define LOGERR

#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/universe.h>

char	userbuf[16]	= "USER=";
char	homebuf[128]	= "HOME=";
char	shellbuf[128]	= "SHELL=";
char	pathbuf[128]	= "PATH=:/usr/ucb:/bin:/usr/bin";
char	*cleanenv[] = { userbuf, homebuf, shellbuf, pathbuf, 0, 0 };
char	*user = "root";
char	*shell = "/bin/sh";
char	*badsus = "/usr/adm/sus";
char	*logname;
int	fulllogin;
int	fastlogin;

extern char	**environ;
struct	passwd *pwd;
char	*crypt();
char	*getpass();
char	*getenv();
char	*getlogin();

main(argc,argv)
	int argc;
	char *argv[];
{
	char *password;
	char buf[1000];

	openlog("su", LOG_AUTH);

again:
	if (argc > 1 && strcmp(argv[1], "-f") == 0) {
		fastlogin++;
		argc--, argv++;
		goto again;
	}
	if (argc > 1 && strcmp(argv[1], "-") == 0) {
		fulllogin++;
		argc--, argv++;
		goto again;
	}
	if (argc > 1 && argv[1][0] != '-') {
		user = argv[1];
		argc--, argv++;
	}
	if ((pwd = getpwuid(getuid())) == NULL) {
		fprintf(stderr, "Who are you?\n");
		exit(1);
	}
	strcpy(buf, pwd->pw_name);
	if ((pwd = getpwnam(user)) == NULL) {
		fprintf(stderr, "Unknown login: %s\n", user);
		exit(1);
	}
	/*
	 * If run from a pty, getlogin() returns NULL, so use the uid name
	 * instead.  Still spoofable, but better than nothing.	-davest@sequent
	 */
	if ((logname = getlogin()) == 0)
		logname = buf;
	/*
	 * Only allow those in group zero to su to root.
	 */
	if (pwd->pw_uid == 0) {
		struct	group *gr;
		int i;

		if ((gr = getgrnam("root")) != NULL) {
			for (i = 0; gr->gr_mem[i] != NULL; i++)
				if (strcmp(buf, gr->gr_mem[i]) == 0)
					goto userok;
			fprintf(stderr, "Sorry\n");
			syslog(LOG_NOTICE,"ROOT SU by %s refused on %s",
				logname, ttyname(2));
			logerr("ROOT SU by %s REFUSED on %s", 
				logname, ttyname(2));
			exit(2);
		}
	userok:
		setpriority(PRIO_PROCESS, 0, -2);
	}

	if (pwd->pw_passwd[0] == '\0' || getuid() == 0)
		goto ok;
	password = getpass("Password:");
	if (strcmp(pwd->pw_passwd, crypt(password, pwd->pw_passwd)) != 0) {
		fprintf(stderr, "Sorry\n");
		if (pwd->pw_uid == 0) {
			syslog(LOG_CRIT, "BAD SU %s on %s",
				logname, ttyname(2));
			logerr("BADSU: %s %s", logname, ttyname(2));
		}
		exit(2);
	}
ok:
	endpwent();
	if (pwd->pw_uid == 0) {
		logerr("SU: %s %s", logname, ttyname(2));
		syslog(LOG_NOTICE, "%s on %s", logname, ttyname(2));
		closelog();
	}
	if (setgid(pwd->pw_gid) < 0) {
		perror("su: setgid");
		exit(3);
	}
	if (initgroups(user, pwd->pw_gid)) {
		fprintf(stderr, "su: initgroups failed\n");
		exit(4);
	}
	if (setuid(pwd->pw_uid) < 0) {
		perror("su: setuid");
		exit(5);
	}
	if (pwd->pw_shell && *pwd->pw_shell)
		shell = pwd->pw_shell;
	if (fulllogin) {
		cleanenv[4] = getenv("TERM");
		environ = cleanenv;
	}
	if (strcmp(user, "root"))
		setenv("USER", pwd->pw_name, userbuf);
	setenv("SHELL", shell, shellbuf);
	setenv("HOME", pwd->pw_dir, homebuf);
	setpriority(PRIO_PROCESS, 0, 0);
	if (fastlogin) {
		*argv-- = "-f";
		*argv = "su";
	} else if (fulllogin) {
		if (chdir(pwd->pw_dir) < 0) {
			fprintf(stderr, "No directory\n");
			exit(6);
		}
		set_universe(pwd);
		*argv = "-su";
	} else
		*argv = "su";
	execv(shell, argv);
	fprintf(stderr, "No shell\n");
	exit(7);
}

setenv(ename, eval, buf)
	char *ename, *eval, *buf;
{
	register char *cp, *dp;
	register char **ep = environ;

	/*
	 * this assumes an environment variable "ename" already exists
	 */
	while (dp = *ep++) {
		for (cp = ename; *cp == *dp && *cp; cp++, dp++)
			continue;
		if (*cp == 0 && (*dp == '=' || *dp == 0)) {
			strcat(buf, eval);
			*--ep = buf;
			return;
		}
	}
}

char *
getenv(ename)
	char *ename;
{
	register char *cp, *dp;
	register char **ep = environ;

	while (dp = *ep++) {
		for (cp = ename; *cp == *dp && *cp; cp++, dp++)
			continue;
		if (*cp == 0 && (*dp == '=' || *dp == 0))
			return (*--ep);
	}
	return ((char *)0);
}

logerr(fmt, a1, a2, a3)
	char *fmt, *a1, *a2, *a3;
{
#ifdef LOGERR
	FILE *efil = fopen(badsus, "a");

/*	code to print login messages to file /usr/adm/sus	*/
	if (efil != NULL) {
		struct timeval tp;
		struct timezone tzp;
		gettimeofday(&tp, &tzp);
		fprintf(efil,"%.24s ",ctime(&tp.tv_sec));
		fprintf(efil, fmt, a1, a2, a3);
		fprintf(efil, "\n");
		fclose(efil);
	}
#endif
}

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

#define U_MAGIC "universe("

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
			n = universe(U_UCB);
		else if (strcmp("att", q) == 0)
			n = universe(U_ATT);
		else {
			printf("su: unknown universe '%s'.\n", q);
			fflush(stdout);
			return;
		}
		if (n < 0)
			perror("su: universe");
	}
}
