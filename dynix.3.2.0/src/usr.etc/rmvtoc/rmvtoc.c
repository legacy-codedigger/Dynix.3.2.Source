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

#ident	"$Header: rmvtoc.c 1.5 90/12/13 $

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/vtoc.h>
#include <sys/param.h>
#include <zdc/zdc.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <mntent.h>

extern char *rindex();

static int tryopen();
static void rmvtoc();
static char *find_mount();
#define ZDBUG
#ifdef ZDBUG
char	*Alt_name;
#endif

#define isdigit(c) (((c) >= '0') && ((c) <= '9'))
#define ispart(c) (((c) >= 'a') && ((c) <= 'Z'))

main(argc, argv)
	int argc;
	char **argv;
{
	int fd;
	char *pt;

	if (argc != 2) {
		fprintf(stderr, "Usage is: %s <device>\n", argv[0]);
		exit(1);
	}
	fd = tryopen(argv+1);
	if (fd < 0)
		perror(argv[1]), exit(1);

	/*
	 * Protect against hosing an active disk.
	 */
	if (pt = find_mount(argv[1])) {
		fprintf(stderr, "%s busy: file system mounted on %s\n",
			argv[1], pt);
		exit(1);
	}

	rmvtoc(fd);
	printf("VTOC on %s removed.\n", argv[1]);
	return(0);
}

/*
 * tryopen()
 *
 * Open a disk device, even if only part of the name is given.
 * If a full pathname is given (ie, "/dev/...") then we trust that
 * this is what was intended.  Otherwise, if we fail to open the
 * literal name, start trying various canonical device names looking
 * for a match.
 */
static int
tryopen(name)
	char **name;
{
	int fd;
	char *p, *buf, c;
	extern char *malloc(), *rindex();
	int	len;

	/* Build a file name from it */
	if ((buf = malloc(128)) == 0)
		return(-1);
	if ((Alt_name = malloc(128)) == 0)
		return(-1);
	if ((p = rindex(*name, '/')) == 0)
		p = *name;
	else
		p++;
	if (*p == 'r')
		++p;
	(void) sprintf(buf, "/dev/r%s", p);
	len = strlen(buf);

	/* Trim partition from name if it's present */
	c = buf[len-1];
	if (ispart(c))
		buf[len-1] = '\0';

#ifdef ZDBUG
	strcpy(Alt_name,buf);
#endif
	/* First try name as-is */
	if ((fd = open(*name, O_RDWR)) >= 0)
		return(fd);

	*name = buf;

	if ((fd = open(buf, O_RDWR)) < 0)
		return(-1);
	return(fd);
}

/*
 * Get rid of a VTOC by calculating its byte offset and writing
 * zeroes there.
 */
static void
rmvtoc(fd)
	int fd;
{
	int err, len = sizeof(struct vtoc);
	unsigned long offset, block = V_VTOCSEC;
	struct vtoc *vt;
	extern char *valloc();

	/* Allocate an aligned buffer (for zd's) */
	/* Make its length an integral number of DEV_BSIZE units */
	len = (sizeof(struct vtoc)+DEV_BSIZE-1) & ~(DEV_BSIZE-1);
	if ((vt = (struct vtoc *)valloc(len)) == NULL)
		perror("rmvtoc"), exit(1);

	/* Verify that a VTOC exists */
	if (ioctl(fd, V_READ, vt) < 0) {
		fprintf(stderr, "Disk does not appear to contain a VTOC.\n");
		exit(1);
	}

	/* Zero it out */
	bzero(vt, len);

	/* Calculate block offset to VTOC */
	err = ioctl(fd, RIOFIRSTSECT, &offset);
	if (err < 0) {
		offset = 0;
	}
	block += offset;

	/* Position there */
	if (lseek(fd, (long)(block * DEV_BSIZE), L_SET) < (long)0) {
		(void) fprintf(stderr,
		     "mkvtoc: Cannot seek disk to block %d\n", block);
		perror("");
		exit(1);
	}

	/* Write it with zeroes */
	if (write(fd, vt, (unsigned)len) != len) {
		(void) fprintf(stderr,
		     "rmvtoc: Cannot write over disk block %ld\n", block);
		perror("");
		exit(1);
	}
	/*
	 * flush incore copy of VTOC
	 */
	if (ioctl(fd, V_READ, vt) != -1) {
		fprintf(stderr, "Disk appears to still contain a VTOC.\n");
		exit(1);
	}
	close(fd);
#ifdef ZDBUG
	strcat(Alt_name, "a");
	close(open(Alt_name, O_RDWR));
#endif
}

/*
 * find_mount()--query the mount table and see if this device has any
 * mounted partitions currently
 */
static char *
find_mount(dev)
	char *dev;
{
	register char *p;
	char buf[128], buf2[128];
	int len;
	register struct mntent *mntp;
	FILE *fp;

	/*
	 * Turn raw device name into block one
	 */
	if ((p = rindex(dev, '/')) == NULL)
		return(NULL);
	++p;
	if (*p == 'r')
		++p;
	sprintf(buf, "/dev/%s", p);
	len = strlen(buf);

	/*
	 * Walk through mount table, see if we match up with any entry
	 */
	if ((fp = setmntent(MOUNTED, "r")) == NULL)
		return(NULL);
	while (mntp = getmntent(fp)) {
		if (!strcmp(mntp->mnt_type, MNTTYPE_IGNORE))
			continue;

		/*
		 * Find the last digit of the unit number, and truncate
		 * name there, so we can just compare the basic device
		 * name for a match
		 */
		strncpy(buf2, mntp->mnt_fsname, sizeof(buf2)-1);
		buf2[sizeof(buf2)-1] = '\0';
		p = buf2;
		p += (strlen(buf2) - 1);
		while (!isdigit(*p) && (p != buf2))
			--p;

		/*
		 * No digit -> some kind of junk, so ignore it
		 */
		if (!isdigit(*p))
			continue;

		/*
		 * Chop off the trailing partition name
		 */
		++p;
		*p = '\0';

		/*
		 * If it matches, return the whole name
		 */
		if (!strcmp(buf2, buf))
			return (mntp->mnt_fsname);
	}
	return(NULL);
}
