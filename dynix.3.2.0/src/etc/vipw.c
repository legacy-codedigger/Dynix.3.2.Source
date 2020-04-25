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
static char rcsid[] = "$Header: vipw.c 2.2 1991/07/18 18:54:02 $";
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <stdio.h>
#include <errno.h>
#include <signal.h>

/*
 * Password file editor with locking.
 */
char	*temp = "/etc/ptmp";
char	*passwd = "/etc/passwd";
char	buf[BUFSIZ];
char	*getenv();
char	*index();
extern	int errno;

main(argc, argv)
	char *argv[];
{
	int fd;
	register int  n, fd_passwd, fd_temp;
	FILE *ft;
	struct stat s1, s2;
	int ok, badsh;
	char *editor;

	ok = 0; 
	badsh = 0;
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	setbuf(stderr, (char *)NULL);
	(void)umask(0);
	if ((fd_passwd = open(passwd, O_RDONLY, 0)) < 0) {
		fputs("vipw: ", stderr);
		perror(passwd);
		exit(1);
	}
	fd_temp = open(temp, O_WRONLY|O_CREAT|O_EXCL, 0644);
	if (fd_temp < 0) {
		if (errno == EEXIST) {
			fprintf(stderr, "vipw: password file busy\n");
			exit(1);
		}
		fprintf(stderr, "vipw: "); perror(temp);
		exit(1);
	}
	while ((n = read(fd_passwd, buf, sizeof(buf))) > 0) {
		if (write(fd_temp, buf, n) != n) {
			perror("vipw: write");
			goto bad;
		}
	}
	if (n == -1) {
		perror("vipw: read");
		goto bad;
	}
	(void)close(fd_passwd);
	if (fsync(fd_temp)) {
		perror("vipw: fsync");
		goto bad;
	}
	if (fstat(fd_temp, &s1)) {
		perror("vipw: fstat");
		goto bad;
	}
	(void)close(fd_temp);
	editor = getenv("EDITOR");
	if (editor == 0)
		editor = "vi";
	sprintf(buf, "%s %s", editor, temp);
	if (system(buf)) {
		perror("vipw: system");
		goto bad;
	}

	ft = fopen(temp, "r");
	if (ft == NULL) {
		fprintf(stderr,
		    "vipw: can't reopen temp file, %s unchanged\n",
		    passwd);
		goto bad;
	}

	/* sanity checks */
	if (fstat(fileno(ft), &s2) < 0) {
		fprintf(stderr,
		    "vipw: can't stat temp file, %s unchanged\n",
		    passwd);
		goto bad;
	}

	if (s1.st_mtime == s2.st_mtime) {
		fprintf(stderr, "vipw: %s unchanged.\n", passwd);
		goto bad;
	}
	if (!s2.st_size) {
		fprintf(stderr, "vipw: bad temp file, %s unchanged\n",
		    passwd);
		goto bad;
	}

	while (fgets(buf, sizeof (buf) - 1, ft) != NULL) {
		register char *cp;

		cp = index(buf, '\n');
		if (cp == 0)
			continue;
		*cp = '\0';
		cp = index(buf, ':');
		if (cp == 0)
			continue;
		*cp = '\0';
		if (strcmp(buf, "root"))
			continue;
		/* password */
		cp = index(cp + 1, ':');
		if (cp == 0)
			break;
		/* uid */
		if (atoi(cp + 1) != 0)
			break;
		cp = index(cp + 1, ':');
		if (cp == 0)
			break;
		/* gid */
		cp = index(cp + 1, ':');
		if (cp == 0)
			break;
		/* gecos */
		cp = index(cp + 1, ':');
		if (cp == 0)
			break;
		/* login directory */
		if (strncmp(++cp, "/:", 2))
			break;
		cp += 2;
		if (*cp && 
		    strcmp(cp, "/bin/sh") &&
		    strcmp(cp, "/bin/csh") &&
		    strcmp(cp, "/bin/ksh")) {
			badsh++;
			break;
		}
		ok++;
	}
	fclose(ft);
	if (ok) {
		if (rename(temp, passwd) < 0)
			fprintf(stderr, "vipw: "), perror("rename");
		else
			exit(0);
	} else {
		if (badsh)
			fprintf(stderr,
			    "vipw: Bad root shell, %s unchanged\n",
			     passwd);
		else
			fprintf(stderr,
			    "vipw: Corrupt root passwd info, %s unchanged\n",
			     passwd);
	}
bad:
	(void)unlink(temp);
	exit(1);
}
