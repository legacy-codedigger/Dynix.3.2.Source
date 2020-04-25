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

#ident	"$Header: rmvtoc.c 1.4 90/07/25 $"
/*
 * rmvtoc.c
 *
 * Delete a disk partition map ("VTOC").
 */

#include <sys/types.h>
#include <sys/inode.h>
#include <sys/vtoc.h>
#include <sys/param.h>
#include <unistd.h>
#include "saio.h"

/*
 * Definitions.
 */
#define PARTBUFSIZE	20	 /* must be big enough for "xx(yy,255)" */

/*
 * External functions.
 */
void	exit();
char	*calloc();
char	*index();
long	lseek();

/*
 * Internal functions.
 */
static int	rmvtoc();

/*
 * Static variables
 */
static char line[100];

main()
{
	register int idx = 0;
	register int i;
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
	printf("rmvtoc\n\n");
	printf("Type a device name whose VTOC you wish to delete\n");
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

			i = open(line, 2);
			if (i < 0) {
				printf("Failed to open device\n");
			}
		} while (i < 0);

		idx = rmvtoc(i);
		close(i);
		if (idx != 0) {
			printf("rmvtoc: VTOC not removed\n");
		} else {
			printf("rmvtoc: VTOC removed\n");
		}
	}
	/*NOTREACHED*/
}

/*
 * rmvtoc()
 *
 * Read the VTOC to see if it actually exists.  If it does, delete it.
 * Returns: -1 on error, 0 otherwise.
 */
static int
rmvtoc(fd)
	int	fd;
{
	register int err;
	int offset;
	static struct	vtoc	*vtoc = 0;
	unsigned long	block = V_VTOCSEC;
	unsigned long	len = V_SIZE;

	if (vtoc == (struct vtoc *)NULL) {
		callocrnd(DEV_BSIZE);
		vtoc = (struct vtoc *)calloc(V_SIZE);
	}
	err = ioctl(fd, SAIOFIRSTSECT, &offset);
	if (err < 0) {
		return(-1);
	}
	block += offset;
	if (lseek(fd, (long)(block * DEV_BSIZE), 0) == -1) {
		(void) printf("rmvtoc: Cannot seek disk to block %d\n", block);
		return(-1);
	}
	if (read(fd, vtoc, (unsigned)len) != len) {
		(void) printf(
		     "rmvtoc: Cannot read VTOC from disk block %d\n", block);
		return(-1);
	}
	if (vtoc->v_sanity != VTOC_SANE ||
	    vtoc->v_version != V_VERSION_1) {
		printf("rmvtoc: Invalid VTOC on device\n");
		return(-1);
	}
	if (lseek(fd, (long)(block * DEV_BSIZE), 0) == -1) {
		(void) printf("rmvtoc: Cannot seek disk to block %d\n", block);
		return(-1);
	}
	bzero(vtoc, len);
	if (write(fd, (char *)vtoc, (unsigned)len) != len) {
		(void) printf(
		     "rmvtoc: Cannot overwrite disk block %d\n", block);
		return(-1);
	}
	return (0);
}
