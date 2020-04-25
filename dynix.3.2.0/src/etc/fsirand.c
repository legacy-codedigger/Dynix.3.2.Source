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

#ifndef	lint
static char rcsid[] = "$Header: fsirand.c 1.3 90/01/23 $";
#endif

#ifndef lint
static  char sccsid[] = "@(#)fsirand.c 1.1 85/05/30 Copyr 1984 Sun Micro";
#endif

/*
 * fsirand - Copyright (c) 1984 Sun Microsystems.
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <ufs/fs.h>
#include <sys/vnode.h>
#include <ufs/inode.h>

struct dinode *dibuf;
extern int errno;
extern time_t time();
extern long random();

#define FSID(fs) (PTXFS(fs) ? ((fs)->fs_id) : ((fs)->fs_un.fs_dsp.dfs_id))

main(argc, argv)
int	argc;
char	*argv[];
{
	struct fs *fs;
	int fd;
	char *dev;
	int bno;
	struct dinode *dip;
	int inum, imax;
	int n;
	int seekaddr, bsize;
	int pflag = 0;
	time_t timeval;
	char *valloc();

	argv++;
	argc--;
	if (argc > 0 && strcmp(*argv, "-p") == 0) {
		pflag++;
		argv++;
		argc--;
	}
	if (argc <= 0) {
		fprintf(stderr, "Usage: fsirand [-p] special\n");
		exit(1);
	}
	dev = *argv;
	fd = open(dev, pflag ? 0 : 2);
	if (fd == -1) {
		fprintf(stderr, "Cannot open %s\n", dev);
		exit(1);
	}
	if (lseek(fd, (off_t)(SBLOCK * DEV_BSIZE), 0) == -1) {
		fprintf(stderr, "Seek to superblock failed\n");
		exit(1);
	}

	dibuf = (struct dinode *) valloc(MAXBSIZE);
	if (dibuf == NULL) {
		fprintf(stderr, "Valloc of buffer failed %d\n", errno);
		exit(1);
	}
	fs = (struct fs *) valloc(SBSIZE);
	if (fs == NULL) {
		fprintf(stderr, "Valloc of buffer failed %d\n", errno);
		exit(1);
	}
	if (read(fd, (char *) fs, SBSIZE) != SBSIZE) {
		fprintf(stderr, "Read of superblock failed %d\n", errno);
		exit(1);
	}
	if (fs->fs_magic != FS_MAGIC) {
		fprintf(stderr, "Not a superblock\n");
		exit(1);
	}
	if (pflag) {
		printf("fsid: %x %x\n", FSID(fs)[0], FSID(fs)[1]);
	} else {
		n = getpid();
		(void)time(&timeval);
		srandom((unsigned)(timeval + n));
	}
	bsize = INOPB(fs) * sizeof (struct dinode);
	inum = 0;
	imax = fs->fs_ipg * fs->fs_ncg;
	while (inum < imax) {
		bno = itod(fs, inum);
		seekaddr = fsbtodb(fs, bno) * DEV_BSIZE;
		if (lseek(fd, seekaddr, 0) == -1) {
			fprintf(stderr, "lseek to %d failed\n", seekaddr);
			exit(1);
		}
		n = read(fd, (char *) dibuf, bsize);
		if (n != bsize) {
			printf("premature EOF\n");
			exit(1);
		}
		for (dip = dibuf; dip < &dibuf[INOPB(fs)]; dip++) {
			if (pflag) {
				printf("ino %d gen %x\n", inum, dip->di_gen);
			} else {
				dip->di_gen = random();
			}
			inum++;
		}
		if (!pflag) {
			if (lseek(fd, seekaddr, 0) == -1) {
				fprintf(stderr, "lseek to %d failed\n",
				    seekaddr);
				exit(1);
			}
			n = write(fd, (char *) dibuf, bsize);
			if (n != bsize) {
				printf("premature EOF\n");
				exit(1);
			}
		}
	}
	if (!pflag) {
		timeval = time((long *)0);
		FSID(fs)[0] = timeval;
		FSID(fs)[1] = timeval + getpid();
		if (lseek(fd, (off_t)(SBLOCK * DEV_BSIZE), 0) == -1) {
			fprintf(stderr, "Seek to superblock failed\n");
			exit(1);
		}
		if (write(fd, (char *) fs, SBSIZE) != SBSIZE) {
			fprintf(stderr, "Write of superblock failed %d\n",
			    errno);
			exit(1);
		}
	}
	for (n = 0; n < fs->fs_ncg; n++ ) {
		seekaddr = fsbtodb(fs, cgsblock(fs, n)) * DEV_BSIZE;
		if (lseek(fd,  seekaddr, 0) == -1) {
			fprintf(stderr, "Seek to alt superblock failed\n");
			exit(1);
		}
		if (pflag) {
			if (read(fd, (char *) fs, SBSIZE) != SBSIZE) {
				fprintf(stderr,
				    "Read of  alt superblock failed %d %d\n",
				    errno, seekaddr);
				exit(1);
			}
			if (fs->fs_magic != FS_MAGIC) {
				fprintf(stderr, "Not an alt superblock\n");
				exit(1);
			}
		} else {
			if (write(fd, (char *) fs, SBSIZE) != SBSIZE) {
				fprintf(stderr,
				    "Write of alt superblock failed\n");
				exit(1);
			}
		}
	}
	exit(0);
}
