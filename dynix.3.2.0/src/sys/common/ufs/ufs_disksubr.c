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

#ifndef	lint
static	char	rcsid[] = "$Header: ufs_disksubr.c 1.16 1991/04/25 18:24:13 $";
#endif

/*
 * Copyright (c) 1982, 1986, 1988 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ufs_disksubr.c	7.10 (Berkeley) 6/11/88
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/vnode.h"
#include "../h/kernel.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/proc.h"
#include "../h/vm.h"
#include "../h/vtoc.h"
#include "../h/file.h"

#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/mftpr.h"
#include "../machine/plocal.h"			/* instrumentation */
#include "../machine/gate.h"			/* instrumentation */

#include "syslog.h"


/*
 * Disk VTOC utilities.
 */


#ifdef DEBUG
int	vtoc_debug = 0;
#endif
char * partchr();

/*
 * data_part is true for partition types which can be used for user
 * data.
 */
#define data_part(T)		((T) == V_RAW || (T) == V_DIAG)

/*
 * Attempt to read a disk label from a device
 * using the indicated stategy routine.
 * The label must be partly set up before this:
 * secpercyl and anything required in the strategy routine
 * (e.g., sector size) must be filled in before calling us.
 * Returns true on success and false failure.
 */
int
readdisklabel(dev, strat, vtp, cmpt, firstsect)
	dev_t dev;
	int (*strat)();
	register struct vtoc *vtp;
	struct	cmptsize *cmpt;
	daddr_t	firstsect;
{
	extern	long	vtoc_get_cksum();
	register struct buf *bp;
	struct vtoc *dvtp;
	int	part;
	int	ret;

#ifdef DEBUG
	if (vtoc_debug)
		printf("readdisklabel(dev:0x%x, strat:0x%x, vtp:0x%x, cmpt:0x%x, firstsect:%d)\n", 
						dev, strat, vtp, cmpt,firstsect);
#endif
	bp = geteblk(V_SIZE);
	bp->b_dev = dev;
	bp->b_blkno = LABELSECTOR+firstsect;
	bp->b_bcount = V_SIZE;
	bp->b_error = 0;
	bp->b_flags = B_READ;
	bp->b_iotype = B_FILIO;
	BIODONE(bp) = 0;			/* new IO starting */
	(*strat)(bp);
	biowait(bp);
	if (bp->b_flags & B_ERROR) {
		u.u_error = EIO;
		ret = 0;
	} else {
		ret = 1;
		dvtp = (struct vtoc *)bp->b_un.b_addr;
		if (dvtp->v_sanity != VTOC_SANE || dvtp->v_version != V_VERSION_1) {
			if (!cmpt) {
#ifdef DEBUG
				if (vtoc_debug)
					printf("readdisklabel: No VTOC\n");
#endif
				vtp->v_nparts = 0;
				u.u_error = EINVAL;
				ret = 0;
			} else {

				vtp->v_nparts = NUMPARTS;
#ifdef DEBUG
				if (vtoc_debug)
					printf("readdisklabel: using compatability\n");
#endif
				for (part=0; part<NUMPARTS; part++ ) {
					vtp->v_part[part].p_start = cmpt[part].cmpt_start;
					vtp->v_part[part].p_size = cmpt[part].cmpt_size;
					vtp->v_part[part].p_type = V_RAW;
					vtp->v_part[part].p_bsize = 0;
					vtp->v_part[part].p_fsize = 0;
				}
			}
			vtp->v_sanity = 0xbadbad;
			vtp->v_version = 0;
			vtp->v_secsize = 0;
			vtp->v_ntracks = 0;
			vtp->v_nsectors = 0;
			vtp->v_ncylinders = 0;
			vtp->v_nseccyl = 0;
		} else if (vtoc_get_cksum(dvtp)-dvtp->v_cksum != dvtp->v_cksum) {
			u.u_error = ENXIO;
			ret = 0;
#ifdef DEBUG
			if (vtoc_debug)
				printf("readdisklabel: bad checksum!0x%x 0x%x\n",
					vtoc_get_cksum(dvtp)-dvtp->v_cksum, dvtp->v_cksum);
#endif
		} else {
			*vtp = *dvtp;
		}
	}
	if (vtp->v_nparts > V_NUMPAR)
		vtp->v_nparts = V_NUMPAR;
	bp->b_flags = B_INVAL | B_AGE;
#ifdef DEBUG
	if (ret && vtoc_debug)
		prtvtoc(vtp);
#endif
	brelse(bp);
	return (ret);
}


/*
 * Check new disk label for sensibility
 * before setting it.
 */
setdisklabel(ovtp, nvtp, vo)
	register struct vtoc *ovtp, *nvtp;
	unsigned short *vo;
{
	int i;

	if (nvtp->v_sanity != VTOC_SANE || nvtp->v_version != V_VERSION_1 ||
 	    vtoc_get_cksum(nvtp)-nvtp->v_cksum != nvtp->v_cksum) 
		return (EINVAL);

	/*
	 * For each partition which is currently open, verify that it is
	 * not having its base changed nor its length shortened
	 */
	for (i = 0; i < V_NUMPAR; ++i) {
		struct partition *op, *np;

		op = &ovtp->v_part[i];
		np = &nvtp->v_part[i];

		/*
		 * Don't need to check these types
		 */
		if ((op->p_type == V_NOPART) || (op->p_type == V_RESERVED))
			continue;

		/*
		 * Don't need to check if not open
		 */
		if (vo[i] == 0)
			continue;

		/*
		 * Otherwise correlate bases and lengths
		 */
		if (op->p_start != np->p_start)
			return(EBUSY);
		if (op->p_size > np->p_size)
			return(EBUSY);

		/*
		 * Don't allow it to be yanked out from under us, either
		 */
		if ((data_part(op->p_type)) && !(data_part(np->p_type)))
			return(EBUSY);
		if (np->p_type == V_NOPART)
			return(EBUSY);
	}

	/*
	 * Calculate checksum.  The initial zeroing reflects the fact that
	 * a PTX checksum is not self-complementing.
	 */
 	nvtp->v_cksum = 0;
 	nvtp->v_cksum = vtoc_get_cksum(nvtp);
	*ovtp = *nvtp;
	return (0);
}

/*
 * Write disk label back to device after modification.
 */
int
writedisklabel(dev, strat, vtp, firstsect)
	dev_t dev;
	int (*strat)();
	register struct vtoc *vtp;
	daddr_t	firstsect;
{
	struct buf *bp;
	int error = 0;

#ifdef DEBUG
	if (vtoc_debug) {
		printf("writedisklabel(dev:0x%x, strat:0x%x, vtp:0x%x, part:%d)\n", 
			dev, strat, vtp, firstsect);
		prtvtoc(vtp);
	}
#endif
	bp = geteblk(V_SIZE);
	bp->b_dev = dev;
	bp->b_blkno = LABELSECTOR+firstsect;
	bp->b_bcount = V_SIZE;
	bp->b_error = 0;
	bp->b_iotype = B_FILIO;
	bp->b_flags = B_WRITE;
	*((struct vtoc *)bp->b_un.b_addr) = *vtp;
	BIODONE(bp) = 0;
	(*strat)(bp);
	biowait(bp);
	if (bp->b_flags & B_ERROR) {
		error = u.u_error;		/* XXX */
		u.u_error = 0;
	}
	brelse(bp);
#ifdef DEBUG
	if (vtoc_debug && error)
		printf("vtoc write error %d\n",error);
#endif
	return (error);
}


/*
 * vtoc_get_cksum()
 * 	return a checksum for the VTOC
 */
static long
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


/*
 * This is the standardised VTOC error checking policy for localy opened
 * ufs disks.
 */

/* ARGSUSED */

vtoc_opencheck(dev, mode, strat, vtp, cmpt, firstsect, lastsect, oldmode, ddev)
	dev_t	dev;
	int	mode;
	int (*strat)();
	register struct vtoc *vtp;
	struct	cmptsize *cmpt;
	daddr_t	firstsect;
	daddr_t lastsect;
	unsigned int oldmode;
	char	*ddev;
{
	int	part;

	if (!V_ALL(dev)) {
		part = VPART(dev);
#ifdef DEBUG
		if (vtoc_debug) {
			printf("vtoc_opencheck(0x%x,0x%x,,,,%d,%d,0x%x,\"%s\")\n",
			dev,mode,firstsect,lastsect,oldmode,ddev);
		}
#endif

		/* Read VTOC info from disk.
		 * get compatibility info from "cmpt" if no VTOC is on 
		 * the disk.  
		 */

		if (!readdisklabel(dev, strat, vtp, cmpt, firstsect)) {
#ifdef DEBUG
			if (vtoc_debug) {
				printf("%s%d: Could not read VTOC\n", 
					ddev, VUNIT(dev));
			}
#endif
			return (ENXIO);
		}
		if (vtp->v_part[part].p_size == 0) {
			/*
			 * Good drive but fail open if partition table is
			 * invalid.
			 */
#ifdef DEBUG
			if (vtoc_debug) {
				printf("%s%d%s: partition size 0\n", 
					ddev, VUNIT(dev), partchr(dev));
			}
#endif
			return (ENXIO);
		}

		if (lastsect != (daddr_t)-1 && vtp->v_part[part].p_size+vtp->v_part[part].p_start > lastsect ) {
			/*
			 * Good drive but fail open if partition table is
			 * invalid.
			 */
#ifdef DEBUG
			if (vtoc_debug) {
				printf("%s%d%s: partition size too big \n", 
					ddev, VUNIT(dev), partchr(dev));
			}
#endif
			return (ENXIO);
		}
		/*
		 * Ensure that V_RESERVED partitions can't be written
		 */
		if ((mode & FWRITE) &&
			(vtp->v_part[part].p_type == V_RESERVED)) {
#ifdef DEBUG
			if (vtoc_debug){
				printf("%s%d%s: partition V_RESERVED\n", 
					ddev, VUNIT(dev), partchr(dev));
			}
#endif
			return (EACCES);
		}
		/*
		 * Ensure that swaping and mounting can only be done to
		 * V_RAW partitions.
		 */
		if ((mode & (FMOUNT|FSWAP)) &&
			                 (vtp->v_part[part].p_type != V_RAW)) {
#ifdef DEBUG
			if (vtoc_debug) {
				printf("%s%d%s: partition is not V_RAW\n", 
					ddev, VUNIT(dev), partchr(dev));
			}
#endif
			return (EACCES);
		}
		/*
		 * Ensure we are not trying to swap to a mounted partition
		 * or swapping to a mounted partition.
		 */
		if ((mode & FMOUNT) && (oldmode & FSWAP)) {
#ifdef DEBUG
			if (vtoc_debug) {
				printf("%s%d%s: is being used for swapping\n", 
					ddev, VUNIT(dev), partchr(dev));
			}
#endif
			return (EACCES);
		}
		if ((mode & FSWAP) && (oldmode & FMOUNT)) {
#ifdef DEBUG
			if (vtoc_debug) {
				printf("%s%d%s: is mounted\n", 
					ddev, VUNIT(dev), partchr(dev));
			}
#endif
			return (EACCES);
		}
	}
	return(0);
}


/*
 * convert partition number into corresponding conventional disk name letter.
 */

char *
partchr(dev)
	dev_t	dev;
{
	static char num_buf[4];

	int c = VPART(dev);

	if (V_ALL(dev)) 	/* no partition number */
		num_buf[0] = '\0';
	else if (c < 26) {	/* use a - z */
		num_buf[0] = c + 'a';
		num_buf[1] = '\0';
	} else {		/* return 3 digit number */
		num_buf[0] = c/100;
		num_buf[1] = (c%100)/10;
		num_buf[2] = c%10;
		num_buf[3] = '\0';
	}
	return (num_buf);
}


#ifdef DEBUG
prtvtoc(vtp)
struct vtoc *vtp;
{
	register i;

	printf("VTOC: @0x%x\n",vtp);
	printf("v_sanity     = 0x%x\n", vtp->v_sanity);
	printf("v_version    = %d\n", vtp->v_version);
	printf("v_cksum      = 0x%x\n", vtp->v_cksum);
	printf("v_size       = 0x%x\n", vtp->v_size);
	printf("v_nparts     = %d\n", vtp->v_nparts);
	printf("v_secsize    = %d\n", vtp->v_secsize);
	printf("v_ntracks    = %d\n", vtp->v_ntracks);
	printf("v_nsectors   = %d\n", vtp->v_nsectors);
	printf("v_ncylinders = %d\n", vtp->v_ncylinders);
	printf("v_rpm        = %d\n", vtp->v_rpm);
	printf("v_capacity   = %d\n", vtp->v_capacity);
	printf("v_nseccyl    = %d\n", vtp->v_nseccyl);
	printf("v_disktype   = \"%s\"\n", vtp->v_disktype);
	printf("v_part[]  p_type  p_bsize p_fsize   p_start   p_size\n");
	for (i=0; i<V_NUMPAR; i++ ) {
		if (vtp->v_part[i].p_type == V_NOPART)
			continue;
		if ( i<10 )
			printf("[0%d]\t ",i);
		else
			printf("[%d]\t ",i);
		printf("0x%x\t%d\t%d\t%d\t%d\n", vtp->v_part[i].p_type,
				vtp->v_part[i].p_bsize, vtp->v_part[i].p_fsize,
				vtp->v_part[i].p_start, vtp->v_part[i].p_size);
	}
}
#endif
