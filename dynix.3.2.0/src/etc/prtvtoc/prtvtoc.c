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

#ident	"$Header: prtvtoc.c 1.7 1991/07/01 23:08:33 $"
/*
 * prtvtoc.c
 *
 * Print a disk partition map ("VTOC").
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <mntent.h>
#include <sys/stat.h>
#include <sys/vtoc.h>
#include <sys/param.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fstab.h>
#include <sys/dir.h>

#define ispart(c) (((c) >= 'a') && ((c) <= ('a'+NUMPARTS)))

#ifdef	lint
struct	streamtab {
	int	dummy;
};
#endif	/* lint */

/*
 * Macros.
 */
#define	strsize(str)	\
		(sizeof(str) - 1)	/* Length of static character array */
/*
 * Definitions.
 */
#define	reg	register		/* Convenience */

/*
 * Disk freespace structure.
 */
typedef struct {
	ulong	fr_start;		/* Start of free space */
	ulong	fr_size;		/* Length of free space */
} Freemap;

/*
 * External functions.
 */
void	exit();
void	perror();
char	*malloc();
char	*valloc();
void	qsort();
char	*strcat();
char	*strcpy();
char	*rindex();
char	*strdup();
int	getopt();
long	lseek();

/*
 * Internal functions.
 */
void	fatal();
Freemap	*findfree();
char	**getmntpt();
int	partcmp();
int	prtvtoc();
void	puttable();
char	*syserr();
void	usage();
int	warn();

/*
 * External variables.
 */
extern int	errno;			/* System error code */
extern char     *sys_errlist[];		/* Error messages */
extern int      sys_nerr;		/* Number of sys_errlist[] entries */
extern int	optind;			/* Argument index */
extern char	*optarg;		/* Option argument */

/*
 * Static variables.
 */
static short	hflag;			/* Omit headers */
static short	sflag;			/* Omit all but the column header */
static short	Tflag;			/* Print out just the disk type */
static char	*fstab = FSTAB;		/* Fstab pathname */
static char	*mnttab = MOUNTED;	/* mnttab pathname */
static char	*myname;		/* Last qualifier of arg0 */

main(ac, av)
int		ac;
reg char	**av;
{
	reg int		idx;

	if (myname = rindex(av[0], '/'))
		++myname;
	else
		myname = av[0];
	while ((idx = getopt(ac, av, "fhsTt:m:")) != -1) {
		switch (idx) {
		case 'h':
			++hflag;
			break;
		case 's':
			++sflag;
			break;
		case 'T':
			++Tflag;
		case 't':
			fstab = optarg;
			break;
		case 'm':
			mnttab = optarg;
			break;
		default:
			usage();
		}
	}
	if (optind >= ac)
		usage();
	idx = 0;
	while (optind < ac) {
		idx |= prtvtoc(av[optind]);
		optind++;
	}
	exit(idx);
	/* NOTREACHED */
}

/*
 * fatal()
 *
 * Print an error message and exit.
 */
static void
fatal(what, why)
reg char	*what;
reg char	*why;
{
	(void) fprintf(stderr, "%s: %s: %s\n", myname, what, why);
	exit(1);
}

/*
 * findfree()
 *
 * Find free space on a disk. Returns a pointer to the static result.
 */

static Freemap *
findfree(vtoc)
reg struct vtoc		*vtoc;
{
	reg struct partition	*part;
	reg struct partition	**list;
	reg Freemap		*freeidx;
	ulong			fullsize;
	struct partition	*sorted[V_NUMPAR + 1];
	static Freemap		freemap[V_NUMPAR + 1];

	fullsize = vtoc->v_capacity;
	if (vtoc->v_nparts > V_NUMPAR)
		fatal("putfree()", "Too many partitions on disk!");
	list = sorted;
	for (part = vtoc->v_part; part < vtoc->v_part + vtoc->v_nparts; ++part)
		*list++ = part;
	*list = 0;
	qsort((char *) sorted, (u_int) (list - sorted), sizeof(*sorted), partcmp);
	freeidx = freemap;
	freeidx->fr_start = 0;
	for (list = sorted; part = *list; ++list) {
		if (part->p_type == V_NOPART)
			continue;
		if (part->p_start == freeidx->fr_start) {
			freeidx->fr_start += part->p_size;
		} else {
			freeidx->fr_size = part->p_start - freeidx->fr_start;
			(++freeidx)->fr_start = part->p_start + part->p_size;
		}
	}
	if (freeidx->fr_start < fullsize) {
		freeidx->fr_size = fullsize - freeidx->fr_start;
		++freeidx;
	}
	freeidx->fr_start = freeidx->fr_size = 0;
	return (freemap);
}

/*
 * getmntpt()
 *
 * Get the filesystem mountpoint of each partition on the disk
 * from the fstab or mnttab . Returns a pointer to an array of pointers to
 * directory names (indexed by partition number).
 */
static char **
getmntpt(slot, nopartminor)
int		slot;
int		nopartminor;
{
	reg char	*item;
	reg int		idx;
	auto struct stat sb;
	auto char	devbuf[40];
	static char	devblk[] = "/dev/";
	static char	devraw[] = "/dev/r";
	static char	*list[V_NUMPAR];
	struct	 mntent *mntp;
	FILE	*mnttabfd, *fstabfd;

	for (idx = 0; idx < V_NUMPAR; ++idx)
		list[idx] = 0;

	/* read mnttab for partition mountpoints */

	if ((mnttabfd = setmntent(mnttab, "r")) == NULL) {
		(void) warn(mnttab, syserr());
	} else {
		while ((mntp = getmntent(mnttabfd)) != NULL) {
			if (strcmp(mntp->mnt_type, MNTTYPE_IGNORE) == 0) {
				continue;
			}
			item = mntp->mnt_fsname;
			if ( strncmp(item, devblk, strsize(devblk)) == 0
			  && stat(strcat(strcpy(devbuf, devraw),
			    item + strsize(devblk)), &sb) == 0
			  && (sb.st_mode & S_IFMT) == S_IFCHR
			  && major(sb.st_rdev) == slot
			  && VUNIT(sb.st_rdev) == nopartminor ) {
				list[VPART(sb.st_rdev)] =
							strdup(mntp->mnt_dir);
			}
		}

		endmntent(mnttabfd);
	}

	/* read fstab for partition mountpoints not present in mnttab */

	if ((fstabfd = setmntent(fstab, "r")) == NULL) {
		(void) warn(fstab, syserr());
		return(list);
	}
	while ((mntp = getmntent(fstabfd)) != NULL) {
		if ((strcmp(mntp->mnt_type, MNTTYPE_IGNORE) == 0) ||
		    (strcmp(mntp->mnt_type, MNTTYPE_SWAP) == 0)) {
			continue;
		}
		item = mntp->mnt_fsname;
		if (strncmp(item, devblk, strsize(devblk)) == 0
		  && stat(strcat(strcpy(devbuf, devraw),
		    item + strsize(devblk)), &sb) == 0
		  && (sb.st_mode & S_IFMT) == S_IFCHR
		  && major(sb.st_rdev) == slot
		  && VUNIT(sb.st_rdev) == nopartminor
		  /* use mnttab if both tables have entries */
		  && list[VPART(sb.st_rdev)] == 0)
			list[VPART(sb.st_rdev)] = strdup(mntp->mnt_dir);
	}
	endmntent(fstabfd);

	return (list);
}


/*
 * partcmp()
 *
 * Qsort() key comparison of partitions by starting sector numbers.
 */
static int
partcmp(one, two)
reg struct partition	**one;
reg struct partition	**two;
{
	return ((*one)->p_start - (*two)->p_start);
}


/*
 * prtvtoc()
 *
 * Read and print a VTOC.
 */
static int
prtvtoc(name)
char		*name;
{
	reg int		fd;
	reg int		idx;
	reg Freemap	*freemap;
	struct stat	sb;
	static struct	vtoc	*vtoc = 0;

	if (vtoc == (struct vtoc *)0) {
		vtoc = (struct vtoc *)valloc(V_SIZE);
		if (vtoc == (struct vtoc *)0) {
			fatal("valloc", "Not enough core");
		}
	} else {
		bzero(vtoc, V_SIZE);
	}
	fd = tryopen(&name);
	if (fd < 0) {
		perror(name);
		fprintf(stderr, "prtvtoc: can not open %s\n", name);
		return(fd);
	}
	if (stat(name, &sb) < 0) {
		return (warn(name, syserr()));
	}
	if ((sb.st_mode & S_IFMT) != S_IFCHR)
		return (warn(name, "Not a raw device"));
	idx = pread(fd, V_VTOCSEC, V_SIZE, vtoc);
	(void) close(fd);
	if (idx != 0)
		return (-1);
	if (Tflag) {
		if (strlen(vtoc->v_disktype) == 0)
			return(-1);
		printf("%s\n", vtoc->v_disktype);
		return(0);
	}
	freemap = findfree(vtoc);
	puttable(vtoc, freemap, name,
	    getmntpt(major(sb.st_rdev), VUNIT(sb.st_rdev)));
	return (0);
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
char		**name;
{
	int	fd;
	char	*p, *buf;
	int	len;
	static char rawpath[] = "/dev/r";

	/* First try name as-is */
	if ((fd = open(*name, O_RDONLY)) >= 0)
		return(fd);

	/* Build a file name from it */

	if ((p = rindex(*name, '/')) == 0)
		p = *name;
	else
		p++;

	/* Trim off leading 'r' to get block device name */
	if (*p == 'r')
		++p;

	/* Copy this as the disk name */
	if ((buf = malloc(strlen(p)+strlen(rawpath)+1)) == 0)
		return(-1);
	sprintf(buf, "/dev/r%s", p);

	/* Chop off trailing partition specification if any */
	len = strlen(p);
	while ( len > 2 && ispart(p[len-1])) {
		p[len-1] = '\0';
		len--;
	}

	*name = buf;
	if ((fd = open(buf, O_RDONLY)) < 0)
		return(-1);
	return(fd);
}

/*
 * puttable()
 *
 * Print a human-readable VTOC.
 */
static void
puttable(vtoc, freemap, name, mtab)
reg struct vtoc		*vtoc;
reg Freemap		*freemap;
char			*name;
char			**mtab;
{
	reg int		idx;

	if (!hflag && !sflag) {
		(void) printf("* %s partition map\n", name);
		(void) printf("*\n");
		(void) printf("* Disk Type: %*s\n", VTYPCHR, vtoc->v_disktype);
		(void) printf("*\n* Dimensions:\n");
		(void) printf("* %7d bytes/sector\n", vtoc->v_secsize);
		(void) printf("* %7d sectors/track\n", vtoc->v_nsectors);
		(void) printf("* %7d tracks/cylinder\n", vtoc->v_ntracks);
		(void) printf("* %7d cylinders\n", vtoc->v_ncylinders);
		(void) printf("* %7d sectors/cylinder\n", vtoc->v_nseccyl);
		(void) printf("* %7d sectors/disk\n", vtoc->v_capacity);
		(void) printf("*\n");
		(void) printf("* Partition Types:\n");
		(void) printf("* %4d: Empty Slot\n", V_NOPART);
		(void) printf("* %4d: Regular Partition\n", V_RAW);
		(void) printf("* %4d: System Error/Diagnostics Area\n", V_DIAG);
		(void) printf("* %4d: Bootstrap Area\n", V_BOOT);
		(void) printf("* %4d: Reserved Area\n", V_RESERVED);
		(void) printf("* %4d: Firmware Area\n", V_FW);
		(void) printf("*\n");
		if (freemap->fr_size) {
			(void) printf("* Unallocated space:\n");
			(void) printf("*\tFirst     Sector    Last\n");
			(void) printf("*\tSector     Count    Sector \n");
			do {
				(void) printf("*\t");
				(void) printf("%-9d",freemap->fr_start);
				(void) printf(" ");
				(void) printf("%-9d",freemap->fr_size);
				(void) printf(" ");
				(void) printf("%-9d",freemap->fr_size +
							freemap->fr_start - 1);
				(void) printf("\n");
			} while ((++freemap)->fr_size);
			(void) printf("*\n");
		}
	}
	if (!hflag)  {
(void) printf("\
*            Start           Size        Block Sz  Frag Sz\n\
*    Type    Sector          in Sectors  in Bytes  in Bytes  Mount point\n");
	}
	for (idx = 0; idx < vtoc->v_nparts; ++idx) {
		if (sflag && vtoc->v_part[idx].p_type == V_NOPART)
			continue;
(void) printf("%-3d  %-2d      %-9d       %-9d   %-5d     %-5d",
		idx,
		vtoc->v_part[idx].p_type,
		vtoc->v_part[idx].p_start,
		vtoc->v_part[idx].p_size,
		vtoc->v_part[idx].p_bsize,
		vtoc->v_part[idx].p_fsize);
		if (!hflag && mtab && mtab[idx]) {
			(void) printf("    %s", mtab[idx]);
		}
		(void)printf("\n");
	}
}


/*
 * syserr()
 *
 * Return a pointer to a system error message.
 */
static char	err1[30] =  "Unknown error - ";
static char	err2[10] =  "         ";
static char *
syserr()
{
	if (errno <= 0) 
		return("No error (?)");
	if (errno < sys_nerr)
		return(sys_errlist[errno]);
	(void) sprintf(err2, "%d", errno);
	return(strcat(err1, err2));
}

/*
 * usage()
 *
 * Print a helpful message and exit.
 */
static void
usage()
{
	static char before[] = "Usage:\t";
	static char after[] = " [ -h ] [ -s ] [ -t fstab ] [ -m mnttab ] rawdisk ...\n";

	(void) fprintf(stderr, "%s %s %s", before, myname, after);
	exit(1);
}

/*
 * warn()
 *
 * Print an error message. Always returns -1.
 */
static int
warn(what, why)
reg char	*what;
reg char	*why;
{
	(void) fprintf(stderr, "%s: %s: %s\n", myname, what, why);
	return (-1);
}

/*
 * pread()
 *	read the VTOC off the disk
 *
 * The disk drive which is prtvtoc's argument may be an unpartitioned
 * disk, or it may have a partitioning driver pushed on it.  Both
 * of these possibilities are supported.  We first try to do a V_READ
 * ioctl, which should return a sane VTOC from the disk.  If the
 * ioctl fails with EINVAL, then this is probably
 * a raw disk, and we will try the read straight off the raw pack.
 *
 * Returns: -1 on error, 0 otherwise.
 */
static int
pread(fd, block, len, buf)
int		fd;
unsigned long	block;
unsigned long	len;
char		*buf;
{
	int	err, offset;
	struct	vtoc *v;

	if (ioctl(fd, V_READ, buf) == -1 && errno == EINVAL) {
		err = ioctl(fd, RIOFIRSTSECT, &offset);
		if (err < 0) {
			offset = 0;
		}
		block += offset;
		if (lseek(fd, (long)(block * DEV_BSIZE), 0) < (long)0) {
			(void) fprintf(stderr,
			     "prtvtoc: Cannot seek disk to block %d\n", block);
			perror("");
			return(-1);
		}
		if (read(fd, buf, (unsigned)len) != len) {
			(void) fprintf(stderr,
			     "prtvtoc: Cannot read VTOC from disk block %d\n",
								     block);
			perror("");
			return(-1);
		}

		/*
		 * confirm that the VTOC is legitamate
		 */
		v = (struct vtoc *)buf;
		if (v->v_sanity == VTOC_SANE && v->v_version == V_VERSION_1) {
			return(0);
		} else {
			(void) fprintf(stderr,
			     "prtvtoc: Disk does not contain a valid VTOC\n");
			return(-1);
		}
	}
	return(0);
}

/*
 * Fake up a sys-V type of function
 */
char *
strdup(orig)
	char *orig;
{
	register char *p;

	if (p = malloc(strlen(orig)+1))
		strcpy(p, orig);
	return(p);
}
