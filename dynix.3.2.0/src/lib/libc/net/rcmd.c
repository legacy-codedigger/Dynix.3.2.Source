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
static char rcsid[] = "$Header: rcmd.c 2.9 90/07/17 $";
#endif

/*
 * Copyright (c) 1983 Regents of the University of California.
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

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)rcmd.c	5.20 (Berkeley) 1/24/89";
#endif /* LIBC_SCCS and not lint */

#include <stdio.h>
#include <ctype.h>
#include <pwd.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <netinet/in.h>

#include <netdb.h>
#include <errno.h>

extern	errno;
char	*index();

rcmd(ahost, rport, locuser, remuser, cmd, fd2p)
	char **ahost;
	u_short rport;
	char *locuser, *remuser, *cmd;
	int *fd2p;
{
	int s, timo = 1, pid;
	long oldmask;
	struct sockaddr_in sin, sin2, from;
	char c;
	int lport = IPPORT_RESERVED - 1;
	struct hostent *hp;
	fd_set reads;

	pid = getpid();
	hp = gethostbyname(*ahost);
	if (hp == 0) {
		herror(*ahost);
		return (-1);
	}
	*ahost = hp->h_name;
	oldmask = sigblock(sigmask(SIGURG));
	for (;;) {
		s = rresvport(&lport);
		if (s < 0) {
			if (errno == EAGAIN)
				fprintf(stderr, "socket: All ports in use\n");
			else
				perror("rcmd: socket");
			sigsetmask(oldmask);
			return (-1);
		}
		fcntl(s, F_SETOWN, pid);
		sin.sin_family = hp->h_addrtype;
		bcopy(hp->h_addr_list[0], (caddr_t)&sin.sin_addr, hp->h_length);
		sin.sin_port = rport;
		if (connect(s, (caddr_t)&sin, sizeof (sin), 0) >= 0)
			break;
		(void) close(s);
		if (errno == EADDRINUSE) {
			lport--;
			continue;
		}
		if (errno == ECONNREFUSED && timo <= 16) {
			sleep(timo);
			timo *= 2;
			continue;
		}
		if (hp->h_addr_list[1] != NULL) {
			int oerrno = errno;

			fprintf(stderr,
			    "connect to address %s: ", inet_ntoa(sin.sin_addr));
			errno = oerrno;
			perror(0);
			hp->h_addr_list++;
			bcopy(hp->h_addr_list[0], (caddr_t)&sin.sin_addr,
			    hp->h_length);
			fprintf(stderr, "Trying %s...\n",
				inet_ntoa(sin.sin_addr));
			continue;
		}
		perror(hp->h_name);
		sigsetmask(oldmask);
		return (-1);
	}
	lport--;
	if (fd2p == 0) {
		write(s, "", 1);
		lport = 0;
	} else {
		char num[8];
		int s2 = rresvport(&lport), s3;
		int len = sizeof (from);

		if (s2 < 0)
			goto bad;
		listen(s2, 1);
		(void) sprintf(num, "%d", lport);
		if (write(s, num, strlen(num)+1) != strlen(num)+1) {
			perror("write: setting up stderr");
			(void) close(s2);
			goto bad;
		}
		FD_ZERO(&reads);
		FD_SET(s, &reads);
		FD_SET(s2, &reads);
		errno = 0;
		if (select(32, &reads, 0, 0, 0) < 1 ||
		    !FD_ISSET(s2, &reads)) {
			if (errno != 0)
				perror("select: setting up stderr");
			else
			    fprintf(stderr,
				"select: protocol failure in circuit setup.\n");
			(void) close(s2);
			goto bad;
		}
		s3 = accept(s2, &from, &len, 0);
		(void) close(s2);
		if (s3 < 0) {
			perror("accept");
			lport = 0;
			goto bad;
		}
		*fd2p = s3;
		from.sin_port = ntohs((u_short)from.sin_port);
		if (from.sin_family != AF_INET ||
		    from.sin_port >= IPPORT_RESERVED ||
		    from.sin_port < IPPORT_RESERVED / 2) {
			fprintf(stderr,
			    "socket: protocol failure in circuit setup.\n");
			goto bad2;
		}
	}
	(void) write(s, locuser, strlen(locuser)+1);
	(void) write(s, remuser, strlen(remuser)+1);
	(void) write(s, cmd, strlen(cmd)+1);
	if (read(s, &c, 1) != 1) {
		perror(*ahost);
		goto bad2;
	}
	if (c != 0) {
		while (read(s, &c, 1) == 1) {
			(void) write(2, &c, 1);
			if (c == '\n')
				break;
		}
		goto bad2;
	}
	sigsetmask(oldmask);
	return (s);
bad2:
	if (lport)
		(void) close(*fd2p);
bad:
	(void) close(s);
	sigsetmask(oldmask);
	return (-1);
}

rresvport(alport)
	int *alport;
{
	struct sockaddr_in sin;
	int s;

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		return (-1);
	for (;;) {
		sin.sin_port = htons((u_short)*alport);
		if (bind(s, (caddr_t)&sin, sizeof (sin)) >= 0)
			return (s);
		if (errno != EADDRINUSE) {
			(void) close(s);
			return (-1);
		}
		(*alport)--;
		if (*alport == IPPORT_RESERVED/2) {
			(void) close(s);
			errno = EAGAIN;		/* close */
			return (-1);
		}
	}
}

int	_check_rhosts_file = 1;

ruserok(rhost, superuser, ruser, luser)
	char *rhost;
	int superuser;
	char *ruser, *luser;
{
	FILE *hostf;
	char fhost[MAXHOSTNAMELEN];
	int first = 1;
	register char *sp, *p;
	int baselen = -1;
	char domain[256];
	struct stat sbuf;
	struct passwd *pwd;
	char pbuf[MAXPATHLEN];
	int euid = -1;

	if (getdomainname(domain, sizeof(domain)) < 0) {
		fprintf(stderr, "rcmd: getdomainname system call missing\n");
		exit(1);
	}

	sp = rhost;
	p = fhost;
	while (*sp) {
		if (*sp == '.') {
			if (baselen == -1)
				baselen = sp - rhost;
			*p++ = *sp++;
		} else {
			*p++ = isupper(*sp) ? tolower(*sp++) : *sp++;
		}
	}
	*p = '\0';
	hostf = superuser ? (FILE *)0 : fopen("/etc/hosts.equiv", "r");

	if (hostf) {
		if (!_validuser(hostf, fhost, luser, ruser, baselen, domain)) {
			(void) fclose(hostf);
			return(0);
		}
		(void) fclose(hostf);
	}

	/*
	 * in checking .rhosts, do so as local user to avoid NFS
	 * mapping this user to someone who can't read the
	 * .rhosts file (e.g. nobody)
	 */

	if ((pwd = getpwnam(luser)) == NULL)
		return(-1);
	(void)strcpy(pbuf, pwd->pw_dir);
	(void)strcat(pbuf, "/.rhosts");

	euid = geteuid();
	(void) seteuid(pwd->pw_uid);
	if ((hostf = fopen(pbuf, "r")) == NULL) {
		if (euid != -1)
			(void) seteuid(euid);
		return(-1);
	}
	/*
	 * if owned by someone other than user or root or if
	 * writeable by anyone but the owner, quit
	 */
	if (fstat(fileno(hostf), &sbuf) ||
	    sbuf.st_uid && sbuf.st_uid != pwd->pw_uid ||
	    sbuf.st_mode&022) {
		fclose(hostf);
		if (euid != -1)
			(void) seteuid(euid);
		return(-1);
	}
	if (!_validuser(hostf, fhost, luser, ruser, baselen, domain)) {
		(void) fclose(hostf);
		if (euid != -1)
			(void) seteuid(euid);
		return(0);
	}
	(void) fclose(hostf);
	if (euid != -1)
		(void) seteuid(euid);

	return (-1);
}


_validuser(hostf, rhost, luser, ruser, baselen, domain)
	char *rhost, *luser, *ruser;
	FILE *hostf;
	int baselen;
	char *domain;
{
	char *user;
	char ahost[MAXHOSTNAMELEN];
	register char *p;
	int hostmatch, usermatch;

	while (fgets(ahost, sizeof (ahost), hostf)) {
		p = ahost;
		while (*p != '\n' && *p != ' ' && *p != '\t' && *p != '\0') {
			*p = isupper(*p) ? tolower(*p) : *p;
			p++;
		}
		if (*p == ' ' || *p == '\t') {
			*p++ = '\0';
			while (*p == ' ' || *p == '\t')
				p++;
			user = p;
			while (*p != '\n' && *p != ' ' && *p != '\t' && *p != '\0')
				p++;
		} else
			user = p;
		*p = '\0';

        /*
         *  + - anything goes
         *  +@<name> - group <name> allowed
         *  -@<name> - group <name> disallowed
         *  -<name> - host <name> disallowed
         */
		if (ahost[0] == '+' && ahost[1] == 0)
			hostmatch = 1;
		else if (ahost[0] == '+' && ahost[1] == '@')
			hostmatch = innetgr(ahost + 2, rhost,
			    NULL, domain);
		else if (ahost[0] == '-' && ahost[1] == '@') {
			if (innetgr(ahost + 2, rhost, NULL, domain))
				break;
		}
		else if (ahost[0] == '-') {
			if (_checkhost(rhost, ahost+1, baselen))
				break;
		}
		else
			hostmatch = _checkhost(rhost, ahost, baselen);
		if (user[0]) {
			if (user[0] == '+' && user[1] == 0)
				usermatch = 1;
			else if (user[0] == '+' && user[1] == '@')
				usermatch = innetgr(user+2, NULL,
				    ruser, domain);
			else if (user[0] == '-' && user[1] == '@') {
				if (hostmatch && innetgr(user+2, NULL,
				    ruser, domain))
					break;
			}
			else if (user[0] == '-') {
				if (hostmatch && !strcmp(user+1, ruser))
					break;
			}
			else
				usermatch = !strcmp(user, ruser);
		}
		else
			usermatch = !strcmp(ruser, luser);
		if (hostmatch && usermatch)
			return (0);
	}
	return (-1);
}

static
_checkhost(rhost, lhost, len)
	char *rhost, *lhost;
	int len;
{
	static char ldomain[MAXHOSTNAMELEN + 1];
	static char *domainp = NULL;
	static int nodomain = 0;
	register char *cp;

	if (len == -1)
		return(!strcmp(rhost, lhost));
	if (strncmp(rhost, lhost, len))
		return(0);
	if (!strcmp(rhost, lhost))
		return(1);
	if (*(lhost + len) != '\0')
		return(0);
	if (nodomain)
		return(0);
	if (!domainp) {
		if (gethostname(ldomain, sizeof(ldomain)) == -1) {
			nodomain = 1;
			return(0);
		}
		ldomain[MAXHOSTNAMELEN] = NULL;
		if ((domainp = index(ldomain, '.')) == (char *)NULL) {
			nodomain = 1;
			return(0);
		}
		for (cp = ++domainp; *cp; ++cp)
			if (isupper(*cp))
				*cp = tolower(*cp);
	}
	return(!strcmp(domainp, rhost + len +1));
}