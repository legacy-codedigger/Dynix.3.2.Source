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

#ident	"$Header: prtvtoc.c 1.2 90/10/10 $"
/*
 * prtvtoc.c
 *
 * Print a disk partition map ("VTOC").
 */

/* $Log:	prtvtoc.c,v $
 */

#include <sys/types.h>
#include <sys/inode.h>
#include <sys/vtoc.h>
#include <sys/param.h>
#include <unistd.h>
#include "saio.h"

typedef unsigned int uint;

/*
 * Definitions.
 */
#define	reg		register /* Convenience */
#define PARTBUFSIZE	20	 /* must be big enough for "xx(yy,255)" */


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
char	*calloc();
char	*index();
void	qsort();
long	lseek();

/*
 * Internal functions.
 */
static void	fatal();
static int	partcmp();
static int	prtvtoc();
static void	puttable();
static int	warn();


/*
 * Static variables
 */
static char line[100];
static char *name = line;

main()
{
	reg int	idx = 0;
	reg int	i;
	char	*dp;
	char	*pp;
	char	partbuf[PARTBUFSIZE];
	int	n = V_NUMPAR;

	/*
	 * Load up partbuf with the character translation of V_NUMPAR.
	 */
	bzero(partbuf, PARTBUFSIZE);
	pp = partbuf + sizeof(partbuf) - 1;
	*pp-- = '\0';
	do {
		*pp-- = "0123456789"[n%10];
		n /= 10;
	} while (n);
	pp++;

	/*
	 * Now open the device
	 */
	printf("prtvtoc\n\n");
	printf("Type a device name which contains a VTOC at the prompt\n");
	printf("Type \"exit\" to exit\n");
	for (;;) {
		do  {
			printf(": ");
			gets(line);
			if (strcmp("exit", line) == 0)
				exit(idx);
			/*
			 * Replace the partition number with V_NUMPAR
			 */
			dp = index(line, ',');
			if (dp == 0) {
				printf("Must specify a device name of the form:");
				printf(" XX(y,z)\n");
				i = -1;
				continue;
			}
			dp++;
			strcpy(dp, pp);
			dp = line + strlen(line);
			strcpy(dp, ")");

			i = open(line, 0);
		} while (i < 0);

		idx |= prtvtoc(i);
		close(i);
	}
	/*NOTREACHED*/
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
	(void) printf("prtvtoc: %s: %s\n", what, why);
	exit(1);
}

/*
 * findfree()
 *
 * Find free space on a disk.  The freemap returned is an array
 * of descriptors which for each chunk of freespace give the
 * starting sector and size of that chunk.
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
		fatal("findfree()", "Too many partitions on disk!");
	list = sorted;
	for (part = vtoc->v_part; part < vtoc->v_part + vtoc->v_nparts; ++part)
		*list++ = part;
	*list = 0;
	qsort((char *) sorted, (uint) (list - sorted), sizeof(*sorted), partcmp);
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
 * Returns: -1 on error, 0 otherwise.
 * No check is made of the checksum, so this tool can be
 * useful for rebuilding a hosed VTOC.
 */
static int
prtvtoc(fd)
	int	fd;
{
	reg int		idx;
	reg Freemap	*freemap;
	static struct	vtoc	*vtoc = 0;

	idx = vtoc_pread(fd, (char **)&vtoc);
	if (idx < 0)
		return (-1);

	if (vtoc->v_sanity != VTOC_SANE ||
	    vtoc->v_version != V_VERSION_1) {
		printf("prtvtoc: Invalid VTOC on device\n");
		return(-1);
	}
	freemap = findfree(vtoc);
	puttable(vtoc, freemap, name);
	return (0);
}

/*
 * puttable()
 *
 * Print a human-readable VTOC.
 */
static void
puttable(vtoc, freemap, name)
	reg struct vtoc		*vtoc;
	reg Freemap		*freemap;
	char			*name;
{
	reg int			idx;

	(void) printf("* %s partition map\n", name);
	(void) printf("*\n");
	(void) printf("* Disk Type: %s\n", vtoc->v_disktype);
	(void) printf("*\n* Dimensions:\n");
	(void) printf("* %d bytes/sector\n", vtoc->v_secsize);
	(void) printf("* %d sectors/track\n", vtoc->v_nsectors);
	(void) printf("* %d tracks/cylinder\n", vtoc->v_ntracks);
	(void) printf("* %d cylinders\n", vtoc->v_ncylinders);
	(void) printf("* %d sectors/cylinder\n", vtoc->v_nseccyl);
	(void) printf("* %d sectors/disk\n", vtoc->v_capacity);
	(void) printf("*\n");
	(void) printf("* Partition Types:\n");
	(void) printf("* %d: Empty Slot\n", V_NOPART);
	(void) printf("* %d: Regular Partition\n", V_RAW);
	(void) printf("* %d: System Error/Diagnostics Area\n", V_DIAG);
	(void) printf("* %d: Bootstrap Area\n", V_BOOT);
	(void) printf("* %d: Reserved Area\n", V_RESERVED);
	(void) printf("* %d: Firmware Area\n", V_FW);
	(void) printf("*\n");
	if (freemap->fr_size) {
		(void) printf("* Unallocated space:\n");
		(void) printf("*\tFirst     Sector    Last\n");
		(void) printf("*\tSector     Count    Sector \n");
		do {
			(void) printf("*\t");
			(void) printf("%d",freemap->fr_start);
			(void) printf(" ");
			(void) printf("%d",freemap->fr_size);
			(void) printf(" ");
			(void) printf("%d",freemap->fr_size +
						freemap->fr_start - 1);
			(void) printf("\n");
		} while ((++freemap)->fr_size);
		(void) printf("*\n");
	}
	(void) printf("\
*		Start		Size		Block Sz	Frag Sz\n\
*	Type	Sector		in Sectors	in Bytes	in Bytes\n");
	for (idx = 0; idx < vtoc->v_nparts; ++idx) {
		if (vtoc->v_part[idx].p_type == V_NOPART)
			continue;
		(void) printf("\
%d	%d	%d		%d		%d		%d",
		idx,
		vtoc->v_part[idx].p_type,
		vtoc->v_part[idx].p_start,
		vtoc->v_part[idx].p_size,
		vtoc->v_part[idx].p_bsize,
		vtoc->v_part[idx].p_fsize);
		(void)printf("\n");
	}
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
	(void) printf("prtvtoc: %s: %s\n", what, why);
	return (-1);
}
