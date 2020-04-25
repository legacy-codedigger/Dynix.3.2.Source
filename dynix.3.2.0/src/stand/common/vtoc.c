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

#ident	"$Header: vtoc.c 1.3 90/06/12 $"

/*
 * vtoc.c
 *	Stand-alone VTOC support
 */

/* $Log:	vtoc.c,v $
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/inode.h>
#include <sys/vtoc.h>
#include <unistd.h>
#include "saio.h"

#define PARTBUFSIZE		20	/* must be big enough for "xx(yy,255)" */
#define INPUTBUFSIZE		80	/* max size of SCED input buffer */
#define	ignore_part(type)	((type) == V_NOPART || (type) == V_RESERVED)

static char *vbuf;
caddr_t calloc();
char	*index();

/*
 * vtoc_setboff
 *
 *	Set the partition offset based on information in the VTOC
 *
 * The parameters are a struct iob pointer from a driver open routine
 * and an offset (in sectors) which is to be applied to the beginning of
 * the drive for searching for a VTOC.  This offset is generally needed
 * when dealing with a controller type which reserves space at the
 * front of the disk for things like bad block lists, etc.
 *
 * When vtoc_setboff is called, io->i_boff is the partition number
 * of a disk which the driver is trying to open.
 * If the VTOC is correct and the partition is valid, vtoc_setboff
 * resets io->i_boff to the offset (in sectors) of the partition.
 * io->i_boff is then used by the rest of standalone to be added
 * to the block number in all subsequent IO operations.
 *
 * If the VTOC is missing or invalid, io->i_error gets set to EDEV and
 * vtoc_setboff returns -1.  If the VTOC is correct but the partition is
 * not valid, io->i_error gets set to EUNIT and vtoc_setboff returns -1.
 *
 * Partition numbers equal to V_NUMPAR are assumed to be
 * referencing the whole disk (ie, by a formatter) and the VTOC
 * is not read.
 *
 * Routine tries to be tidy and efficient, to make minimal impact
 * on 8K bootstraps.
 */

vtoc_setboff(io, offset)
	register struct iob *io;
	int offset;
{
	struct vtoc *v;
	ulong cksum;

#ifndef	BOOTXX
	/*
	 * Partition numbers one larger than the max are assumed to
	 * refer to the whole disk.  Skip the VTOC read.
	 */
	if (io->i_boff == V_NUMPAR) {
		io->i_boff = 0;
		return(0);
	}
#endif	/* !BOOTXX */
	if (vbuf == NULL) {
		callocrnd(DEV_BSIZE);
		vbuf = (char *)calloc(V_SIZE);
	}
	io->i_ma = vbuf;
	io->i_cc = V_SIZE;
	io->i_bn = V_VTOCSEC + offset;
	if (devread(io) < 0) {
		io->i_boff = 0;
		return(-1);
	}
	v = (struct vtoc *)vbuf;
	if (v->v_sanity != VTOC_SANE || v->v_version != V_VERSION_1) {
		/*printf("Warning: No VTOC present on device\n");*/
		io->i_error = EDEV;
		return(-1);
	}
#ifndef	BOOTXX
	cksum = v->v_cksum;
	v->v_cksum = 0;
	if (cksum != vtoc_get_cksum(v)) {
		printf("VTOC checksum mismatch - suspect a corrupted VTOC\n");
		io->i_error = EDEV;
		return(-1);
	}
#endif	/* !BOOTXX */
	if (io->i_boff >= v->v_nparts ||
	   v->v_part[io->i_boff].p_type == V_NOPART) {
		printf("Partition %d not in VTOC\n", io->i_boff);
		io->i_error = EUNIT;
		return(-1);
	}
	io->i_boff = v->v_part[io->i_boff].p_start;
	return(0);
}

#ifndef	BOOTXX

/*
 * vtoc_getpsize()
 *	Return the the size of a given partition.
 *
 * The device name passed in as parameter is looked up in the
 * VTOC which is read from disk.  If no VTOC is present on disk
 * or the partition itself is not valid, the size returned is
 * -1.  Note that the assumption is that the driver will try to
 * open() a partition before trying to figure out what its
 * partition size is, since vtoc_getpsize() does no sanity
 * checks of the VTOC.
 */
vtoc_getpsize(device)
	char	*device;
{
	char		*dp;
	char		*pp;
	int		i;
	struct	vtoc	*v = (struct vtoc *)NULL;
	char		partbuf[PARTBUFSIZE];	/* character version of V_NUMPAR */
	char		savedev[INPUTBUFSIZE];	/* local copy of device */
	int		n = V_NUMPAR;
	int		partno;

	/*
	 * Load up partbuf with the character translation of V_NUMPAR.
	 */
	bzero(partbuf, 20);
	pp = partbuf + sizeof(partbuf) - 1;
	*pp-- = '\0';
	do {
		*pp-- = "0123456789"[n%10];
		n /= 10;
	} while (n);
	pp++;

	/*
	 * save the device name to a buffer where we can mess with it.
	 */
	strcpy(savedev, device);

	/*
	 * Replace the partition number with V_NUMPAR
	 */
	dp = index(savedev, ',');
	if (dp == 0) {
		printf("Must specify a device name of the form:");
		printf(" XX(y,z)\n");
		return(-1);
	}
	dp++;
	strcpy(dp, pp);
	dp = savedev + strlen(savedev);
	strcpy(dp, ")");

	/*
	 * Now try opening this device and reading the VTOC
	 */
	i = open(savedev, 0);
	if (i < 0) {
		printf("Unable to open device %s\n", savedev);
		return(-1);
	}
	if (vtoc_pread(i, &v) < 0) {
		printf("Error reading VTOC from %s\n", savedev);
		return(-1);
	}
	close(i);

	if (v->v_sanity != VTOC_SANE || v->v_version != V_VERSION_1) {
		return(-1);
	}

	/*
	 * Determine which partition needs to be looked up.  partbuf[]
	 * is reused here to house partition number string.
	 */
	bzero(partbuf, sizeof(partbuf));
	dp = index(device, ',');
	dp++;
	strcpy(partbuf, dp);
	dp = index(partbuf, ')');
	*dp = '\0';
	partno = atoi(partbuf);
	if (partno < 0 || partno >= v->v_nparts) {
		printf("Unable to find partition %d in VTOC\n", partno);
		return(-1);
	}
	if (ignore_part(v->v_part[partno].p_type)) {
		printf("Unable to find partition %d in VTOC\n", partno);
		return(-1);
	}
	return(v->v_part[partno].p_size);
}

/*
 * vtoc_pread()
 *	read the VTOC off the disk
 *
 * The alligned space is allocated for the buffer if it is zero.
 *
 * Returns: -1 on error, 0 otherwise.
 */
int
vtoc_pread(fd, buf)
	int		fd;
	char		**buf;
{
	unsigned long	block = V_VTOCSEC;
	unsigned long	len = V_SIZE;
	int	err, offset;

	if (*buf == (char *)NULL) {
		callocrnd(DEV_BSIZE);
		*buf = calloc(V_SIZE);
	}
	err = ioctl(fd, SAIOFIRSTSECT, &offset);
	if (err < 0) {
		return(-1);
	}
	block += offset;
	if (lseek(fd, (long)(block * DEV_BSIZE), 0) < (long)0) {
		(void) printf("vtoc: Cannot seek disk to block %d\n",
								block);
		return(-1);
	}
	if (read(fd, *buf, (unsigned)len) != len) {
		(void) printf(
		     "vtoc: Cannot read VTOC from disk block %d\n",
							     block);
		return(-1);
	}
	return(0);
}

/*
 * vtoc_get_cksum()
 * 	return a checksum for the VTOC
 */
vtoc_get_cksum(v)
	struct	vtoc *v;
{
	register long sum;
	register int  nelem = sizeof(struct vtoc) / sizeof(long);
	register long *lptr = (long *)v;

	sum = 0;
	while (nelem-- > 0) {
		sum += *lptr;
		++lptr;
	}
	return (sum);
}


#endif	/* !BOOTXX */
