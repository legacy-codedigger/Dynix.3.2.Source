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
static char rcsid[] = "$Header: login.c 1.18 1991/06/12 23:55:51 $";
#endif

/*
 * login [ name ]
 * login -h hostname	(for telnetd, etc.)
 * login -f name	(for pre-authenticated login: datakit, xterm, etc.)
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
#include <errno.h>
#include <syslog.h>
#include <grp.h>
#include <pwd.h>
#include <setjmp.h>
#include <stdio.h>
#include <strings.h>
#include <lastlog.h>
#include "pathnames.h"

#include "pathnames.h"

#define NSTACK	(1024*1024)		/* kernel default stack size limit */
#define	TTYGRPNAME	"tty"		/* name of group to own ttys */


/*
 * This bounds the time given to login.  Not a define so it can
 * be patched on machines where it's too small.
 */
unsigned timeout = 240;

struct	passwd *pwd;
int	failures;
char	term[64], *hostname, *username, *tty;

struct	sgttyb sgttyb;
struct	tchars tc = {
	CINTR, CQUIT, CSTART, CSTOP, CEOT, CBRK
};
struct	ltchars ltc = {
	CSUSP, CDSUSP, CRPRNT, CFLUSH, CWERASE, CLNEXT
};

#ifdef EXPIRE
char *months[] =
	{ "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug",
	  "Sep", "Oct", "Nov", "Dec" };
#endif
/*
 * This is the max number of tries the user gets to get his password
 * right.
 */
int maxtries = 3;

struct	utmp utmp;
struct	passwd *getpwnam();
char	*strcat(), *rindex(), *index(), *sprintf();
int	setpwent();
int	timedout();
extern	char **environ;
extern	int errno;

#define NMAX	sizeof(utmp.ut_name)
#ifndef UT_NAMESIZE
#define UT_NAMESIZE	NMAX
#endif

char	QUOTAWARN[] =   "/usr/ucb/quota";       /* warn user about quotas */
char	CANTRUN[] =     "login: Can't run ";
char	securetty[] =	"/etc/securetty";

int	univ = U_UCB;
int	fflag;
int	pflag;
int	hflag;
int	usererr = -1;

main(argc, argv)
	int argc;
	char **argv;
{
	extern int errno, optind;
	extern char *optarg, **environ;
	register int t;
	struct group *gr;
	register int ch;
	char	*p;
	int zero = 0;
	int locl = LCRTBS|LCTLECH|LDECCTQ;
	extern char _sobuf[];
        int ask,cnt;
	int quietlog, passwd_req, ioctlval, timedout();
	char *domain, *salt, *envinit[1], *ttyn, *pp;
	char tbuf[MAXPATHLEN + 2], tname[sizeof(_PATH_TTY) + 10];
	char *ctime(), *ttyname(), *stypeof(), *crypt(), *getpass();
	time_t time();
	off_t lseek();
	char	*namep;
	struct	rlimit	rl;
	int	r;
	int	uid;

	setbuf(stdout, _sobuf);
	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGINT, SIG_IGN);
	(void)signal(SIGTSTP, SIG_IGN);
	(void)signal(SIGALRM, timedout);
	(void)alarm(timeout);
	(void)setpriority(PRIO_PROCESS, 0, 0);

	for (t = getdtablesize(); t >= 3; t--)
		close(t);

	/*
	 * Reset all the resource limits to the max.
	 * This caused a problem when the file size limit
	 * was set to 750K and the wtmp file was larger
	 * than this.  login would bomb.
	 */
	for (r=0; r < RLIM_NLIMITS; r++) {
		if (getrlimit(r, &rl) == 0) {
			rl.rlim_cur = rl.rlim_max;
			(void) setrlimit(r, &rl);
		}
	}
	/*
	 * Set STACK current limit to NSTACK (1024k). (This is
	 * what the kernel default is.)
	 */
	if (getrlimit(RLIMIT_STACK, &rl) == 0) {
		if (rl.rlim_cur > NSTACK)
			rl.rlim_cur = NSTACK;
		(void) setrlimit(RLIMIT_STACK, &rl);
	}

        openlog("login", LOG_ODELAY, LOG_AUTH);

	/*
	 * -p is used by getty to tell login not to destroy environment
	 * -f is used to skip a second login authentication
	 * -h is used by other servers to pass the name of the
	 * remote host to login so that it may be placed in utmp and wtmp
	 */
	domain = NULL;
	if (gethostname(tbuf, sizeof(tbuf)) < 0)
		syslog(LOG_ERR, "couldn't get local hostname: %m");
	else
		domain = index(tbuf, '.');

	fflag = hflag = pflag = 0;
	passwd_req = 1;
	uid = getuid();
	while ((ch = getopt(argc, argv, "ph:f")) != EOF) {
		switch (ch) {
		case 'f':
			fflag = 1;
			break;
		case 'h':
			if (uid) {
				(void)fprintf(stderr,
				    "login: -h for super-user only.\n");
				exit(1);
			}
			hflag = 1;
			if (domain && (p = index(optarg, '.')) &&
					strcasecmp(p, domain) == 0)
				*p = '\0';
			hostname = optarg;
			break;
		case 'p':
			pflag = 1;
			break;
		default:
			(void)fprintf(stderr,
				"usage: login [-fp] [username]\n");
			exit(1);
		}
	}
	argc -= optind;
	argv += optind;
	if (*argv) {
		username = *argv;
		if (strlen(username) > UT_NAMESIZE)
			username[UT_NAMESIZE] = '\0';
		ask = (*username == '\0');
	} else
		ask = 1;

	ioctlval = 0;
	(void)ioctl(0, TIOCLSET, &ioctlval);
	(void)ioctl(0, TIOCNXCL, 0);
	(void)fcntl(0, F_SETFL, ioctlval); /* not sure on this !!!! */
	(void)ioctl(0, FIONBIO, &zero);
	(void)ioctl(0, TIOCGETP, &sgttyb);
	sgttyb.sg_erase = CERASE;
	sgttyb.sg_kill = CKILL;
	(void) universe(univ);
	(void)ioctl(0, FIOASYNC, &zero);
	if (sgttyb.sg_ospeed >= B1200)
		locl |= LCRTERA|LCRTKIL;
	(void)ioctl(0, TIOCLSET, &locl);
	(void)ioctl(0, TIOCSLTC, &ltc);
	(void)ioctl(0, TIOCSETC, &tc);
	(void)ioctl(0, TIOCSETP, &sgttyb);

	ttyn = ttyname(0);
	if (ttyn == NULL || *ttyn == '\0') {
		(void)sprintf(tname, "%s??", _PATH_TTY);
		ttyn = tname;
	}
	if (tty = rindex(ttyn, '/'))
		++tty;
	else
		tty = ttyn;


	for (cnt = 0;; ask = 1) {
		ioctlval = 0;
		(void)ioctl(0, TIOCSETD, &ioctlval);

		if (ask) {
			fflag = 0;
			getloginname();
		}

		/*
		 * Note if trying multiple user names;
		 * log failures for previous user name,
		 * but don't bother logging one failure
		 * for nonexistent name (mistyped username).
		 */
		if (failures && strcmp(tbuf, username)) {
			if (failures > (pwd ? 0 : 1))
				badlogin(tbuf);
			failures = 0;
		}
		(void)strcpy(tbuf, username);
		if (pwd = getpwnam(username))
			salt = pwd->pw_passwd;
		else
			salt = "xx";

		/*
		 * Disallow automatic login to root; if not invoked by
		 * root, disallow if the uid's differ.
		 */
		if (pwd && fflag && (uid == 0 || uid == pwd->pw_uid)) {
				passwd_req = 0;	
		}
				
		if (passwd_req && (!pwd || *pwd->pw_passwd)) {
			setpriority(PRIO_PROCESS, 0, -4);
			pp = getpass("Password:");
			p = crypt(pp, salt);
			setpriority(PRIO_PROCESS, 0, 0);
			(void) bzero(pp, strlen(pp));
			if (!pwd || strcmp(p, pwd->pw_passwd))
				goto bad;
		}

		/* if user not super-user, check for disabled logins */
		if (pwd == NULL || pwd->pw_uid)
			checknologin();
		set_universe(pwd);

		/*
		 * If trying to log in as root, but with insecure terminal,
		 * refuse the login attempt.
		 */
		if (pwd && pwd->pw_uid == 0 && !rootterm(tty)) {
			(void)fprintf(stderr,
			    "%s login refused on this terminal.\n",
			    pwd->pw_name);
			if (hostname)
				syslog(LOG_NOTICE,
				    "LOGIN %s REFUSED FROM %s ON TTY %s",
				    pwd->pw_name, hostname, tty);
			else
				syslog(LOG_NOTICE,
				    "LOGIN %s REFUSED ON TTY %s",
				     pwd->pw_name, tty);
			continue;
		}

		/*
		 * If no pre-authentication and a password exists
		 * for this user, prompt for one and verify it.
		 */
		if (!passwd_req || (pwd && !*pwd->pw_passwd))
			break;

		if (pwd && !strcmp(p, pwd->pw_passwd))
			break;

bad:
		(void)printf("Login incorrect\n");
		fflush(stdout);
		failures++;
		/* we allow 10 tries, but after maxtries we start backing off */
		if (++cnt > maxtries) {
			if (cnt >= 10) {
				badlogin(username);
				(void)ioctl(0, TIOCHPCL, (struct sgttyb *)NULL);
				sleepexit(1);
			}
			sleep((u_int)((cnt - 3) * 5));
		}
	}
/* committed to login turn off timeout */
	(void)alarm(0);

#ifdef	SCGACCT
	initaccount();
#endif
	/* paranoia... */
	endpwent();

	if (chdir(pwd->pw_dir) < 0) {
		(void)fprintf(stderr, "No directory %s!\n", pwd->pw_dir);
		if (chdir("/"))
			exit(0);
		pwd->pw_dir = "/";
		(void)fprintf(stderr, "Logging in with home = \"/\".\n");
		fflush(stdout);
	}

	quietlog = access(_PATH_HUSHLOGIN, F_OK) == 0;


#ifdef EXPIRE
#define	TWOWEEKS	(14*24*60*60)
	if (pwd->pw_change || pwd->pw_expire)
		(void)gettimeofday(&tp, (struct timezone *)NULL);
	if (pwd->pw_change)
		if (tp.tv_sec >= pwd->pw_change) {
			(void)printf("Sorry -- your password has expired.\n");
			fflush(stdout);
			sleepexit(1);
		}
		else if (tp.tv_sec - pwd->pw_change < TWOWEEKS && !quietlog) {
			ttp = localtime(&pwd->pw_change);
			(void)printf("Warning: your password expires on %s %d, %d\n",
			    months[ttp->tm_mon], ttp->tm_mday, TM_YEAR_BASE + ttp->tm_year);
			fflush(stdout);
		}
	if (pwd->pw_expire)
		if (tp.tv_sec >= pwd->pw_expire) {
			(void)printf("Sorry -- your account has expired.\n");
			sleepexit(1);
			fflush(stdout);
		}
		else if (tp.tv_sec - pwd->pw_expire < TWOWEEKS && !quietlog) {
			ttp = localtime(&pwd->pw_expire);
			(void)printf("Warning: your account expires on %s %d, %d\n",
			    months[ttp->tm_mon], ttp->tm_mday, TM_YEAR_BASE + ttp->tm_year);
			fflush(stdout);
		}
#endif

	/* nothing else left to fail -- really log in */
	{
		struct utmp utmp;

		bzero((char *)&utmp, sizeof(utmp));
		(void)time(&utmp.ut_time);
		strncpy(utmp.ut_name, username, sizeof(utmp.ut_name));
		if (hostname)
			strncpy(utmp.ut_host, hostname, sizeof(utmp.ut_host));
		strncpy(utmp.ut_line, tty, sizeof(utmp.ut_line));
		login(&utmp);
	}

	dolastlog(quietlog);

	if (!hflag)  {
		static struct winsize win = { 0, 0, 0, 0 };

		(void)ioctl(0, TIOCSWINSZ, &win);
	}

	(void)chown(ttyn, pwd->pw_uid,
	    (gr = getgrnam(TTYGRPNAME)) ? gr->gr_gid : pwd->pw_gid);
	(void)chmod(ttyn, 0600);
	(void)setgid(pwd->pw_gid);

	initgroups(username, pwd->pw_gid);

#ifdef BSD4_3
	quota(Q_DOWARN, pwd->pw_uid, (dev_t)-1, 0);
#endif
	if (*pwd->pw_shell == '\0')
		pwd->pw_shell = _PATH_BSHELL;
	/* turn on new line discipline for the csh */
	else if (!strcmp(pwd->pw_shell, _PATH_CSHELL)) {
		ioctlval = NTTYDISC;
		(void)ioctl(0, TIOCSETD, &ioctlval);
	}

	/* destroy environment unless user has requested preservation */
	if (!pflag)
		environ = envinit;
	(void)setenv("HOME", pwd->pw_dir, 1);
	(void)setenv("SHELL", pwd->pw_shell, 1);
	if (term[0] == '\0')
		strncpy(term, stypeof(tty), sizeof(term));
	(void)setenv("TERM", term, 0);
	(void)setenv("USER", pwd->pw_name, 1);
	set_dual(pwd);
	(void)setenv("PATH", _PATH_DEFPATH, 0);

	if (tty[sizeof("tty")-1] == 'd')
		syslog(LOG_INFO, "DIALUP %s, %s", tty, pwd->pw_name);
	/* if fflag is on, assume caller/authenticator has logged root login */
	if (pwd->pw_uid == 0 && fflag == 0)
		if (hostname)
			syslog(LOG_NOTICE, "ROOT LOGIN ON %s FROM %s",
			    tty, hostname);
		else
			syslog(LOG_NOTICE, "ROOT LOGIN ON %s", tty);

	umask(022);

	if (!quietlog && (univ == U_UCB)) {
		struct stat st;
		int pid, w;

		motd();
		(void)sprintf(tbuf, "%s/%s", _PATH_MAILDIR, pwd->pw_name);
		if (stat(tbuf, &st) == 0 && st.st_size != 0) {
			(void)printf("You have %smail.\n",
			    (st.st_mtime > st.st_atime) ? "new " : "");
			fflush(stdout);
		}
		if ((pid = vfork()) == 0) {
		execl(QUOTAWARN, QUOTAWARN, pwd->pw_name, (char *)0);
		write(2, CANTRUN, sizeof(CANTRUN));
			perror(QUOTAWARN);
			_exit(127);
		} else if (pid == -1) {
			fprintf(stderr, CANTRUN);
			fflush(stdout);
			perror(QUOTAWARN);
		} else {
			while ((w = wait((int *)NULL)) != pid && w != -1)
				;
		}
	}

	(void)signal(SIGALRM, SIG_DFL);
	(void)signal(SIGQUIT, SIG_DFL);
	(void)signal(SIGINT, SIG_DFL);

	tbuf[0] = '-';
	strcpy(tbuf + 1, (p = rindex(pwd->pw_shell, '/')) ?
	    p + 1 : pwd->pw_shell);

	/* discard permissions last so can't get killed and drop core */
	(void)setuid(pwd->pw_uid);

	execlp(pwd->pw_shell, tbuf, 0);
	(void)fprintf(stderr, "login: no shell: %s.\n", strerror(errno));
	perror(pwd->pw_shell);
	exit(0);
}


getloginname()
{
	register int ch;
	register char *p;
	static char nbuf[UT_NAMESIZE + 1];

	for (;;) {
		(void)printf("login: ");
		fflush(stdout);
		for (p = nbuf; (ch = getchar()) != '\n'; ) {
			if (ch == EOF) {
				badlogin(username);
				exit(0);
			}
			if (p < nbuf + UT_NAMESIZE)
				*p++ = ch;
		}
		if (p > nbuf)
			if (nbuf[0] == '-')
				(void)fprintf(stderr,
				    "login names may not start with '-'.\n");
			else {
				*p = '\0';
				username = nbuf;
				break;
			}
	}
}

timedout()
{

	(void)fprintf(stderr, "Login timed out after %d seconds\n", timeout);
	exit(0);
}

rootterm(tty)
/*
 * returns 1 if tty is in /etc/securetty or this file is non-existant,
 * 0 otherwise.
 */
	char *tty;
{
	register FILE *fd;
	char buf[100];

	if ((fd = fopen(securetty, "r")) == NULL)
		return(1);
	while (fgets(buf, sizeof buf, fd) != NULL) {
		buf[strlen(buf)-1] = '\0';
		if (strcmp(tty, buf) == 0) {
			fclose(fd);
			return(1);
		}
	}
	fclose(fd);
	return(0);
}


jmp_buf motdinterrupt;

motd()
{
	register int fd, nchars;
	int (*oldint)(), sigint();
	char tbuf[8192];

	if ((fd = open(_PATH_MOTDFILE, O_RDONLY, 0)) < 0)
		return;
	oldint = signal(SIGINT, sigint);
	if (setjmp(motdinterrupt) == 0)
		while ((nchars = read(fd, tbuf, sizeof(tbuf))) > 0)
			(void)write(fileno(stdout), tbuf, nchars);
	(void)signal(SIGINT, oldint);
	(void)close(fd);
}

sigint()
{
	longjmp(motdinterrupt, 1);
}

checknologin()
{
	register int fd, nchars;
	char tbuf[8192];

	if ((fd = open(_PATH_NOLOGIN, O_RDONLY, 0)) >= 0) {
		fflush(stdout);
		while ((nchars = read(fd, tbuf, sizeof(tbuf))) > 0)
			(void)write(fileno(stdout), tbuf, nchars);
		close(fd);
		sleepexit(0);
	}
}

dolastlog(quiet)
	int quiet;
{
	struct lastlog ll;
	int fd;
	char *ctime();

	if ((fd = open(_PATH_LASTLOG, O_RDWR, 0)) >= 0) {
		(void)lseek(fd, (off_t)pwd->pw_uid * sizeof(ll), L_SET);
		if (!quiet) {
			if (read(fd, (char *)&ll, sizeof(ll)) == sizeof(ll) &&
			    ll.ll_time != 0) {
				(void)printf("Last login: %.*s ",
				    24-5, (char *)ctime(&ll.ll_time));
				if (*ll.ll_host != '\0')
					(void)printf("from %.*s\n",
					    sizeof(ll.ll_host), ll.ll_host);
				else
					(void)printf("on %.*s\n",
					    sizeof(ll.ll_line), ll.ll_line);
				fflush(stdout);
			}
			(void)lseek(fd, (off_t)pwd->pw_uid * sizeof(ll), L_SET);
		}
		bzero((char *)&ll, sizeof(ll));
		(void)time(&ll.ll_time);
		strncpy(ll.ll_line, tty, sizeof(ll.ll_line));
		if (hostname)
			strncpy(ll.ll_host, hostname, sizeof(ll.ll_host));
		(void)write(fd, (char *)&ll, sizeof(ll));
		(void)close(fd);
	}
}

badlogin(name)
	char *name;
{
	if (failures == 0)
		return;
	if (hostname)
		syslog(LOG_NOTICE, "%d LOGIN FAILURE%s FROM %s, %s",
		    failures, failures > 1 ? "S" : "", hostname, name);
	else
		syslog(LOG_NOTICE, "%d LOGIN FAILURE%s ON %s, %s",
		    failures, failures > 1 ? "S" : "", tty, name);
}

#undef	UNKNOWN
#define	UNKNOWN	"su"

char *
stypeof(ttyid)
	char *ttyid;
{
	static char typebuf[16];
	char buf[50];
	register FILE *f;
	register char *p, *t, *q;

	if (ttyid == NULL)
		return (UNKNOWN);
	f = fopen("/etc/ttytype", "r");
	if (f == NULL)
		return (UNKNOWN);
	/* split off end of name */
	for (p = q = ttyid; *p != 0; p++)
		if (*p == '/')
			q = p + 1;

	/* scan the file */
	while (fgets(buf, sizeof buf, f) != NULL) {
		for (t = buf; *t != ' ' && *t != '\t'; t++)
			if (*t == '\0')
				goto next;
		*t++ = 0;
		while (*t == ' ' || *t == '\t')
			t++;
		for (p = t; *p > ' '; p++)
			;
		*p = 0;
		if (strcmp(q,t) == 0) {
			strcpy(typebuf, buf);
			fclose(f);
			return (typebuf);
		}
	next: ;
	}
	fclose (f);
	return (UNKNOWN);
}


getstr(buf, cnt, err)
	char *buf, *err;
	int cnt;
{
	char ch;

	do {
		if (read(0, &ch, sizeof(ch)) != sizeof(ch))
			exit(1);
		if (--cnt < 0) {
			(void)fprintf(stderr, "%s too long\r\n", err);
			sleepexit(1);
		}
		*buf++ = ch;
	} while (ch);
}

sleepexit(eval)
	int eval;
{
	sleep((u_int)5);
	exit(eval);
}
