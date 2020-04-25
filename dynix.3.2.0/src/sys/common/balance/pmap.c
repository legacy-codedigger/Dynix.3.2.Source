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

#ifndef	lint
static	char	rcsid[] = "$Header: pmap.c 2.5 90/06/09 $";
#endif

/*
 * pmap.c
 *	Physical mapping driver.
 *
 * Semantics:
 *	+ This is a pseudo-device (eg, software driver).
 *	+ Up to 65536 physical space regions (with 16-bit major/minor #'s).
 *	+ Configurable initial parameters, number of units.
 *	+ Can map arbitrary ranges of physical space.
 *	+ Handles MM_PHYS and MM_NPMEM types of physical maps.
 *	+ Full physical d_mmap() support.
 *	+ No read/write procedures in the driver.  Are meaningful,
 *	  just not interesting yet.
 *	+ No checking for validity of physical addresses; use care.
 */

/* $Log:	pmap.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/ioctl.h"
#include "../h/user.h"

#include "../balance/pmap.h"

/*
 * Configuration variables defined in conf/conf_pmap.c
 */

extern	struct	pmap_unit pmap_unit[];
extern	int	pmap_nunit;

#ifdef	ns32000
extern	gate_t	pmap_gate;		/* only needed in FGS system */
#endif	ns32000

/*
 * Driver local variables.
 */

sema_t		pmap_mutex;

/*
 * pmapboot()
 *	Initialize driver structures, set up default units.
 */

pmapboot()
{
	init_sema(&pmap_mutex, 1, 0, pmap_gate);
}

/*
 * pmapopen()
 *	"Open" the phys-map device -- just check if legal unit.
 */

pmapopen(dev)
	dev_t	dev;
{
	struct	pmap_unit *pm;
	int	unit = minor(dev);
	int	error = 0;

	if (unit >= pmap_nunit)
		return(ENXIO);

	/* 
	 * Is legal unit.  Allow the open unless held exclusively
	 * (except root).
	 */

	pm = &pmap_unit[unit];

	p_sema(&pmap_mutex, PZERO-1);

	if ((pm->pm_flags & PMAP_EXCL) && u.u_uid != 0)
		error = EBUSY;
	else
		++pm->pm_nopen;

	v_sema(&pmap_mutex);

	return(error);
}

/*
 * pmapclose()
 *	Close a phys-mapped device.
 */

pmapclose(dev)
	dev_t	dev;
{
	struct	pmap_unit *pm = &pmap_unit[minor(dev)];

	p_sema(&pmap_mutex, PZERO-1);

	if (--pm->pm_nopen == 0)
		pm->pm_flags &= ~PMAP_EXCL;

	ASSERT_DEBUG(pm->pm_nopen>0 || pm->pm_nmaps==0, "pmapclose: mapped");
	ASSERT_DEBUG(pm->pm_nopen >= 0, "pmapclose: open cnt");

	v_sema(&pmap_mutex);
}

/*
 * pmapioctl()
 *	Misc ioctl's to fuss parameters.
 */

pmapioctl(dev, com, data)
	dev_t		dev;
	int		com;
	register struct pmap_ioc *data;
{
	register struct pmap_unit *pm = &pmap_unit[minor(dev)];
	register int	error = 0;

	p_sema(&pmap_mutex, PZERO-1);			/* for consistency */

	switch(com) {

	case PMAPIOCGETP:				/* get parameters */
		data->pi_paddr = pm->pm_paddr;
		data->pi_size = pm->pm_size * NBPG;
		data->pi_flags = pm->pm_flags;
		if (pm->pm_nmaps > 0)
			data->pi_flags |= PMAP_MAPPED;
		break;

	case PMAPIOCSETP:				/* set parameters */
		/*
		 * If paddr, size are ok and unit not mapped, change
		 * the setting.  Don't copy in flag bits.
		 */

		if (!suser())
			error = EPERM;
		else if ((data->pi_paddr|data->pi_size) & CLOFSET)
			error = EINVAL;
		else if (pm->pm_nmaps != 0)
			error = EBUSY;
		else {
			pm->pm_paddr = data->pi_paddr;
			pm->pm_size = data->pi_size / NBPG;
			pm->pm_flags &= ~PMAP_IOCFLAGS;
			pm->pm_flags |= (data->pi_flags & PMAP_IOCFLAGS);
		}
		break;

	case PMAPIOCEXCL:				/* exclusive access */
		if (pm->pm_nopen > 1)
			error = EBUSY;
		else
			pm->pm_flags |= PMAP_EXCL;
		break;

	case PMAPIOCNXCL:				/* shared access */
		pm->pm_flags &= ~PMAP_EXCL;
		break;

	default:
		error = ENXIO;
		break;
	}

	v_sema(&pmap_mutex);

	return(error);
}

/*
 * pmapmmap()
 *	Perform mapping functions.
 */

/*ARGSUSED*/
pmapmmap(dev, cmd, off, size, prot)
	dev_t	dev;
	int	cmd;
	u_long	off;			/* HW pages */
	int	size;			/* HW pages */
	int	prot;			/* PROT_READ|PROT_WRITE */
{
	register struct	pmap_unit *pm = &pmap_unit[minor(dev)];
	int	val = 0;

	switch(cmd) {

	case MM_MAP:		/* verify, set up map: (offset, size, prot) */
		/*
		 * Verify size against what's there.
		 */

		p_sema(&pmap_mutex, PZERO-1);

		if (pm->pm_size == 0)			/* can't map 0 size */
			val = ENXIO;
		else if (off+size > pm->pm_size)	/* too big */
			val = ENOSPC;
		else {
			++pm->pm_nmaps;
			if (pm->pm_flags & PMAP_NPMEM)
				val = MM_NPMEM;
			else
				val = MM_PHYS;
		}

		v_sema(&pmap_mutex);
		break;

	case MM_UNMAP:		/* done with map: (offset, size, prot) */
		p_sema(&pmap_mutex, PZERO-1);
		--pm->pm_nmaps;
		ASSERT_DEBUG(pm->pm_nmaps >= 0, "pmapmmap: unmap count");
		v_sema(&pmap_mutex);
		break;

	case MM_REFPG:		/* add ref to page: (offset) */
		ASSERT_DEBUG(pm->pm_nmaps > 0, "pmapmmap: refpg count");
		ASSERT_DEBUG(off < pm->pm_size, "pmapmmap: refpg size");
		val = pm->pm_paddr + (off * NBPG);
		break;

	case MM_SWPOUT:		/* swap out ref to map:	(offset, size) */
	case MM_SWPIN:		/* swap in ref to map:	(offset, size) */
		break;

	case MM_DEREFPG:	/* remove page ref:	(offset) */
	case MM_REALLOC:	/* loose claim to page:	(page, ndx) */
	case MM_PGOUT:		/* page-out page:	(page, ndx) */
	default:
		printf("cmd=%d\n", cmd);
		panic("pmapmmap: bad function");
                /*
                 *+ The pmap driver was called through its mmap entry point
                 *+ to perform an unsupported operation.
                 */
		/*NOTREACHED*/
	}

	return(val);
}
