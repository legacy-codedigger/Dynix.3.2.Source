/* $Copyright:	$
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

#ifndef	lint
static char rcsid[] = "$Header: df.c 2.9 91/04/09 $";
#endif

/* NFSSRC @(#)df.c	2.1 86/04/17 */
#ifndef lint
static	char sccsid[] = "@(#)df.c 1.1 86/02/03 SMI"; /* from UCB 4.18 84/02/02 */
#endif
/*
 * df
 */
#include <sys/param.h>
#include <errno.h>
#include <ufs/fs.h>
#include <sys/stat.h>
#include <sys/vfs.h>

#include <stdio.h>
#include <mntent.h>

char	*mpath();
int	iflag;
int	type;
char	*typestr;
int	errors;

struct mntent *getmntpt(), *mntdup();
struct fs *sblock;
char *valloc();
int	tty;

main(argc, argv)
	int argc;
	char **argv;
{
	int i;
	struct stat statb;
	char tmpname[1024];

	tty = isatty(1);
	while (argc > 1 && argv[1][0]=='-') {
		switch (argv[1][1]) {

		case 'i':
			iflag++;
			break;

		case 't':
			type++;
			typestr = argv[2];
			argv++;
			argc--;
			break;

		default:
			usage();
		}
		argc--, argv++;
	}
	if (argc > 1 && type) {
		usage();
	}

	/*
	 * Allocate page aligned memory to hold superblock.
	 */
	sblock = (struct fs *)valloc(SBSIZE);
	if (sblock == (struct fs *)NULL) {
		printf("df: cannot valloc sblock.\n");
		exit(1);
	}

	sync();
	printf("Filesystem            kbytes    used   avail capacity");
	if (iflag) {
		printf(" iused  ifree %%iused");
		if (tty)
			printf("\n\t\t\t\t");
	}
	printf(" Mounted on\n");
	if (argc <= 1) {
		FILE *mtabp;
		struct mntent *mnt;

		if ((mtabp = setmntent(MOUNTED, "r")) == 0) {
			perror(MOUNTED);
			exit(1);
		}
		while (mnt = getmntent(mtabp)) {
			if (strcmp(mnt->mnt_type, MNTTYPE_IGNORE) == 0 ||
			    strcmp(mnt->mnt_type, MNTTYPE_SWAP) == 0)
				continue;
			if (type && strcmp(typestr, mnt->mnt_type)) {
				continue;
			}
			if ((stat(mnt->mnt_fsname, &statb) >= 0) &&
			    (strcmp (mnt->mnt_type, MNTTYPE_42) == 0) &&
			   (((statb.st_mode & S_IFBLK) == S_IFBLK) ||
			    ((statb.st_mode & S_IFCHR) == S_IFCHR))) {
				strcpy(tmpname, mnt->mnt_fsname);
				dfreedev(tmpname);
			} else {
				dfreemnt(mnt->mnt_dir, mnt);
			}
		}
		endmntent(mtabp);
		exit(0);
	}
	for (i=1; i<argc; i++) {
		struct mntent *mnt;

		if (stat(argv[i], &statb) < 0) {
			perror(argv[i]);
			errors = 1;
		} else {
			if ((statb.st_mode & S_IFBLK) == S_IFBLK ||
			    (statb.st_mode & S_IFCHR) == S_IFCHR) {
				dfreedev(argv[i]);
			} else {
				if ((mnt = getmntpt(argv[i])) != NULL)
					if (type &&
					    strcmp(typestr, mnt->mnt_type)) {
						continue;
					}
					dfreemnt(argv[i], mnt);
			}
		}
	}
	exit(errors);
}

dfreedev(file)
	char *file;
{
	long totalblks, availblks, avail, free, used;
	int fi;

	fi = open(file, 0);
	if (fi < 0) {
		perror(file);
		errors = 1;
		return;
	}
	if (bread(fi, SBLOCK, (char *)sblock, SBSIZE) == 0) {
		(void) close(fi);
		errors = 1;
		return;
	}
	printf("%-20.20s", file);
	totalblks = sblock->fs_dsize;
	free = sblock->fs_cstotal.cs_nbfree * sblock->fs_frag +
	    sblock->fs_cstotal.cs_nffree;
	used = totalblks - free;
	availblks = totalblks * (100 - sblock->fs_minfree) / 100;
	avail = availblks > used ? availblks - used : 0;
	printf("%8u%8u%8u",
	    totalblks * sblock->fs_fsize / 1024,
	    used * sblock->fs_fsize / 1024,
	    avail * sblock->fs_fsize / 1024);
	printf("    %2d%%", ((used*100) + (availblks/2))/availblks);
	if (iflag) {
		int inodes = sblock->fs_ncg * sblock->fs_ipg;
		used = inodes - sblock->fs_cstotal.cs_nifree;
		printf("%8lu%7lu   %2d%%", used, sblock->fs_cstotal.cs_nifree,
		  	  ((used*100) + (inodes/2)) / inodes);
		if (tty)
			printf("\n\t\t\t\t");
	} else
		printf("  ");
	printf(" %s\n", mpath(file));
	(void) close(fi);
}

dfreemnt(file, mnt)
	char *file;
	struct mntent *mnt;
{
	struct statfs fs;
	long totalblks, avail, free, used, reserved;

	if (strlen(mnt->mnt_fsname) > 20) {
		printf("%s\n", mnt->mnt_fsname);
		printf("                    ");
	} else {
		printf("%-20.20s", mnt->mnt_fsname);
	}
	fflush(stdout);		/* show them what fs we're working on NOW */

	if (statfs(file, &fs) < 0) {
		perror(file);
		errors = 1;
		return;
	}

	totalblks = fs.f_blocks;
	free = fs.f_bfree;
	used = totalblks - free;
	avail = fs.f_bavail;
	reserved = free - avail;
	if (avail < 0)
		avail = 0;
	printf("%8u%8u%8u", totalblks * fs.f_bsize / 1024,
	    used * fs.f_bsize / 1024, avail * fs.f_bsize / 1024);
	totalblks -= reserved;
	printf("    %2d%%", ((used*100) + (totalblks/2))/totalblks);
	if (iflag) {
		long files;

		files = fs.f_files;
		used = files - fs.f_ffree;
		printf("%8ld%7ld    %2d%%", used, fs.f_ffree,
		  	  ((used*100) + (files/2)) / files);
		if (tty)
			printf("\n\t\t\t\t");
	} else {
		printf("  ");
	}
	printf(" %s\n", mnt->mnt_dir);
}

/*
 * Given a name like /usr/src/etc/foo.c returns the mntent
 * structure for the file system it lives in.
 */
struct mntent *
getmntpt(file)
	char *file;
{
	FILE *mntp;
	struct mntent *mnt, *mntsave;
	struct stat filestat, dirstat;

	if (stat(file, &filestat) < 0) {
		perror(file);
		errors = 1;
		return(NULL);
	}

	if ((mntp = setmntent(MOUNTED, "r")) == 0) {
		perror(MOUNTED);
		errors = 1;
		exit(1);
	}

	mntsave = NULL;
	while ((mnt = getmntent(mntp)) != 0) {
		if (strcmp(mnt->mnt_type, MNTTYPE_IGNORE) == 0 ||
		    strcmp(mnt->mnt_type, MNTTYPE_SWAP) == 0)
			continue;
		if ((stat(mnt->mnt_dir, &dirstat) >= 0) &&
		   (filestat.st_dev == dirstat.st_dev)) {
			mntsave = mntdup(mnt);
			break;
		}
	}
	endmntent(mntp);
	if (mntsave) {
		return(mntsave);
	} else {
		fprintf(stderr, "Couldn't find mount point for %s\n", file);
		exit(1);
	}
	/*NOTREACHED*/
}

/*
 * Given a name like /dev/rrp0h, returns the mounted path, like /usr.
 */
char *
mpath(file)
	char *file;
{
	FILE *mntp;
	register struct mntent *mnt;

	if ((mntp = setmntent(MOUNTED, "r")) == 0) {
		perror(MOUNTED);
		exit(1);
	}

	while ((mnt = getmntent(mntp)) != 0) {
		if (strcmp(file, mnt->mnt_fsname) == 0) {
			endmntent(mntp);
			return (mnt->mnt_dir);
		}
	}
	endmntent(mntp);
	return "";
}

long lseek();

bread(fi, bno, buf, cnt)
	int fi;
	daddr_t bno;
	char *buf;
{
	int n;
	extern errno;

	(void) lseek(fi, (long)(bno * DEV_BSIZE), 0);
	if ((n=read(fi, buf, cnt)) != cnt) {
		/* probably a dismounted disk if errno == EIO */
		if (errno != EIO) {
			printf("\nread error bno = %ld\n", bno);
			printf("count = %d; errno = %d\n", n, errno);
		}
		return (0);
	}
	return (1);
}

char *
xmalloc(size)
	int size;
{
	char *ret;
	
	if ((ret = (char *)malloc(size)) == NULL) {
		fprintf(stderr, "df: ran out of memory!\n");
		exit(1);
	}
	return (ret);
}

struct mntent *
mntdup(mnt)
	struct mntent *mnt;
{
	struct mntent *new;

	new = (struct mntent *)xmalloc(sizeof(*new));

	new->mnt_fsname = (char *)xmalloc(strlen(mnt->mnt_fsname) + 1);
	strcpy(new->mnt_fsname, mnt->mnt_fsname);

	new->mnt_dir = (char *)xmalloc(strlen(mnt->mnt_dir) + 1);
	strcpy(new->mnt_dir, mnt->mnt_dir);

	new->mnt_type = (char *)xmalloc(strlen(mnt->mnt_type) + 1);
	strcpy(new->mnt_type, mnt->mnt_type);

	new->mnt_opts = (char *)xmalloc(strlen(mnt->mnt_opts) + 1);
	strcpy(new->mnt_opts, mnt->mnt_opts);

	new->mnt_freq = mnt->mnt_freq;
	new->mnt_passno = mnt->mnt_passno;

	return (new);
}

usage()
{

	fprintf(stderr, "usage: df [ -i ] [-t type | file... ]\n");
	exit(1);
}
