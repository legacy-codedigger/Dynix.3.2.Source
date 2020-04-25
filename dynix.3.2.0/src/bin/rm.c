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
static char rcsid[] = "$Header: rm.c 2.2 90/12/21 $";
#endif

/*
 * rm - for ReMoving files, directories & trees.
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/file.h>

int	fflg;		/* -f force - supress error messages */
int	iflg;		/* -i interrogate user on each file */
int	rflg;		/* -r recurse */

int	errcode;	/* true if errors occured */

char	*strcpy(), *malloc();
DIR	*extend_fd_opendir();

main(argc, argv)
	int argc;
	char **argv;
{
	extern int optind;
	int ch;

	fflg = !isatty(0);
	while ((ch = getopt(argc, argv, "-Rfir")) != EOF)
		switch((char)ch) {
		case '-':
			goto endarg;
		case 'f':
			fflg++;
			break;
		case 'i':
			iflg++;
			break;
		case 'R':
		case 'r':
			rflg++;
			break;
		case '?':
		default:
			usage();
		}
endarg:	argv += optind;

	if (!*argv) {
		if (fflg)
			exit(0);
		usage();
	}
	do {
		(void)rm(*argv, 0);
	} while (*++argv);
	exit(errcode != 0);
}

char	*path;		/* pointer to malloc'ed buffer for path */
char	*pathp;		/* current pointer to end of path */
int	pathsz;		/* size of path */

#define	isdot(a)	(a[0] == '.' && (!a[1] || a[1] == '.' && !a[2]))
/*
 * Return TRUE if sucessful. Recursive with -r (rflg)
 */
rm(arg, level)
	char arg[];
{
	int ok;				/* true if recursive rm succeeded */
	struct stat buf;		/* for finding out what a file is */
	struct direct *dp;		/* for reading a directory */
	DIR *dirp;			/* for reading a directory */
	char prevname[MAXNAMLEN + 1];	/* previous name for -r */
	char *cp;

	if (isdot(arg)) {
		fprintf(stderr, "rm: cannot remove `.' or `..'\n");
		return (0);
	}
	if (lstat(arg, &buf)) {
		if (!fflg) {
			fprintf(stderr, "rm: %s nonexistent\n", arg);
			errcode++;
		}
		return (0);		/* error */
	}
	if ((buf.st_mode&S_IFMT) == S_IFDIR) {
		if (!rflg) {
			if (!fflg) {
				fprintf(stderr, "rm: %s directory\n", arg);
				errcode++;
			}
			return (0);
		}
		if (iflg && level != 0) {
			printf("rm: remove directory %s? ", arg);
			if (!yes())
				return (0);	/* didn't remove everything */
		}
		if (access(arg, R_OK|W_OK|X_OK) != 0) {
			if (rmdir(arg) == 0)
				return (1);	/* salvaged: removed empty dir */
			if (!fflg) {
				fprintf(stderr, "rm: %s not changed\n", arg);
				errcode++;
			}
			return (0);		/* error */
		}
		if ((dirp = extend_fd_opendir(arg)) == NULL) {
			if (!fflg) {
				fprintf(stderr, "rm: cannot read %s?\n", arg);
				errcode++;
			}
			return (0);
		}
		if (level == 0)
			append(arg);
		prevname[0] = '\0';
		while ((dp = readdir(dirp)) != NULL) {
			if (isdot(dp->d_name)) {
				strcpy(prevname, dp->d_name);
				continue;
			}
			append(dp->d_name);
			closedir(dirp);
			ok = rm(path, level + 1);
			for (cp = pathp; *--cp != '/' && cp > path; )
				;
			pathp = cp;
			*cp++ = '\0';
			if ((dirp = extend_fd_opendir(arg)) == NULL) {
				if (!fflg) {
					fprintf(stderr, "rm: cannot read %s?\n", arg);
					errcode++;
				}
				break;
			}
			/* pick up where we left off */
			if (prevname[0] != '\0') {
				while ((dp = readdir(dirp)) != NULL &&
				    strcmp(prevname, dp->d_name) != 0)
					;
			}
			/* skip the one we just failed to delete */
			if (!ok) {
				dp = readdir(dirp);
				if (dp != NULL && strcmp(cp, dp->d_name)) {
					fprintf(stderr,
			"rm: internal synchronization error: %s, %s, %s\n",
						arg, cp, dp->d_name);
				}
				strcpy(prevname, dp->d_name);
			}
		}
		closedir(dirp);
		if (level == 0) {
			pathp = path;
			*pathp = '\0';
		}
		if (iflg) {
			printf("rm: remove %s? ", arg);
			if (!yes())
				return (0);
		}
		if (rmdir(arg) < 0) {
			if (!fflg || iflg) {
				fprintf(stderr, "rm: %s not removed\n", arg);
				errcode++;
			}
			return (0);
		}
		return (1);
	}

	if (!fflg) {
		if ((buf.st_mode&S_IFMT) != S_IFLNK && access(arg, W_OK) < 0) {
			printf("rm: override protection %o for %s? ",
				buf.st_mode&0777, arg);
			if (!yes())
				return (0);
			goto rm;
		}
	}
	if (iflg) {
		printf("rm: remove %s? ", arg);
		if (!yes())
			return (0);
	}
rm:	if (unlink(arg) < 0) {
		if (!fflg || iflg) {
			fprintf(stderr, "rm: %s: ", arg);
			perror((char *)NULL);
			errcode++;
		}
		return (0);
	}
	return (1);
}

/*
 * Get a yes/no answer from the user.
 */
yes()
{
	int i, b;

	i = b = getchar();
	while (b != '\n' && b != EOF)
		b = getchar();
	return (i == 'y');
}

/*
 * Append 'name' to 'path'.
 */
append(name)
	char *name;
{
	register int n;

	n = strlen(name);
	if (path == NULL) {
		pathsz = MAXNAMLEN + MAXPATHLEN + 2;
		if ((path = malloc((u_int)pathsz)) == NULL) {
			fprintf(stderr, "rm: ran out of memory\n");
			exit(1);
		}
		pathp = path;
	} else if (pathp + n + 2 > path + pathsz) {
		fprintf(stderr, "rm: path name too long: %s\n", path);
		exit(1);
	} else if (pathp != path && pathp[-1] != '/')
		*pathp++ = '/';
	strcpy(pathp, name);
	pathp += n;
}

usage()
{
	fputs("usage: rm [-rif] file ...\n", stderr);
	exit(1);
}