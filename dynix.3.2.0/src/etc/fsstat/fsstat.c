/*
 * $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 *
 */

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Header: fsstat.c 1.1 90/01/23 $@(#)fsstat:fsstat.c	1.2"

#include <stdio.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/types.h>
#include <ufs/fs.h>
#include <sys/stat.h>
#include <mntent.h>

static int mounted_fs();

/* Length of contents of a string array */
#define strsize(str) (sizeof(str)-1)

/*
 * exit 0 - file system is unmounted and okay
 * exit 1 - file system is unmounted and needs checking
 * exit 2 - file system is mounted
 *          for root file system
 * exit 0 - okay
 * exit 1 - needs checking
 *
 * exit 3 - unexpected failures
 */
main(argc, argv)
char *argv[];
{
	register dev;
	register char *fp;
	struct fs *fb;
	struct stat stbd, stbr;

	if (argc != 2) {
		fprintf(stderr, "usage: fsstat special\n");
		exit(3);
	}
	fp = argv[1];
	if ((dev = open(fp, O_RDONLY)) < 0) {
		fprintf(stderr, "fsstat: cannot open %s\n", fp);
		exit(3);
	}
	fstat(dev, &stbd);
	if ((stbd.st_mode&S_IFMT) != S_IFBLK) {
		fprintf(stderr, "fsstat: %s not a block device\n", fp);
		exit(3);
	}
	stat("/", &stbr);
	if ((fb = (struct fs *)valloc(SBSIZE)) == NULL) {
		fprintf(stderr, "fsstat: out of memory\n");
		exit(3);
	}
	lseek(dev, (long)dbtob(SBLOCK), 0);
	if (read(dev, fb, SBSIZE) != SBSIZE) {
		fprintf(stderr, "fsstat: cannot read %s\n", fp);
		exit(3);
	}
	if (!PTXFS(fb)) {
		fprintf(stderr, "fsstat: %s is pre-DYNIX 3.1 file system.\n", fp);
		exit(1);
	}
	if (stbr.st_dev == stbd.st_rdev) {	/* root file system */
		if (fb->fs_state != FS_ACTIVE) {
			fprintf(stderr, "fsstat: root file system needs checking\n");
			exit(1);
		} else {
			fprintf(stderr, "fsstat: root file system okay\n");
			exit(0);
		}
	}
	if (mounted_fs(stbd.st_rdev, stbd.st_mode & S_IFMT )) {
		fprintf(stderr, "fsstat: %s mounted\n", fp);
		exit(2);
	}
	if (fb->fs_magic != FS_MAGIC) {
		fprintf(stderr, "fsstat: %s not a valid file system\n", fp);
		exit(3);
	}
	if ((fb->fs_state + (long)fb->fs_time) != FS_OKAY) {
		fprintf(stderr, "fsstat: %s needs checking\n", fp);
		exit(1);
	}
	fprintf(stderr, "fsstat: %s okay\n", fp);
	exit(0);
}

/*
 * mounted_fs()
 *
 * Answer whether or not the specified device has a file system
 * mounted on it.
 */
static
mounted_fs(rdev, fmt)
	dev_t rdev;
	unsigned fmt;
{
	char		*item;
	struct stat	sb;
	char		devbuf[40];
	static char	devblk[] = "/dev/";
	static char	devraw[] = "/dev/r";
	struct	 mntent *mntp;
	FILE		*mnttabfd;
	int		wantraw;

	/* read mnttab for partition mountpoints */
	if ((mnttabfd = setmntent(MOUNTED, "r")) == NULL) {
		perror(MOUNTED);
		return(0);
	}

	/* Tell whether we're looking for char or block dev */
	wantraw = (fmt == S_IFCHR);

	/* Scan each entry looking for a match */
	while ((mntp = getmntent(mnttabfd)) != NULL) {
		if (strcmp(mntp->mnt_type, MNTTYPE_IGNORE) == 0) {
			continue;
		}
		item = mntp->mnt_fsname;
		if (strncmp(item, devblk, strsize(devblk)))
			continue;
		if (wantraw) {
			strcpy(devbuf, devraw);
			strcat(devbuf, item+strsize(devblk));
		} else
			strcpy(devbuf, item);
		if (stat(devbuf, &sb) < 0)
			continue;
		if ((sb.st_mode & S_IFMT) != fmt)
			continue;
		if (sb.st_rdev == rdev) {
			endmntent(mnttabfd);
			return(1);
		}
	}
	endmntent(mnttabfd);
	return(0);
}
