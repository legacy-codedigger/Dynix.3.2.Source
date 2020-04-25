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

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /*not lint*/

/*
 * #ifndef lint
 * static char sccsid[] = "@(#)main.c	5.8 (Berkeley) 5/3/88";
 * #endif not lint
 */

#ident "$Header: main.c 1.3 90/04/10 $"

/* $Log:	main.c,v $
 */

#include <sys/types.h>
#include <sys/param.h>
#include <ufs/inode.h>
#include <ufs/fs.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <strings.h>
#include <signal.h>
#include <stdio.h>
#include <mntent.h>
#include <fcntl.h>
#define KERNEL
#include <sys/dir.h>
#undef KERNEL
#include <fstab.h>
#include "fsck.h"

char	*rawname();
extern int	catch(), catchquit(), voidquit();
int	returntosingle;
char	*devname;
#ifndef STANDALONE
extern char *malloc();
#endif
extern char *strtok();
char *blockcheck(), *unrawname();
void checkfilesys();
extern int	errno;


#ifdef	STANDALONE

time_t 	time(){};	/* stub */
int	sync(){};	/* stub */
#undef 	getchar

/*
 * Fake stat routine for standalone mode
 * Catch stat of "/" and other calls.
 */
stat(s, sp)
	register caddr_t s;
	register struct stat *sp;
{
	register caddr_t p;

	/* make it look like a character device (raw) */
	sp->st_mode = S_IFCHR;
	if (strcmp(s, "/") == 0) {
		sp->st_dev = 0;
		return (0);
	}
	/* name is of the form ``xx(unit,offset)'' */
	sp->st_rdev = atoi(index(s, ',')+1);
	return (0);
}

/*
 * Allocate memory on a page-aligned address.
 * Round allocated chunk to a page multiple to
 * ease next request.  Zero memory before returning.
 */

char *
mycalloc(nelem, size)
	int nelem, size;
{
	extern caddr_t calloc();

	callocrnd(DEV_BSIZE);
	return(calloc(((nelem * size) + (DEV_BSIZE-1)) & ~(DEV_BSIZE-1)));
}

/*
 * This does nothing. Very small risk of running out of memory if allocate
 * mucho memory per file-system checked.
 */
free(n)
{
}

#endif	STANDALONE

main(argc, argv)
	int	argc;
	char	*argv[];
{
	struct fstab *fsp;
	int pid, passno, anygtr, sumstatus;
	char *name;
	struct worklist {
		int	pid;		/* pid of child doing the check */
		struct	worklist *next;	/* next in list */
		char	name[MAXMNTLEN];/* name of file system */
	} *listhead = 0, *freelist = 0, *badlist = 0;
	register struct worklist *wp, *pwp;

	sync();
#ifndef STANDALONE
	while (--argc > 0 && **++argv == '-') {
		switch (*++*argv) {

		case 'p':
			preen++;
			break;
		
		case 'P':
			Preen++;
			preen++;
			break;

		case 'b':
			if (argv[0][1] != '\0') {
				bflag = atoi(argv[0]+1);
			} else {
				bflag = atoi(*++argv);
				argc--;
			}
			printf("Alternate super block location: %d\n", bflag);
			break;

		case 'c':
			cvtflag++;
			break;

		case 'd':
			debug++;
			break;

		case 'n':	/* default no answer flag */
		case 'N':
			nflag++;
			yflag = 0;
			break;

		case 'y':	/* default yes answer flag */
		case 'Y':
			yflag++;
			nflag = 0;
			break;

		default:
			errexit("%c option?\n", **argv);
		}
	}
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		(void)signal(SIGINT, catch);
	if (preen)
		(void)signal(SIGQUIT, catchquit);
	if (argc) {
		while (argc-- > 0) {
			hotroot = 0;
			checkfilesys(*argv++);
		}
		exit(0);
	}
	if (Preen) {
		newpreen();
		/*NOTREACHED*/
	}
	sumstatus = 0;
	passno = 1;
	do {
		anygtr = 0;
		if (setfsent() == 0)
			errexit("Can't open checklist file: %s\n", FSTAB);
		while ((fsp = getfsent()) != 0) {
			if (strcmp(fsp->fs_type, FSTAB_RW) &&
			    strcmp(fsp->fs_type, FSTAB_RO) &&
			    strcmp(fsp->fs_type, FSTAB_RQ))
				continue;
			if (preen == 0 ||
			    passno == 1 && fsp->fs_passno == passno) {
				name = blockcheck(fsp->fs_spec);
				if (name != NULL)
					checkfilesys(name);
				else if (preen)
					exit(8);
			} else if (fsp->fs_passno > passno) {
				anygtr = 1;
			} else if (fsp->fs_passno == passno) {
				name = blockcheck(fsp->fs_spec);
				if (name == NULL) {
					pwarn("BAD DISK NAME %s\n",
						fsp->fs_spec);
					sumstatus |= 8;
					continue;
				}
				pid = fork();
				if (pid < 0) {
					perror("fork");
					exit(8);
				}
				if (pid == 0) {
					(void)signal(SIGQUIT, voidquit);
					checkfilesys(name);
					exit(0);
				} else {
					if (freelist == 0) {
						wp = (struct worklist *) malloc
						    (sizeof(struct worklist));
					} else {
						wp = freelist;
						freelist = wp->next;
					}
					wp->next = listhead;
					listhead = wp;
					wp->pid = pid;
					(void)sprintf(wp->name, "%s (%s)", name,
					    fsp->fs_file);
				}
			}
		}
		if (preen) {
			union wait status;
			while ((pid = wait(&status)) != -1) {
				sumstatus |= status.w_retcode;
				pwp = 0;
				for (wp = listhead; wp; pwp = wp, wp = wp->next)
					if (wp->pid == pid)
						break;
				if (wp == 0) {
					printf("Unknown pid %d\n", pid);
					continue;
				}
				if (pwp == 0)
					listhead = wp->next;
				else
					pwp->next = wp->next;
				if (status.w_retcode != 0) {
					wp->next = badlist;
					badlist = wp;
				} else {
					wp->next = freelist;
					freelist = wp;
				}
			}
		}
		passno++;
	} while (anygtr);
	if (sumstatus) {
		if (badlist == 0)
			exit(8);
		printf("THE FOLLOWING FILE SYSTEM%s HAD AN %s\n\t",
			badlist->next ? "S" : "", "UNEXPECTED INCONSISTENCY:");
		for (wp = badlist; wp; wp = wp->next)
			printf("%s%s", wp->name, wp->next ? ", " : "\n");
		exit(8);
	}
	(void)endfsent();
	if (returntosingle)
		exit(2);
#else
	for(;;) {
		char cmdbuf[100];
		(void) printf("filesystem: ");
		(void) gets(cmdbuf);
		if (strcmp(cmdbuf, "q") == 0)
			break;
		(void) checkfilesys(cmdbuf);
	}
#endif	/* STANDALONE */
	exit(0);
}

#define setstate(fsp, state)	if ((fsp)->fs_postblformat != FS_42POSTBLFMT) \
					(fsp)->fs_state = (state);
void
checkfilesys(filesys)
	char *filesys;
{
	daddr_t n_ffree, n_bfree;
	struct dups *dp;
	struct zlncnt *zlnp;
	char rebflg;		/* needs reboot if set */
	char fixstate;		/* is FS_STATE to be fixed? */

	devname = filesys;
	hotroot = 0;
	switch (setup(filesys)) {
	case 0:		/* Can't access file system */
		if (preen)
			pfatal("CAN'T CHECK FILE SYSTEM.");
		return;
	case 1:		/* Must do full fsck */
		break;
	case 2:		/* PTXFS file sys already clean */
		printf("** File system on %s already clean\n", filesys);
		return;
	default:
		errexit("Bad status from setup()\n");
	}

	/*
	 * 1: scan inodes tallying blocks used
	 */
	if (preen == 0) {
		printf("** Last Mounted on %s\n", sblock.fs_fsmnt);
		if (hotroot)
			printf("** Root file system\n");
		printf("** Phase 1 - Check Blocks and Sizes\n");
	}
	pass1();

	/*
	 * 1b: locate first references to duplicates, if any
	 */
	if (duplist) {
		if (preen)
			pfatal("INTERNAL ERROR: dups with -p");
		printf("** Phase 1b - Rescan For More DUPS\n");
		pass1b();
	}

	/*
	 * 2: traverse directories from root to mark all connected directories
	 */
	if (preen == 0)
		printf("** Phase 2 - Check Pathnames\n");
	pass2();

	/*
	 * 3: scan inodes looking for disconnected directories
	 */
	if (preen == 0)
		printf("** Phase 3 - Check Connectivity\n");
	pass3();

	/*
	 * 4: scan inodes looking for disconnected files; check reference counts
	 */
	if (preen == 0)
		printf("** Phase 4 - Check Reference Counts\n");
	pass4();

	/*
	 * 5: check and repair resource counts in cylinder groups
	 */
	if (preen == 0)
		printf("** Phase 5 - Check Cyl groups\n");
	pass5();

	/*
	 * print out summary statistics
	 */
	n_ffree = sblock.fs_cstotal.cs_nffree;
	n_bfree = sblock.fs_cstotal.cs_nbfree;
	pwarn("%d files, %d used, %d free ",
	    n_files, n_blks, n_ffree + sblock.fs_frag * n_bfree);
	printf("(%d frags, %d blocks, %d.%d%% fragmentation)\n",
	    n_ffree, n_bfree, 
		((n_ffree * 100) / sblock.fs_dsize),
		(((n_ffree * 1000) + (sblock.fs_dsize/2)) / sblock.fs_dsize)
								% 10 );
	if (debug && (n_files -= imax - ROOTINO - sblock.fs_cstotal.cs_nifree))
		printf("%d files missing\n", n_files);
	if (debug) {
		n_blks += sblock.fs_ncg *
			(cgdmin(&sblock, 0) - cgsblock(&sblock, 0));
		n_blks += cgsblock(&sblock, 0) - cgbase(&sblock, 0);
		n_blks += howmany(sblock.fs_cssize, sblock.fs_fsize);
		if (n_blks -= fmax - (n_ffree + sblock.fs_frag * n_bfree))
			printf("%d blocks missing\n", n_blks);
		if (duplist != NULL) {
			printf("The following duplicate blocks remain:");
			for (dp = duplist; dp; dp = dp->next)
				printf(" %d,", dp->dup);
			printf("\n");
		}
		if (zlnhead != NULL) {
			printf("The following zero link count inodes remain:");
			for (zlnp = zlnhead; zlnp; zlnp = zlnp->next)
				printf(" %d,", zlnp->zlncnt);
			printf("\n");
		}
	}
	zlnhead = (struct zlncnt *)0;
	duplist = (struct dups *)0;
	ckflush();
	if (PTXFS(&sblock)) {
		if (hotroot && sblock.fs_state == FS_ACTIVE)
			fixstate = 1;
		else if ((sblock.fs_state + (long)sblock.fs_time) != FS_OKAY
		     || dfile.mod) {
			if (preen) {
				fixstate = 1;
			} else if (dfile.mod || rplyflag) {
				if (reply("SET FILE SYSTEM STATE TO OKAY") == 1)
					fixstate = 1;
				else
					fixstate = 0;
			} else if (nflag) {
				printf("%s FILE SYSTEM STATE NOT SET TO OKAY\n",devname);
				fixstate = 0;
			} else {
				printf("%s FILE SYSTEM STATE SET TO OKAY\n",devname);
				fixstate = 1;
			}
		}
	}
	rebflg = dfile.mod;
	if (fixstate) {
		(void)time(&sblock.fs_time);
		if (hotroot) {
			if (xxflag || rebflg) {
				setstate(&sblock,
					FS_OKAY - (long)sblock.fs_time);
			 } else {
				setstate(&sblock, FS_ACTIVE);
			}
		} else {
			setstate(&sblock, FS_OKAY - (long)sblock.fs_time);
		}
		sbdirty();
	}

	ckfini();
	free((char *)blockmap);
	free((char *)statemap);
	free((char *)lncntp);
	if (!rebflg)
		return;
	if (hotroot) {
		printf("\n%s ***** ROOT FILE SYSTEM WAS MODIFIED *****\n",
			devname);
#ifdef STANDALONE
		sync();
#else
		if (xxflag) {
			printf("  ***** SYSTEM RETURNING TO FIRMWARE *****\n");
			printf("  ***** will REBOOT if AUTO enabled *****\n");
			(void)fflush(stdout);

			/* Let print & sync finish */
			(void)sleep((unsigned)2);

			(void)offline_all();
			reboot(RB_NOSYNC|RB_BOOT);
		}
#endif
		printf("%s ***** REBOOT DYNIX *****\n",devname);
		exit(4);
	}
	printf("%s ***** FILE SYSTEM WAS MODIFIED *****\n",devname);
	return;
}

char *
blockcheck(name)
	char *name;
{
	struct stat stslash, stblock, stchar;
	char *raw;
	int looped = 0;

	hotroot = 0;
	if (stat("/", &stslash) < 0){
#ifdef STANDALONE
		printf("/ errno=%d\n", errno);
#else
		perror("/");
#endif
		printf("Can't stat root\n");
		return (0);
	}
retry:
	if (stat(name, &stblock) < 0){
#ifdef STANDALONE
		printf("%s errno=%d\n", name, errno);
#else
		perror(name);
#endif
		printf("Can't stat %s\n", name);
		return (0);
	}
	if ((stblock.st_mode & S_IFMT) == S_IFBLK) {
		if (stslash.st_dev == stblock.st_rdev) {
			hotroot++;
			return (name);
		}
		raw = rawname(name);
		if (stat(raw, &stchar) < 0){
#ifdef STANDALONE
			printf("%s errno=%d\n", raw, errno);
#else
			perror(raw);
#endif
			printf("Can't stat %s\n", raw);
			return (name);
		}
		if ((stchar.st_mode & S_IFMT) == S_IFCHR)
			return (raw);
		else {
			printf("%s is not a character device\n", raw);
			return (name);
		}
	} else if ((stblock.st_mode & S_IFMT) == S_IFCHR) {
		if (looped) {
			printf("Can't make sense out of name %s\n", name);
			return (0);
		}
		name = unrawname(name);
		looped++;
		goto retry;
	}
	printf("Can't make sense out of name %s\n", name);
	return (0);
}

/*
 * Convert a block device name to the equivalent raw name, by prepending
 * an "r" after "/dev/".  The new name is stuck into static storage.
 * If "/dev/" is not present - just return the name as is.
 *
 * Note that this routine will (correctly) not convert "/dev/root".
 * Examples of conversion might be:
 *	/dev/zd0a	yields	/dev/rzd0a
 *	/dev/dsk/zd0s0	yields	/dev/rdsk/zd0s0
 */

char *
rawname(name)
	register char	*name; 
{
	register char	*p;
	static char	newnm[MAXPATHLEN];	/* don't trash 'name' */

	if (strncmp(name, "/dev/", 5)) {
		return(name);
	}
	p = name + 5;
	if (*p == 'r')
		return(name);
#ifdef STANDALONE
	(void)strcpy(newnm, "/dev/r");
	(void)strcat(newnm, p);
#else
	(void)sprintf(newnm, "/dev/r%s", p);
#endif
	return(newnm);
}

char *
unrawname(cp)
	char *cp;
{
	char *dp = rindex(cp, '/');
	struct stat stb;

	if (dp == 0)
		return (cp);
	if (stat(cp, &stb) < 0)
		return (cp);
	if ((stb.st_mode&S_IFMT) != S_IFCHR)
		return (cp);
	if (*(dp+1) != 'r')
		return (cp);
	(void)strcpy(dp+1, dp+2);
	return (cp);
}
