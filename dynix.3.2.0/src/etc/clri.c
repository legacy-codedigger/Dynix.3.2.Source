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
static char rcsid[] = "$Header: clri.c 2.2 86/12/22 $";
#endif

/*
 * clri filsys inumber ...
 */

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>

#define ISIZE	(sizeof(struct dinode))
#define	NI	(MAXBSIZE/ISIZE)

struct dinode *ibp;
struct fs  *sblock;

int	status;

main(argc, argv)
	int argc;
	char *argv[];
{
	register i, f;
	unsigned n;
	int j, k;
	long off;
	char *valloc();
	long gen;

	if (argc < 3) {
		printf("usage: clri filsys inumber ...\n");
		exit(4);
	}
	f = open(argv[1], 2);
	if (f < 0) {
		printf("cannot open %s\n", argv[1]);
		exit(4);
	}
	/*
	 * Allocate memory to hold superblock.
	 */
	sblock = (struct fs *)valloc(SBSIZE);
	if (sblock == (struct fs *)NULL) {
		printf("Cannot valloc sblock.\n");
		exit(4);
	}
	lseek(f, SBLOCK * DEV_BSIZE, 0);
	if (read(f, (char *)sblock, SBSIZE) != SBSIZE) {
		printf("cannot read %s\n", argv[1]);
		exit(4);
	}
	if (sblock->fs_magic != FS_MAGIC) {
		printf("bad super block magic number\n");
		exit(4);
	}
	/*
	 * Allocate memory to ino data buffer.
	 */
	ibp = (struct dinode *)valloc(NI * sizeof(struct dinode));
	if (ibp == NULL) {
		printf("Cannot valloc ino data buffer.\n");
		exit(4);
	}
	for (i = 2; i < argc; i++) {
		if (!isnumber(argv[i])) {
			printf("%s: is not a number\n", argv[i]);
			status = 1;
			continue;
		}
		n = atoi(argv[i]);
		if (n == 0) {
			printf("%s: is zero\n", argv[i]);
			status = 1;
			continue;
		}
		off = fsbtodb(sblock, itod(sblock, n)) * DEV_BSIZE;
		lseek(f, off, 0);
		if (read(f, (char *)ibp, sblock->fs_bsize) != sblock->fs_bsize) {
			printf("%s: read error\n", argv[i]);
			status = 1;
		}
	}
	if (status)
		exit(status);
	for (i = 2; i < argc; i++) {
		n = atoi(argv[i]);
		printf("clearing %u\n", n);
		off = fsbtodb(sblock, itod(sblock, n)) * DEV_BSIZE;
		lseek(f, off, 0);
		read(f, (char *)ibp, sblock->fs_bsize);
		j = itoo(sblock, n);
		gen = ibp[j].di_gen;
		bzero((caddr_t)&ibp[j], ISIZE);
		ibp[j].di_gen = gen + 1;
		lseek(f, off, 0);
		write(f, (char *)ibp, sblock->fs_bsize);
	}
	exit(status);
}

isnumber(s)
	char *s;
{
	register c;

	while(c = *s++)
		if (c < '0' || c > '9')
			return(0);
	return(1);
}
