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

/* $Header: pmap.h 2.0 86/01/28 $ */

/*
 * pmap.h
 *	Physical-mapped memory driver ioctl and misc driver parameters.
 */

/* $Log:	pmap.h,v $
 */

/* 
 * IO controls.
 */

struct	pmap_ioc {
	u_long	pi_paddr;			/* start physical address */
	u_long	pi_size;			/* size (bytes) */
	u_char	pi_flags;			/* flags (see below) */
};

#define	PMAPIOCGETP	_IOR(p,0,struct pmap_ioc)	/* get parameters */
#define	PMAPIOCSETP	_IOW(p,1,struct pmap_ioc)	/* set parameters */
#define	PMAPIOCEXCL	_IO(p, 2)			/* exclusive access */
#define	PMAPIOCNXCL	_IO(p, 3)			/* shared access */

/*
 * Flags.
 * PMAP_IOCFLAGS are those copied in by PMAPIOCSETP.
 */

#define	PMAP_EXCL	0x01			/* exclusive access */
#define	PMAP_MAPPED	0x02			/* is currently mapped */
#define	PMAP_NPMEM	0x04			/* map MM_NPMEM, else MM_PHYS */

#define	PMAP_IOCFLAGS	(PMAP_NPMEM)

#ifdef	KERNEL
/*
 * Internal driver structures.
 */

struct	pmap_unit {
	u_long		pm_paddr;		/* start physical address */
	u_long		pm_size;		/* size (bytes) */
	u_char		pm_flags;		/* flags (see above) */
	short		pm_nopen;		/* current # opens */
	short		pm_nmaps;		/* current # maps */
};
#endif	KERNEL
