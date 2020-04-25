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
static char rcsid[] = "$Header: man.c 2.9 87/09/28 $";
#endif

#include <stdio.h>
#include <ctype.h>
#include <sgtty.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
/*
 * man
 * link also to apropos and whatis
 * This version uses more for underlining and paging.
 */
#define	MANDIR	"/usr/man"		/* this is deeply embedded */
#define WHATIS	"/usr/lib/whatis"
#define	MANALIASES "/usr/man/aliases"	/* for aliasing man pages */
#define	NROFFCAT "nroff -man"		/* for nroffing to cat file */
#define	NROFF	"nroff -man"		/* for nroffing to tty */
#define	MORE	"/usr/ucb/more -s"	/* paging filter */
#define	CAT_	"/bin/cat"		/* for when output is not a tty */
#define	CAT_S	"/bin/cat -s"		/* for '-' opt (no more) */
#define	COMPRESS "/usr/ucb/compress -q -f"	/* for compressing pages */
#define	UNCOMPRESS "/usr/ucb/zcat"		/* for uncompressing pages */

#define TROFFCMD \
"troff -t -man /usr/lib/tmac/tmac.vcat %s | /usr/lib/rvsort |/usr/ucb/vpr -t"

/*  for troff:
#define	TROFFCMD "troff -man %s"
*/

#define	ALLSECT	"1nl6823457po"	/* order to look through sections */
#define	SECT1	"1nlo"		/* sections to look at if 1 is specified */
#define	SUBSEC1	"mcgprvs"	/* subsections to try in section 1 */
#define	SUBSEC2	"v"
#define	SUBSEC3	"jxmsnvcfrp"
#define	SUBSEC4	"pfvsn"
#define	SUBSEC8	"vcs"

int	nomore;
int	cflag;
char	*CAT	= CAT_;
char	*manpath = "/usr/man";
char	*pager;
char	*troffcmd;
char	*alias;
char	*getenv();
char	*strcpy();
char	*strcat();
char	*index();
char	*trim();
int	remove();
int	section;
int	subsec;
int	troffit;
int	killtmp;

#define	eq(a,b)	(strcmp(a,b) == 0)

main(argc, argv)
	int argc;
	char *argv[];
{

	if (signal(SIGINT, SIG_IGN) == SIG_DFL) {
		signal(SIGINT, remove);
		signal(SIGQUIT, remove);
		signal(SIGTERM, remove);
	}
	umask(0);
	if (strcmp(argv[0], "apropos") == 0) {
		apropos(argc-1, argv+1);
		exit(0);
	}
	if (strcmp(argv[0], "whatis") == 0) {
		whatis(argc-1, argv+1);
		exit(0);
	}
	argc--, argv++;
	while (argc > 0 && argv[0][0] == '-') {
		switch(argv[0][1]) {

		case 0:
			nomore++;
			CAT = CAT_S;
			break;

		case 't':
			troffit++;
			if ((troffcmd = getenv("MANTROFF")) == NULL)
				troffcmd = TROFFCMD;
			break;

		case 'k':
			apropos(argc-1, argv+1);
			exit(0);

		case 'f':
			whatis(argc-1, argv+1);
			exit(0);

		case 'P':
			argc--, argv++;
			manpath = *argv;
			break;

		default:
			goto usage;
		}
		argc--, argv++;
	}
	if (argc <= 0) {
usage:
		fprintf(stderr,
		    "Usage:  man [-t] [-P path] [-] [ section ] name ...\n");
		fprintf(stderr, "\tman -k keyword ...\n\tman -f file ...\n");
		exit(1);
	}
	if (chdir(manpath) < 0) {
		fprintf(stderr, "Can't chdir to %s.\n", manpath);
		exit(1);
	}
	if ((pager = getenv("PAGER")) == NULL)
		pager = MORE;
	if (troffit == 0 && nomore == 0 && !isatty(1))
		nomore++;
	section = 0;
	do {
		if (eq(argv[0], "local") || eq(argv[0], "l")) {
			section = 'l';
			goto sectin;
		} else if (eq(argv[0], "new") || eq(argv[0], "n")) {
			section = 'n';
			goto sectin;
		} else if (eq(argv[0], "old") || eq(argv[0], "o")) {
			section = 'o';
			goto sectin;
		} else if (eq(argv[0], "public") || eq(argv[0], "p")) {
			section = 'p';
			goto sectin;
		} else if (isdigit(argv[0][0]) &&
		    (argv[0][1] == 0 || argv[0][2] == 0)) {
			section = argv[0][0];
			subsec = argv[0][1];
			if (isupper(subsec))
				subsec = tolower(subsec);
sectin:
			argc--, argv++;
			if (argc == 0) {
				fprintf(stderr, "But what do you want from section %s?\n", argv[-1]);
				exit(1);
			}
			continue;
		}
		if (search_alias(section, argv[0]))
			manual(section, alias);
		else
			manual(section, argv[0]);
		argc--, argv++;
	} while (argc > 0);
	exit(0);
}

manual(sec, name)
	char sec;
	char *name;
{
	char section = sec;
	char work[100], work2[100], work3[100], cmdbuf[150];
	int ss;
	struct stat stbuf, stbuf2;
	int last;
	char *sp = ALLSECT;

	strcpy(work, "manx/");
	strcat(work, name);
	strcat(work, ".x");
	last = strlen(work) - 1;
	if (section == '1') {
		sp = SECT1;
		section = 0;
	}
	if (section == 0) {
		ss = 0;
		for (section = *sp++; section; section = *sp++) {
			work[3] = section;
			work[last] = section;
			work[last+1] = 0;
			work[last+2] = 0;
			strcpy(work2, work);
			work2[0] = 'c'; work2[2] = 't';
			strcpy(work3, work2);
			strcat(work3, ".Z");
			if (stat(work, &stbuf) >= 0 || stat(work2, &stbuf) >= 0 || stat(work3, &stbuf) >= 0)
				break;
			if (work[last] >= '1' && work[last] <= '8') {
				char *cp;
search:
				switch (work[last]) {
				case '1': cp = SUBSEC1; break;
				case '2': cp = SUBSEC2; break;
				case '3': cp = SUBSEC3; break;
				case '4': cp = SUBSEC4; break;
				case '8': cp = SUBSEC8; break;
				}
				while (*cp) {
					work[last+1] = *cp++;
					strcpy(work2, work);
					work2[0] = 'c'; work2[2] = 't';
					strcpy(work3, work2);
					strcat(work3, ".Z");
					if (stat(work, &stbuf) >= 0 || stat(work2, &stbuf) >= 0 || stat(work3, &stbuf) >= 0) {
						ss = work[last+1];
						goto found;
					}
				}
				if (ss == 0)
					work2[last+1] = work[last+1] = 0;
			}
		}
		if (section == 0) {
			if (sec == 0)
				printf("No manual entry for %s.\n", name);
			else
				printf("No entry for %s in section %c of the manual.\n", name, sec);
			return;
		}
	} else {
		work[3] = section;
		work[last] = section;
		work[last+1] = subsec;
		work[last+2] = 0;
		strcpy(work2, work);
		work2[0] = 'c'; work2[2] = 't';
		strcpy(work3, work2);
		strcat(work3, ".Z");
		if (stat(work, &stbuf) < 0 && stat(work2, &stbuf) < 0 && stat(work3, &stbuf) < 0) {
			if ((section >= '1' && section <= '8') && subsec == 0) {
				sp = "\0";
				ss = 0;
				goto search;
			}
			printf("No entry for %s in section %c", name, section);
			if (subsec)
				putchar(subsec);
			printf(" of the manual.\n");
			return;
		}
	}
found:
	if (troffit)
		troff(work);
	else {
		FILE *it;
		char abuf[BUFSIZ];

		if (!nomore) {
			if ((it = fopen(work, "r")) != NULL &&
			   fgets(abuf, BUFSIZ-1, it) &&
			   abuf[0] == '.' && abuf[1] == 's' &&
			   abuf[2] == 'o' && abuf[3] == ' ') {
				register char *cp = abuf+strlen(".so ");
				char *dp;

				while (*cp && *cp != '\n')
					cp++;
				*cp = 0;
				while (cp > abuf && *--cp != '/')
					;
				dp = ".so man";
				if (cp != abuf+strlen(dp)+1) {
tohard:
					nomore = 1;
					strcpy(work, abuf+4);
					fclose(it);
					goto hardway;
				}
				for (cp = abuf; *cp == *dp && *cp; cp++, dp++)
					;
				if (*dp)
					goto tohard;
				strcpy(work, cp-3);
			}
			if (it != NULL) 
				fclose(it);
			strcpy(work2, "cat");
			strcpy(work2+3, work+3);
			work2[4] = 0;
			if (stat(work2, &stbuf2) < 0)	/* cat? directory missing */
				goto hardway;
			strcpy(work2+3, work+3);
			strcpy(work3, work2);
			strcat(work3, ".Z");
			if ((stat(work2, &stbuf2) < 0 || stbuf2.st_mtime < stbuf.st_mtime) &&
			    (stat(work3, &stbuf2) < 0 || stbuf2.st_mtime < stbuf.st_mtime)) {
				printf("Reformatting page.  Wait...");
				fflush(stdout);
				unlink(work2);
				unlink(work3);
				sprintf(cmdbuf,
			"%s %s > /tmp/man%d; trap '' 1 2 3 15; mv /tmp/man%d %s",
				    NROFFCAT, work, getpid(), getpid(), work2);
				if (system(cmdbuf)) {
					printf(" aborted (sorry)\n");
					remove();
					/*NOTREACHED*/
				}
compress:
				printf("Compressing page. Wait...");
				fflush(stdout);
				sprintf(cmdbuf, "%s %s", COMPRESS, work2);
				if (system(cmdbuf)) {
					printf(" aborted (sorry)\n");
					remove();
					/*NOTREACHED*/
				}
				printf(" done\n");
			}
			if (stat(work2, &stbuf2) >= 0 && stat(work3, &stbuf2) < 0)
				goto compress;
			strcpy(work, stat(work3, &stbuf2) >= 0 ? work3 : work2);
		}
hardway:
		nroff(work);
	}
}

nroff(cp)
	char *cp;
{
	register int n;
	char cmd[BUFSIZ];
	struct stat stbuf;

tryagain:
	n = strlen(cp);
	if (cp[0] == 'c') {
		if (n > 2 && cp[n-2] == '.' && cp[n-1] == 'Z')
			sprintf(cmd, nomore? "%s %s" : "%s %s|%s", UNCOMPRESS, cp, pager);
		else
			sprintf(cmd, "%s %s", nomore? CAT : pager, cp);
	} else 	{
		if (stat(cp, &stbuf) >= 0) {
			sprintf(cmd, nomore? "%s %s" : "%s %s|%s", NROFF, cp, pager);
		} else {
			cp[0] = 'c'; cp[2] = 't';
			if (stat(cp, &stbuf) >= 0)
				goto tryagain;
			strcat(cp, ".Z");
			if (stat(cp, &stbuf) >= 0)
				goto tryagain;
			fprintf(stderr, "more: sorry, can't find your file\n");
			return;
		}
	}
	system(cmd);
}

troff(cp)
	char *cp;
{
	char cmdbuf[BUFSIZ];

	sprintf(cmdbuf, troffcmd, cp);
	system(cmdbuf);
}

any(c, sp)
	register int c;
	register char *sp;
{
	register int d;

	while (d = *sp++)
		if (c == d)
			return (1);
	return (0);
}

remove()
{
	char name[15];

	sprintf(name, "/tmp/man%d", getpid());
	unlink(name);
	exit(1);
}

apropos(argc, argv)
	int argc;
	char **argv;
{
	char buf[BUFSIZ];
	char *gotit;
	register char **vp;

	if (argc == 0) {
		fprintf(stderr, "apropos what?\n");
		exit(1);
	}
	if (freopen(WHATIS, "r", stdin) == NULL) {
		perror(WHATIS);
		exit (1);
	}
	gotit = (char *) calloc(1, blklen(argv));
	while (fgets(buf, sizeof buf, stdin) != NULL)
		for (vp = argv; *vp; vp++)
			if (match(buf, *vp)) {
				printf("%s", buf);
				gotit[vp - argv] = 1;
				for (vp++; *vp; vp++)
					if (match(buf, *vp))
						gotit[vp - argv] = 1;
				break;
			}
	for (vp = argv; *vp; vp++)
		if (gotit[vp - argv] == 0)
			printf("%s: nothing appropriate\n", *vp);
}

match(buf, str)
	char *buf, *str;
{
	register char *bp;

	bp = buf;
	for (;;) {
		if (*bp == 0)
			return (0);
		if (amatch(bp, str))
			return (1);
		bp++;
	}
}

amatch(cp, dp)
	register char *cp, *dp;
{

	while (*cp && *dp && lmatch(*cp, *dp))
		cp++, dp++;
	if (*dp == 0)
		return (1);
	return (0);
}

lmatch(c, d)
	char c, d;
{

	if (c == d)
		return (1);
	if (!isalpha(c) || !isalpha(d))
		return (0);
	if (islower(c))
		c = toupper(c);
	if (islower(d))
		d = toupper(d);
	return (c == d);
}

blklen(ip)
	register int *ip;
{
	register int i = 0;

	while (*ip++)
		i++;
	return (i);
}

whatis(argc, argv)
	int argc;
	char **argv;
{
	register char **avp;

	if (argc == 0) {
		fprintf(stderr, "whatis what?\n");
		exit(1);
	}
	if (freopen(WHATIS, "r", stdin) == NULL) {
		perror(WHATIS);
		exit (1);
	}
	for (avp = argv; *avp; avp++)
		*avp = trim(*avp);
	whatisit(argv);
	exit(0);
}

whatisit(argv)
	char **argv;
{
	char buf[BUFSIZ];
	register char *gotit;
	register char **vp;

	gotit = (char *)calloc(1, blklen(argv));
	while (fgets(buf, sizeof buf, stdin) != NULL)
		for (vp = argv; *vp; vp++)
			if (wmatch(buf, *vp)) {
				printf("%s", buf);
				gotit[vp - argv] = 1;
				for (vp++; *vp; vp++)
					if (wmatch(buf, *vp))
						gotit[vp - argv] = 1;
				break;
			}
	for (vp = argv; *vp; vp++)
		if (gotit[vp - argv] == 0)
			printf("%s: not found\n", *vp);
}

wmatch(buf, str)
	char *buf, *str;
{
	register char *bp, *cp;

	bp = buf;
again:
	cp = str;
	while (*bp && *cp && lmatch(*bp, *cp))
		bp++, cp++;
	if (*cp == 0 && (*bp == '(' || *bp == ',' || *bp == '\t' || *bp == ' '))
		return (1);
	while (isalpha(*bp) || isdigit(*bp))
		bp++;
	if (*bp != ',')
		return (0);
	bp++;
	while (isspace(*bp))
		bp++;
	goto again;
}

char *
trim(cp)
	register char *cp;
{
	register char *dp;

	for (dp = cp; *dp; dp++)
		if (*dp == '/')
			cp = dp + 1;
	if (cp[0] != '.') {
		if (cp + 3 <= dp && dp[-2] == '.' && any(dp[-1], "cosa12345678npP"))
			dp[-2] = 0;
		if (cp + 4 <= dp && dp[-3] == '.' && any(dp[-2], "13") && isalpha(dp[-1]))
			dp[-3] = 0;
	}
	return (cp);
}

system(s)
char *s;
{
	int status, pid, w;
	int (*saveintr)(), (*savequit)();

	if ((pid = vfork()) == 0) {
		execl("/bin/sh", "sh", "-c", s, 0);
		_exit(127);
	}

	/* 
	 * Block these signals while waiting
	 */
	saveintr = signal (SIGINT, SIG_IGN);
	savequit = signal (SIGQUIT, SIG_IGN);

	while ((w = wait(&status)) != pid && w != -1)
		;
	/*
	 * Restore signal state before returning
	 */
	signal (SIGINT, saveintr);
	signal (SIGQUIT, savequit);

	if (w == -1)
		status = -1;
	return (status);
}

/*
 * search_alias
 *
 * Search for an alias for a manual page
 * Returns 1 for successful alias, 0 otherwise.
 */

search_alias(sec, name)
	char	sec;
	char	*name;
{
	register int n;
	struct  alias_t {
		char 	*name, sec, subsec;
		char	*a_name, a_sec, a_subsec;
	};
	static	struct alias_t *alias_list = NULL;
	register struct	alias_t *ap;
	FILE *f;
	char *filebytes;
	struct	stat sbuf;

	if (alias_list == NULL) {
		register char *cp, *end;
		if ((f = fopen(MANALIASES, "r")) == NULL
		  || fstat(fileno(f), &sbuf) != 0
		  || (filebytes = (char *)calloc(1, sbuf.st_size + 1)) == NULL
		  || fread(filebytes, sbuf.st_size, 1, f) != 1 
		  || (alias_list = (struct alias_t *)calloc(sbuf.st_size / 5, sizeof(struct alias_t))) == NULL)
			return 0;
		(void) fclose(f);
		end = &filebytes[sbuf.st_size];
		*end = '\n';
		cp = filebytes; 
		n = 0;
		ap = alias_list;
		while ( cp < end ) {
			/* comment lines start with '#' */
			if ( *cp == '#' ) {
				cp = index(cp, '\n');
				if (cp == NULL)
					break;
				cp++;
				continue;
			}
			while ( (*cp == ' ' || *cp == '\t')  && *cp != '\n')
				++cp;
			if ( *cp == '\n' ) {
				++cp;
				continue;
			}
			ap[n].name = cp++;
			while (*cp != ' ' && *cp != '\t' && *cp != '\n')
				++cp;
			if (*cp == '\n') {
bad: 				ap[--n].name = (char *)NULL;
				ap[n].sec = ap[n].subsec = '\0';
				++cp;
				continue;
			}
			*cp = '\0';
			if (cp[-2] == '.') {
				ap[n].sec = cp[-1];
				cp[-2] = '\0';
			} else if (cp[-3] == '.') {
				ap[n].sec = cp[-2];
				ap[n].subsec = cp[-1];
				cp[-3] = '\0';
			}
			++cp;
			while ( (*cp == ' ' || *cp == '\t')  && *cp != '\n')
				++cp;
			if (*cp == '\n')
				goto bad;
			ap[n].a_name = cp;
			cp = index(cp, '\n');
			if (cp == NULL) {
				ap[n].a_name = NULL;
				goto bad;
				break;
			}
			*cp = '\0';
			if (cp[-2] == '.') {
				ap[n].a_sec = cp[-1];
				cp[-2] = '\0';
			} else if (cp[-3] == '.') {
				ap[n].a_sec = cp[-2];
				ap[n].a_subsec = cp[-1];
				cp[-3] = '\0';
			}
			++n;
			++cp;
		}
	}
	for (ap = alias_list; ap->name; ap++) {
		if (strcmp(name, ap->name) == 0) { /* found a name match */
			/* section does not a match */
			if (sec && sec != ap->sec)
				continue;
			/* subsection does not a match */
			if (subsec && subsec != ap->subsec)
				continue;
			/* match, so do alias */
			if (ap->a_sec) {
				section = ap->a_sec;
				subsec = ap->a_subsec;
			}
			alias = ap->a_name;
			return 1;
		}
	}
	return 0;
}
