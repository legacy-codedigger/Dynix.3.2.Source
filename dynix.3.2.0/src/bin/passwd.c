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
static char rcsid[] = "$Header: passwd.c 2.4 89/07/05 $";
#endif

/*
 * passwd
 *
 * Enter a password in the password file.
 * This program should be suid with an owner
 * with write permission on /etc/passwd.
 */
#include <sys/file.h>

#include <stdio.h>
#include <signal.h>
#include <pwd.h>
#include <errno.h>

/* __PASSWD_ defined in libc/gen/getpwent.c */
extern char *__PASSWD_;
char	*temp = "/etc/ptmp";
struct	passwd *pwd;
struct	passwd *_getpwent();
int	_endpwent();
char	*strcpy();
char	*crypt();
char	*getpass();
char	*getlogin();
char	*pw;
char	pwbuf[10];
char	hostname[256];
extern	int errno;

main(argc, argv)
	char *argv[];
{
	char *p;
	int i;
	char saltc[2];
	long salt;
	int u;
	int insist;
	int ok, flags;
	int c, pwlen, fd;
	FILE *tf;
	char *uname;

	insist = 0;
	uname = NULL;
	while (argc > 1) {
		if (argv[1][0] == '-') {
			if (argv[1][1] != 'f' || argc < 3) {
				usage();
				exit(1);
			}
			__PASSWD_ = argv[2];
			/* Make new lock file name for specified
			 * passwd file, then drop down to invoker's
			 * uid.  Invoker must be able to write
			 * alternate passwd file anyway, and this
			 * avoids leaving root owned files laying
			 * about.  Down side is that 'passwd -f
			 * /etc/passwd' doesn't work, and shouldn't be
			 * used (different lock file name).
			 */
			temp = (char *) malloc(strlen(__PASSWD_)+5);
			strcpy(temp, __PASSWD_);
			strcat(temp, "ptmp");
			setuid(getuid());
			argc--;
			argv++;
		}
		else {
			if (uname) {
				usage();
				exit(1);
			}
			uname = argv[1];
		}
		argc--;
		argv++;
	}
	if (uname == NULL) {
		if ((uname = getlogin()) == NULL) {
			usage();
			exit(1);
		}
		gethostname(hostname, sizeof(hostname));
		printf("Changing password for %s on %s\n", uname, hostname);
	}

	while (((pwd = _getpwent()) != NULL) && strcmp(pwd->pw_name, uname))
		;
	u = getuid();
	if (pwd == NULL) {
		printf("Not in passwd file.\n");
		exit(1);
	}
	if (u != 0 && u != pwd->pw_uid) {
		printf("Permission denied.\n");
		exit(1);
	}
	_endpwent();
	if (pwd->pw_passwd[0] && u != 0) {
		strcpy(pwbuf, getpass("Old password:"));
		pw = crypt(pwbuf, pwd->pw_passwd);
		if (strcmp(pw, pwd->pw_passwd) != 0) {
			printf("Sorry.\n");
			exit(1);
		}
	}
tryagain:
	strcpy(pwbuf, getpass("New password:"));
	pwlen = strlen(pwbuf);
	if (pwlen == 0) {
		printf("Password unchanged.\n");
		exit(1);
	}
	/*
	 * Insure password is of reasonable length and
	 * composition.  If we really wanted to make things
	 * sticky, we could check the dictionary for common
	 * words, but then things would really be slow.
	 */
	ok = 0;
	flags = 0;
	p = pwbuf;
	while (c = *p++) {
		if (c >= 'a' && c <= 'z')
			flags |= 2;
		else if (c >= 'A' && c <= 'Z')
			flags |= 4;
		else if (c >= '0' && c <= '9')
			flags |= 1;
		else
			flags |= 8;
	}
	if (flags >= 7 && pwlen >= 4)
		ok = 1;
	if ((flags == 2 || flags == 4) && pwlen >= 6)
		ok = 1;
	if ((flags == 3 || flags == 5 || flags == 6) && pwlen >= 5)
		ok = 1;
	if (!ok && insist < 2) {
		printf("Please use %s.\n", flags == 1 ?
			"at least one non-numeric character" :
			"a longer password");
		insist++;
		goto tryagain;
	}
	if (strcmp(pwbuf, getpass("Retype new password:")) != 0) {
		printf("Mismatch - password unchanged.\n");
		exit(1);
	}
	time(&salt);
	salt = 9 * getpid();
	saltc[0] = salt & 077;
	saltc[1] = (salt>>6) & 077;
	for (i = 0; i < 2; i++) {
		c = saltc[i] + '.';
		if (c > '9')
			c += 7;
		if (c > 'Z')
			c += 6;
		saltc[i] = c;
	}
	pw = crypt(pwbuf, saltc);
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	(void) umask(0);
	fd = open(temp, O_WRONLY|O_CREAT|O_EXCL, 0644);
	if (fd < 0) {
		fprintf(stderr, "passwd: ");
		if (errno == EEXIST)
			fprintf(stderr, "password file busy - try again.\n");
		else
			perror(temp);
		exit(1);
	}
	signal(SIGTSTP, SIG_IGN);
	if ((tf = fdopen(fd, "w")) == NULL) {
		fprintf(stderr, "passwd: fdopen failed?\n");
		exit(1);
	}
	/*
	 * Copy passwd to temp, replacing matching lines
	 * with new password.
	 */
	while ((pwd = _getpwent()) != NULL) {
		if (strcmp(pwd->pw_name,uname) == 0) {
			u = getuid();
			if (u && u != pwd->pw_uid) {
				fprintf(stderr, "passwd: permission denied.\n");
				unlink(temp);
				exit(1);
			}
			pwd->pw_passwd = pw;
			if (pwd->pw_gecos[0] == '*')
				pwd->pw_gecos++;
		}
		if (fprintf(tf,"%s:%s:%d:%d:%s:%s:%s\n",
			pwd->pw_name,
			pwd->pw_passwd,
			pwd->pw_uid,
			pwd->pw_gid,
			pwd->pw_gecos,
			pwd->pw_dir,
			pwd->pw_shell) == EOF) {
				fprintf(stderr, "passwd: "); 
				perror("write");
				unlink(temp);
				exit(1);
		}
	}
	if (fflush(tf) == EOF) {
		fprintf(stderr, "passwd: "); 
		perror("write");
		unlink(temp);
		exit(1);
	}
	_endpwent();
	if (getuid() && (0 != strncmp(__PASSWD_, "/etc/passwd", 11)) &&
	    (-1 == access(__PASSWD_, W_OK))) {
		perror("access");
		unlink(temp);
		exit(1);
	}
	if (rename(temp, __PASSWD_) < 0) {
		fprintf(stderr, "passwd: "); perror("rename");
		unlink(temp);
		exit(1);
	}
	fclose(tf);
	exit(0);
}

usage()
{
	fprintf(stderr, "Usage: passwd [-f file] [user]\n");
}
