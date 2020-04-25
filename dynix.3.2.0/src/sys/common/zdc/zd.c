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
static	char	rcsid[] = "$Header: zd.c 1.47 1991/08/06 18:18:39 $";
#endif

/*
 * zd.c
 *
 * ZDC SMD Disk Driver
 */

/* $Log: zd.c,v $
 *
 *
 *
 *
 *
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/file.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/systm.h"
#include "../h/uio.h"
#include "../h/ioctl.h"
#include "../h/proc.h"
#include "../h/kernel.h"
#include "../h/dk.h"
#include "../h/vm.h"
#include "../h/vtoc.h"
#include "../h/cmn_err.h"

#include "../balance/slicreg.h"
#include "../balance/clkarb.h"

#include "../machine/intctl.h"
#include "../machine/pte.h"
#include "../machine/mftpr.h"
#include "../machine/plocal.h"

#include "../zdc/zdc.h"
#include "../zdc/zdbad.h"
#include "../zdc/ioconf.h"
#include "../zdc/zdioctl.h"

#define PARTCHR	partchr
static char *partchr();

#define	NUDGE_ZDC(ctlp, cbp, s)	{ \
	(s) = splhi(); \
	mIntr((ctlp)->zdc_slicaddr, CBBIN, (u_char)((cbp) - (ctlp)->zdc_cbp)); \
	splx((s)); \
	}
#define	PZOPEN		(PZERO - 1)		/* not signallable */
#define	b_diskaddr	b_resid
#define	b_psect		b_error

extern	struct	zdc_ctlr *zdctrlr;	/* zdctrlr array */
extern	struct	zd_unit	 *zdunit;	/* zdunit array */
extern	struct	cmptsize *zdparts[];	/* Partition tables */
extern	int	zdntypes;		/* known drive types */
extern	int	zdc_iovpercb;		/* no of iovecs per cb */
extern	int	zdc_AB_throttle;	/* Channel A&B DMA throttle */
extern	short	zdcretry;		/* retry count on errors */
extern	u_char	zdctrl;			/* additional icb_ctrl bits */
extern	u_char	base_cb_intr;		/* base interrupt for zdc driver */
extern	u_char	base_err_intr;		/* base controller interrupt */
extern	lock_t	zdcprlock;		/* coordinate error printfs */

int	zdstrat();			/* Forward reference */
int	zdcstrat();			/* Forward reference */

#ifdef	DEBUG
int	zddebug = 0;
#endif	DEBUG

#ifdef MIRRORTEST
#define ZIOCTESTFAIL  _IOW(z, 255, u_char)
static struct zd_unit *zdtofail;
static u_char zdfailcode;
lock_t zdfaillock;
#endif /* MIRRORTEST */

caddr_t	zd_compcodes[] = {
	"Command in progress",
	"Successful completion",
	"Write protect fault",
	"Drive Fault",
	"Seek error",
	"Seek timeout",
	"Channel timeout",
	"DMA timeout",
	"Header ECC error",
	"Soft ECC error",
	"Correctable ECC error",
	"Uncorrectable ECC error",
	"Sector not found",
	"Bad data sector",
	"Sector overrun",
	"No data synch",
	"Fifo data lost",
	"Illegal cb_cmd",
	"Illegal cb_mod",
	"Illegal disk address",
	"cb_addr not 16-byte aligned",
	"Illegal cb_count",
	"cb_iovec not 32-byte aligned",
	"Non-zero cb_iovec and page size invalid",
	"Illegal icb_pagesize",
	"icb_dumpaddr not 16-byte aligned",
	"Bad drive",
	"In-use CB reused",
	"Access error during DMA",
	"Channel not configured",
	"Channel was reset",
	"Unexpected status from DDC",
	"Unknown Completion code"
};
int	zdncompcodes = sizeof(zd_compcodes) / sizeof(zd_compcodes[0]);

/*
 *	This maximum is used to prevent counter overflows
 */
#define ZDMAXOPEN 3200
int	zd_max_open = ZDMAXOPEN;

/*
 * zdopen
 *
 * Return:
 *	0 - success.
 *	EACCES - open for write but drive write-protected.
 *	EBUSY - exclusive conflict.
 *	EIO - failure to read media.
 *	ENXIO - all other failures.
 */
zdopen(dev, mode)
	dev_t	dev;
	int	mode;
{
	register struct zd_unit *up;
	int	retval;				/* return value */
	struct	zdcdd	*zdget_chancfg();
	struct zdc_ctlr *ctlrp;
	int chan, part;

	retval = 0;
	up = &zdunit[VUNIT(dev)];
	part = VPART(dev);
#ifdef	DEBUG
	if (zddebug)
		printf("zdopen(dev:0x%x, mode:0x%x\n", dev, mode);
#endif	DEBUG

	/*
	 * Fail open if the unit number is bad or if the unit was not bound
	 * to a drive during configuration.
	 */
	if (VUNIT(dev) >= zdc_conf->zc_nent || up->zu_state == ZU_NOTFOUND) {
#ifdef DEBUG
		printf("zd%d: Bad unit number zc_nent=%d zu_state=0x%x\n",
			VUNIT(dev), zdc_conf->zc_nent, up->zu_state);
#endif
		return (ENXIO);
	}

	/*
	 * Now get serious
	 */
	p_sema(&up->zu_ocsema, PZOPEN);

	(void)p_lock(&up->zu_lock, SPLZD);

	/*
	 * Ensure that a writer to the V_ALL device holds out all other
	 * accessors of the V_ALL device
	 */
	if (V_ALL(dev) && (up->zu_flags & ZUF_ALLBUSY)) {
		v_lock(&up->zu_lock, SPL0);
		v_sema(&up->zu_ocsema);
#ifdef DEBUG
		if (zddebug)
			printf("zd%d: Whole disc open already open\n", VUNIT(dev));
#endif
		
		return (EBUSY);
	}

	/*
	 * check to see if still good drive.
	 */
	if (up->zu_state == ZU_BAD) {
		v_lock(&up->zu_lock, SPL0);
		v_sema(&up->zu_ocsema);
#ifdef DEBUG
		if (zddebug)
			printf("zd%d: Bad state\n", VUNIT(dev));
#endif
		return (ENXIO);
        }
	/*
	 * check to see if writing to a write protected disk.
	 */
	if ((mode & FWRITE) && (up->zu_cfg & ZD_READONLY)) {
		v_lock(&up->zu_lock, SPL0);
		v_sema(&up->zu_ocsema);
		/*
		 * Fail open for write on a write-protected drive.
		 */
#ifdef DEBUG
		if (zddebug)
			printf("zd%d: Read Only\n", VUNIT(dev));
#endif
		return (EACCES);
	}

	/*
	 * Drop lock since second opens will block behind
	 * up->zu_ocsema
	 * NOTE: zu_state and zu_flags are unprotected  against
	 * modifications for second open, so need to lock if modified.
	 */
	v_lock(&up->zu_lock, SPL0);

        /*
         * Verify that the channel is not owned.
         * If so, error out.
         */
        chan = up->zu_drive & 1;
        ctlrp = &zdctrlr[up->zu_ctrlr];
        (void) p_lock(&ctlrp->zdc_ctlrlock, SPLHI);
        if (ctlrp->zdc_chan_owner[chan]) {
#ifdef DEBUG
		if (zddebug)
			printf("zd%d: Channel is locked down\n", VUNIT(dev));
#endif
		retval = EACCES;
	} else if (up->zu_nopen >= zd_max_open) {
		/*
		 * Refuse further opens to prevent counter overflow
		 */
#ifdef	DEBUG
		if (zddebug)
			printf("zd%d: refusing further opens already have %d\n",
				VUNIT(dev), up->zu_nopen);
#endif	DEBUG
		retval = EBUSY;
	}

	v_lock(&ctlrp->zdc_ctlrlock, SPL0);

	/*
	 * If any errors, bomb out now after releasing lock/sema
	 */
	if (retval) {
		v_sema(&up->zu_ocsema);
		return (retval);
	}


	if (up->zu_nopen != 0) {
		/*
		 * Already opened at least once.
		 */
		if ((mode & FEXCL) || (up->zu_flags & ZUF_EXCLUSIVE)) {
			/*
			 * Trying to open exclusively when device already
			 * open, or trying to open when device open
			 * exclusively!
			 */
#ifdef DEBUG
			if (zddebug)
				printf("zd%d: Exclusive open already open\n", 
					VUNIT(dev));
#endif
			retval = EBUSY;
		} else if (up->zu_state == ZU_NO_RW) {
			/*
			 * Only one formatter at a time please...
			 */
#ifdef DEBUG
			if (zddebug)
				printf("zd%d: Formatter open already open\n",
					VUNIT(dev));
#endif
			retval = EACCES;
		}
	} else {
		retval = zd_first_open(dev, up);
	}

	if (!retval && (retval = vtoc_opencheck(dev, mode, zdcstrat,
				up->zu_part, zdparts[up->zu_drive_type],
				(daddr_t)up->zu_firstsect,
				(daddr_t)up->zu_drive_size,
				up->zu_modes[part], "zd")) == 0) {
		/*
		 * Successful open
		 */
		up->zu_nopen++;
		if (V_ALL(dev)) {
			/*
			 * Successful open on whole-disk;
			 * lock others out
			 */
			if (mode & FWRITE) {
				(void)p_lock(&up->zu_lock, SPLZD);
				up->zu_flags |= ZUF_ALLBUSY;
				v_lock(&up->zu_lock, SPL0);
			}
		} else {
			up->zu_opens[part]++;
			up->zu_modes[part] |= mode;
		}
		if (mode & FEXCL) {		/* open for exclusive use */
			(void)p_lock(&up->zu_lock, SPLZD);
			up->zu_flags |= ZUF_EXCLUSIVE;
			v_lock(&up->zu_lock, SPL0);
		}
	}
#ifdef DEBUG
	if (zddebug)
		printf("zd%d: open state = %d\n", VUNIT(dev), up->zu_state);
#endif  /* DEBUG */

	v_sema(&up->zu_ocsema);
	return (retval);
}


/*
 * First open!
 * Get channel configuration information and bad block list.
 * Return error value but for certain errors set zu_state = ZU_NO_RW 
 * and return ok. This permits for example formatting of a bad disk.
 */

static int
zd_first_open(dev, up)
	dev_t	dev;
	register struct zd_unit *up;
{
	int	retval;
	register struct zdcdd	*dd;		/* channel configuration */
	register struct	zdc_dev *zdv;		/* config data */
	u_char	oldcfg;				/* previous cfg state */

	up->zu_state = ZU_GOOD;		/* assume good */
	oldcfg = up->zu_cfg;

	if ((retval = zdprobe_drive(dev, up)) != 0) {
		/*
		 * Error whilst attempting probe!
		 */
#ifdef DEBUG
		printf("zd%d: Bad probe\n", VUNIT(dev));
#endif
		return (retval);
	}
	/*
	 * Error if drive still not there or not online.
	 */
	if (up->zu_cfg == ZD_NOTFOUND ||		/* Not there */
	    (up->zu_cfg & ZD_ONLINE) != ZD_ONLINE) {	/* Offline */
		if ((oldcfg & ZD_ONLINE) == ZD_ONLINE) {
			/*
			 * It had been online.
			 */
			disk_offline();
		}
#ifdef DEBUG
		if (zddebug)
			printf("zd%d: is not found or offline\n", VUNIT(dev));
#endif
		return (ENXIO);
	}

	if (oldcfg == ZD_NOTFOUND || ((oldcfg & ZD_ONLINE) != ZD_ONLINE)) {
		/*
		 * It had been offline.
		 */
		disk_online();
	}

	if ((dd = zdget_chancfg(dev, up)) == NULL) {
#ifdef DEBUG
		printf("zd%d: Cannot get channel configuration\n", VUNIT(dev));
#endif
		return (EIO);
	}
	zdv = &zdc_conf->zc_dev[VUNIT(dev)];

	/*
	 * Does channel configuration match the drive configuration.
	 * If bound drive_type and mismatch, then drive is marked to
	 * disallow normal read/write operations.
	 */
	if (dd->zdd_sectors == 0) {
		up->zu_state = ZU_NO_RW;
		printf("zd%d: drive unformatted\n", up - zdunit);
                /*
                 *+ The drive was not formatted.
                 *+ Normal read/write operations will be disallowed on
                 *+ this drive.
                 */
		return (0);
	} else if ( (zdv->zdv_drive_type != ANY &&
	     zdv->zdv_drive_type != dd->zdd_drive_type)) {
		up->zu_state = ZU_NO_RW;
		printf("zd%d: drive type mismatch - check configuration.\n",
			up - zdunit);
                /*
                 *+ There was a mismatch in the drive types between the
                 *+ channel configuration and the drive configuration.
                 *+ Normal read/write operations will be disallowed on
                 *+ this drive.
                 */
		return (0);
	}
	/*
	 * Make sure drive size and first sect are set,
	 * since the channel config may have been set
	 * since system boot.
	 */
	up->zu_drive_size = dd->zdd_sectors * dd->zdd_tracks * dd->zdd_cyls;
	up->zu_firstsect = dd->zdd_sectors * dd->zdd_tracks;

	/*
	 * If the drive is not formatted or is formatted differently than
	 * the other drives on the channel, then cannot read the bad block
	 * list. In this case, only format operations via ioctl will be
	 * allowed. Read and write operations will return error.
	 */
	if ((up->zu_cfg & (ZD_FORMATTED|ZD_MATCH)) != (ZD_FORMATTED|ZD_MATCH)) {
		up->zu_state = ZU_NO_RW;
		printf("zd%d: warning: %s.\n", up - zdunit,
			((up->zu_cfg & ZD_FORMATTED) != ZD_FORMATTED)
				? "drive unformatted"
				: "drive/channel mismatch");
                /*
                 *+ Either the drive was not formatted or it was not formatted 
		 *+ in the same manner as other drives on the channel. 
		 *+ This prevents the bad
                 *+ block list from being read in.
                 */
		return (0);
	}
	if (dd->zdd_drive_type >= zdntypes) {
		/*
		 * Unknown drive type. Check zdparts[] in binary conf file or VTOC info.
		 */
		up->zu_state = ZU_NO_RW;	/* allow reformat */
		printf("zd%d: unknown drive type - check configuration.\n",
			up - zdunit);
		/*
		 *+ The drive type for this is unknown, Check zdpart[] in
		 *+ conf_zd.c for list of supported types.
		 */
		return (0);
	}

	up->zu_drive_type = dd->zdd_drive_type;

	/*
	 * Online and formatted drive - get bad block list.
	 * If cannot correctly read bad block list - only allow ioctl
	 * operations.
	 */
	if (zdgetbad(dev, up) < 0) {
		printf("zd%d: Cannot read bad block list.\n", up - zdunit);
                /*
                 *+ The driver couldn't read the bad block list.  Only ioctl
                 *+ operations will be allowed on this drive.
                 */
		up->zu_state = ZU_NO_RW;
	}


	/*
	 * Sanity: zero out open counts and modes.
	 */
	bzero((char *)up->zu_opens, sizeof(unsigned short) * V_NUMPAR);
	bzero((char *)up->zu_modes, sizeof(unsigned int) * V_NUMPAR);

	return (0);
}


/*
 * zdclose
 *	Close the device.
 *	If last close, free memory holding bad block list.
 */
/*ARGSUSED*/
zdclose(dev, flag)
	dev_t dev;
	int flag;
{
	register struct zd_unit *up;
	register int size;
	struct zdc_ctlr *ctlrp;
	int chan, part;

	up = &zdunit[VUNIT(dev)];
	part = VPART(dev);
#ifdef	DEBUG
	if (zddebug)
		printf("C");
#endif	DEBUG
	p_sema(&up->zu_ocsema, PZOPEN);
	p_lock(&up->zu_lock, SPLZD);
	if (!V_ALL(dev)) {
		up->zu_opens[part] -= 1;
		up->zu_modes[part] &= ~(flag&FUSEM);
	}

	if (--up->zu_nopen == 0) {
		/*
		 * Last close!
		 * Free memory allocated to hold bad block list.
		 * If drive has removable media, the next open may
		 * have different bad block list.
		 */
		if (up->zu_zdbad != NULL) {
			size = (up->zu_zdbad->bz_nsnf * sizeof(struct bz_bad))
				+ sizeof(struct zdbad) - sizeof(struct bz_bad);
			size = roundup(size, DEV_BSIZE);
			wmemfree((caddr_t)up->zu_zdbad, size);
			up->zu_zdbad = NULL;
		}
		up->zu_flags &= ~(ZUF_EXCLUSIVE|ZUF_FORMATTED);

		/*
		 * Don't let it go away with the controller 
		 * channel reserved.  Release it if it failed 
		 * to restore it.
		 */
		chan = up->zu_drive & 1;
		ctlrp = &zdctrlr[up->zu_ctrlr];
		if (ctlrp->zdc_chan_owner[chan] == up)
			ctlrp->zdc_chan_owner[chan] = (struct zd_unit *)NULL;
	}

	/*
	 * If we're holding the whole-disk partition, flag it as free now
	 */
	if (V_ALL(dev) && (up->zu_flags & ZUF_ALLBUSY))
		up->zu_flags &= ~ZUF_ALLBUSY;

	v_lock(&up->zu_lock, SPL0);
	v_sema(&up->zu_ocsema);
}


/*
 * zdprobe_drive
 *	Probe for the status of a particular drive then update
 *	appropriate fields in controller and unit structures.
 */
static int
zdprobe_drive(dev, up)
	dev_t	dev;
	register struct zd_unit *up;
{
	register struct buf *bp;	/* ioctl buffer */
	register struct cb  *cbp;	/* cb argument */
	int	error;

	bp  = &up->zu_ioctl;
	cbp = &up->zu_ioctlcb;
	bufalloc(bp);			/* will always get since 1st open */
	bp->b_flags = B_IOCTL;
	bp->b_bcount = 0;		/* data in CB */
	bp->b_un.b_addr = NULL;
	bp->b_dev = dev;
	bp->b_blkno = 0;
	bp->b_error = 0;
	bp->b_proc = u.u_procp;
	bp->b_iotype = B_FILIO;
	BIODONE(bp) = 0;
	/* Fill out CB */
	cbp->cb_cmd = ZDC_PROBEDRIVE;
	/*
	 * Insert at head of list and zdstart!
	 */
	(void)p_lock(&up->zu_lock, SPLZD);
	bp->av_forw = NULL;
	up->zu_bhead.av_forw = bp;
	zdstart(up);
	v_lock(&up->zu_lock, SPL0);
	biowait(bp);
	error = geterror(bp);
	if (error) {
		buffree(bp);
		return (error);
	}
	/*
	 * extract drive cfg data.
	 */
	up->zu_cfg = ((struct probe_cb *)cbp)->pcb_drivecfg[up->zu_drive];
	zdctrlr[up->zu_ctrlr].zdc_drivecfg[up->zu_drive] = up->zu_cfg;
	buffree(bp);
	return (0);
}

/*
 * zdget_chancfg
 *	Get the channel configuration for the channel on which this
 *	drive resides. Fills in channel configuration in controller structure.
 */
static struct zdcdd *
zdget_chancfg(dev, up)
	dev_t	dev;
	register struct zd_unit *up;
{
	register struct buf	*bp;	/* ioctl buffer */
	register struct cb	*cbp;	/* cb argument */
	struct zdcdd	*dd;		/* channel configuration data */
	int	chan;			/* Channel A (0) or channel B (1) */

	bp  = &up->zu_ioctl;
	cbp = &up->zu_ioctlcb;
	bufalloc(bp);			/* will always get since 1st open */
	bp->b_un.b_addr = wmemall(sizeof(struct zdcdd), 1);
	bzero(bp->b_un.b_addr, sizeof(struct zdcdd));
	bp->b_flags = B_READ | B_IOCTL;
	bp->b_bcount = sizeof(struct zdcdd);
	bp->b_dev = dev;
	bp->b_blkno = 0;
	bp->b_error = 0;
	bp->b_proc = u.u_procp;
	bp->b_iotype = B_FILIO;
	BIODONE(bp) = 0;
	/* Fill out CB */
	cbp->cb_cmd = ZDC_GET_CHANCFG;
	cbp->cb_addr = (u_long)bp->b_un.b_addr;
	cbp->cb_count = sizeof(struct zdcdd);
	/*
	 * Insert at head of list and zdstart!
	 */
	(void)p_lock(&up->zu_lock, SPLZD);
	bp->av_forw = NULL;
	up->zu_bhead.av_forw = bp;
	zdstart(up);
	v_lock(&up->zu_lock, SPL0);
	biowait(bp);
	if (bp->b_flags & B_ERROR) {
		wmemfree(bp->b_un.b_addr, sizeof(struct zdcdd));
		buffree(bp);
		return (NULL);
	}
	/*
	 * Extract Channel configuration data.
	 */
	chan = up->zu_drive & 1;
	dd = (chan) ? &zdctrlr[up->zu_ctrlr].zdc_chanB
		    : &zdctrlr[up->zu_ctrlr].zdc_chanA;
	bcopy(bp->b_un.b_addr, (caddr_t)dd, sizeof(struct zdcdd));
	wmemfree(bp->b_un.b_addr, sizeof(struct zdcdd));
	buffree(bp);
	/*
	 * On the first get of the channel configuration set the
	 * dma throttle for the channel. Make sure that the configuration
	 * is valid.
	 */
	if (dd->zdd_sectors != 0 &&
	    (zdctrlr[up->zu_ctrlr].zdc_dma_throttle & (1 << chan)) == 0)
		set_dma_throttle(&zdctrlr[up->zu_ctrlr], dd, chan);
	return (dd);
}

/*
 * set_dma_throttle
 *	If 1st open on channel, set the dma throttle
 */
/*ARGSUSED*/
static
set_dma_throttle(ctlrp, dd, chan)
	register struct zdc_ctlr *ctlrp;	/* controller */
	struct zdcdd	*dd;			/* channel configuration data */
	int	chan;				/* channel A (0) or B (1) */
{
	register int	throttle;	/* throttle count */
	spl_t	s_ipl;

	s_ipl = p_lock(&ctlrp->zdc_ctlrlock, SPLHI);
	if ((ctlrp->zdc_dma_throttle & (1 << chan)) == 0) {
		throttle = zdc_AB_throttle;
		if (throttle > SLB_TVAL)
			throttle = SLB_TVAL;
		wrslave(ctlrp->zdc_slicaddr, (u_char)(SL_G_CHAN0 + chan),
					(u_char)(SLB_TH_ENB | throttle));
		ctlrp->zdc_dma_throttle |= (1 << chan);
	}
	v_lock(&ctlrp->zdc_ctlrlock, s_ipl);
}

/*
 * getchksum
 *	Calculate bad block list checksum
 */
static
getchksum(lptr, nelem, seed)
	register long *lptr;
	register int nelem;
	long seed;
{
	register long sum;

	sum = seed;
	while (nelem-- > 0) {
		sum ^= *lptr;
		++lptr; 
	}
	return (sum);
}

/*
 * zdgetbad
 *	- get the bad block list for this unit.
 * Return:
 *	 0 - success
 *	-1 - failure
 */
static
zdgetbad(dev, up)
	dev_t	dev;
	struct zd_unit *up;
{
	register struct cb	*cbp;
	register struct	bz_bad	*fbzp, *tbzp;
	register struct buf	*bp;		/* ioctl buffer */
	register int	size;
	struct	zdbad	*zdp;			/* bad block list */
	struct	zdcdd	*dd;			/* disk description */
	int	zdpsize;
	int	block;
	caddr_t	addr;
	static struct	zdbad	null_bbl;	/* fake bad block list */

	/*
	 * Initialize a fake bad block list in case we receive a
	 * SNF failure whilst reading the bad block list!
	 */
	null_bbl.bz_nelem = 0;
	null_bbl.bz_nsnf = 0;
	up->zu_zdbad = &null_bbl;

	dd = (up->zu_drive & 1) ? &zdctrlr[up->zu_ctrlr].zdc_chanB
				: &zdctrlr[up->zu_ctrlr].zdc_chanA;
	size = 1;				/* get 1 for starters */
	zdp = (struct zdbad *)wmemall(DEV_BSIZE, 1);
	/*
	 * Read bad block list.
	 */
	bp = &up->zu_ioctl;
	bufalloc(bp);
	bp->b_dev = dev;
	bp->b_blkno = 0;
	bp->b_proc = u.u_procp;
	bp->b_iotype = B_FILIO;
	cbp = &up->zu_ioctlcb;
	cbp->cb_mod = 0;
	cbp->cb_cmd = ZDC_READ;
	cbp->cb_cyl = ZDD_DDCYL;
	addr = (caddr_t)zdp;
	for (block = 0; block < size; block++) {
		cbp->cb_head = 0;
		cbp->cb_sect = ZDD_NDDSECTORS + block;
		while (cbp->cb_head < MIN(dd->zdd_tracks, BZ_NBADCOPY)) {
			bp->b_bcount = DEV_BSIZE;
			bp->b_flags = B_READ | B_IOCTL;
			bp->b_un.b_addr = addr;
			bp->b_error = 0;
			BIODONE(bp) = 0;
			cbp->cb_count = DEV_BSIZE;
			cbp->cb_addr = (u_long)addr;
			/*
			 * Queue and start I/O
			 */
			(void)p_lock(&up->zu_lock, SPLZD);
			bp->av_forw = NULL;
			up->zu_bhead.av_forw = bp;
			zdstart(up);
			v_lock(&up->zu_lock, SPL0);
			biowait(bp);
			if ((bp->b_flags & B_ERROR) != B_ERROR)
				break;
			/*
			 * If could not read - try next track.
			 */
			cbp->cb_head++;
			cbp->cb_sect = block;
		}

		/*
		 * If cannot read the first block of bad block list.
		 * give up and return.
		 */
		if (bp->b_flags & B_ERROR) {
			printf("zd%d: Cannot read block %d of bad block list.\n",
					up - zdunit, block);
                        /*
                         *+ The driver couldn't read the first block of the bad
                         *+ block list.
                         */
			if (block == 0)
				wmemfree((caddr_t)zdp, DEV_BSIZE);
			else
				wmemfree((caddr_t)zdp, size << DEV_BSHIFT);
			buffree(bp);
			up->zu_zdbad = (struct zdbad *)NULL;
			return (-1);
		}
		if (block == 0) {
			size = (zdp->bz_nelem * sizeof(struct bz_bad))
				+ sizeof(struct zdbad) - sizeof(struct bz_bad);
			size = howmany(size, DEV_BSIZE);
			if (size > ((dd->zdd_sectors - ZDD_NDDSECTORS) >> 1)) {
				printf("zd%d: Bad block list corrupted!\n",
					up - zdunit);
                                /*
                                 *+ The bad block list was corrupted.
                                 */
				wmemfree((caddr_t)zdp, DEV_BSIZE);
				buffree(bp);
				up->zu_zdbad = (struct zdbad *)NULL;
				return (-1);
			}
			zdpsize = size << DEV_BSHIFT;
			/*
			 * copy block 0 to new location in free memory, so
			 * that bad block list will be contiguous.
			 */
			addr = wmemall(zdpsize, 1);
			bcopy((caddr_t)zdp, addr, (unsigned)DEV_BSIZE);
			wmemfree((caddr_t)zdp, DEV_BSIZE);
			zdp = (struct zdbad *)addr;
		}
		addr += DEV_BSIZE;
	} /* end of for */ 

	/*
	 * done with I/O - free buf header.
	 */
	buffree(bp);

	/*
	 * Confirm data integrity via checksum.
	 */
	size = (zdp->bz_nelem * sizeof(struct bz_bad)) / sizeof(long);
	if (zdp->bz_csn != getchksum((long *)zdp->bz_bad, size,
				(long)(zdp->bz_nelem ^ zdp->bz_nsnf))) {
		printf("zd%d: Checksum failed!\n", up - zdunit);
                /*
                 *+ The checksum on the bad block list failed.
                 */
		wmemfree((caddr_t)zdp, zdpsize);
		up->zu_zdbad = (struct zdbad *)NULL;
		return (-1);
	}

	/*
	 * Copy only BZ_SNF entries into unit's bad block list.
	 * 
	 * Calculate size of needed bad block list.
	 * That is, only BZ_SNF entries are needed.
	 */
	size = (zdp->bz_nsnf * sizeof(struct bz_bad))
		+ sizeof(struct zdbad) - sizeof(struct bz_bad);
	size = roundup(size, DEV_BSIZE);
	up->zu_zdbad = (struct zdbad *)wmemall(size, 1);
#ifdef	DEBUG
	if (zddebug)
		bzero((caddr_t)up->zu_zdbad, (u_int)size);
#endif	DEBUG
	*up->zu_zdbad = *zdp;
	tbzp = up->zu_zdbad->bz_bad;
	for (fbzp = zdp->bz_bad; fbzp < &zdp->bz_bad[zdp->bz_nelem]; fbzp++) {
		if (fbzp->bz_rtype == BZ_SNF)
			*tbzp++ = *fbzp;
	}
#ifdef DEBUG
	if (zddebug)
		zd_printbbl(up);
#endif /* DEBUG */
	wmemfree((caddr_t)zdp, zdpsize);
	return (0);
}

/*
 * zdstrat
 * 	zd disk read/write routine.
 * Perform various checks and queue request and call start routine to
 * initiate I/O to the device.
 */
zdstrat(bp)
	register struct buf *bp;
{
	register struct zd_unit *up;
	register struct zdcdd	*dd;		/* channel configuration */
	register int sector;
	register int nspc;
	struct	partition *part;
	struct	diskaddr diskaddress;
	spl_t s_ipl;
	long	sect_off;
	long	plength;

#ifdef	DEBUG
	if (zddebug > 1)
		printf("zdstrat(%c): bp=%x, dev=%x, cnt=%d, blk=%x, vaddr=%x\n",
			(bp->b_flags & B_READ) ? 'R' : 'W',
			bp, bp->b_dev, bp->b_bcount, bp->b_blkno,
			bp->b_un.b_addr);
	else	if (zddebug)
			printf("%c", (bp->b_flags & B_READ) ? 'R' : 'W');
#endif	DEBUG

	up = &zdunit[VUNIT(bp->b_dev)];
	/*
	 * Error if NO_RW operations are permitted.
	 */
	if ((up->zu_state == ZU_NO_RW) && !(bp->b_flags & B_IOCTL)) {
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		bp->b_resid = bp->b_bcount;
		biodone(bp);
		return;
	}
	dd = (up->zu_drive & 1)	? &zdctrlr[up->zu_ctrlr].zdc_chanB
				: &zdctrlr[up->zu_ctrlr].zdc_chanA;
	nspc = dd->zdd_sectors * dd->zdd_tracks;

	if (up->zu_flags & ZUF_FORMATTED) {
		sect_off = 0;
		plength = 0x7fffffff;
	} else {
		part = &(up->zu_part->v_part[VPART(bp->b_dev)]);
		sect_off = part->p_start;
		plength = part->p_size;
	}
	/*
	 * Size and partitioning check.
	 *
	 * Fail request if bogus byte count, if address not aligned to
	 * ADDRALIGN boundary, or if transfer is not entirely within a
	 * disk partition.
	 */
	if (bp->b_bcount <= 0
	||  ((bp->b_bcount & (DEV_BSIZE -1)) != 0)		/* size */
	||  ((bp->b_iotype == B_RAWIO) &&
	     (((int)bp->b_un.b_addr & (ADDRALIGN - 1)) != 0))	/* alignment */
	||  (bp->b_blkno < 0)					/* partition */
	||  ((bp->b_blkno + (bp->b_bcount >> DEV_BSHIFT)) > plength)) {
		bp->b_flags |= B_ERROR;
		bp->b_error = EINVAL;
		bp->b_resid = bp->b_bcount;
		biodone(bp);
		return;
	}

	sector = bp->b_blkno + sect_off;
	diskaddress.da_cyl = sector / nspc;
	sector %= nspc;
	diskaddress.da_head = sector / dd->zdd_sectors;
	diskaddress.da_sect = sector % dd->zdd_sectors;
	bp->b_diskaddr = *(long *)&diskaddress;
	bp->b_psect = zdgetpsect(&diskaddress, dd);

	s_ipl = p_lock(&up->zu_lock, SPLZD);
	if (up->zu_state == ZU_BAD) {
		v_lock(&up->zu_lock, s_ipl);
		/*
		 * Controller/channel/Drive has gone bad!
		 */
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		bp->b_resid = bp->b_bcount;
		biodone(bp);
		return;
	}
	disksort(&up->zu_bhead, bp);
	up->zu_flags |= ZUF_ACTIVE;
	zdstart(up);
	v_lock(&up->zu_lock, s_ipl);
}

/*
 * zdgetpsect
 *	Determine physical sector in track where the I/O  transfer is to
 *	occur. Used by ZDC firmware for RPS optimization.
 */
static
zdgetpsect(dp, dd)
	register struct	diskaddr *dp;
	register struct zdcdd	 *dd;
{
	if (dd->zdd_tskew == 1)
		return (( ((dd->zdd_tracks - 1 + dd->zdd_cskew) * dp->da_cyl)
				+ dp->da_head + dp->da_sect)
					 % (dd->zdd_sectors + dd->zdd_spare));
	/*
	 * track skew != 1
	 */
	return (( (((dd->zdd_tskew*(dd->zdd_tracks-1)) + dd->zdd_cskew)
								 * dp->da_cyl)
		  + (dd->zdd_tskew*dp->da_head) + dp->da_sect)
					% (dd->zdd_sectors + dd->zdd_spare));
}

/*
 * zdfill_iovec
 *	- fill out cb_iovec for the I/O request.
 *
 * B_RAWIO, B_PTEIO, B_PTBIO cases must flush TLB to avoid stale mappings
 * thru Usrptmap[], since this is callable from interrupt procedures (SGS only).
 *
 * Panics if bad pte found; "can't" happen.
 */

static u_long
zdfill_iovec(bp, iovstart)
#ifndef	i386
	register				/* want optimial on 032's */
#endif
	struct buf *bp;
	u_long *iovstart;
{
	register struct pte *pte;
	register int	count;
	register u_long	*iovp;
	u_long	retval;
	unsigned offset;
	extern struct pte *vtopte();

	/*
	 * Source/target pte's are found differently based on type
	 * of IO operation.
	 */
	switch(bp->b_iotype) {

	case B_RAWIO:					/* RAW IO */
		/*
		 * In this case, must look into alignment of physical
		 * memory, since we can start on any ADDRALIGN boundary.
		 */
		flush_tlb();
		pte = vtopte(bp->b_proc, clbase(btop(bp->b_un.b_addr)));
		count = (((int)bp->b_un.b_addr & CLOFSET) + bp->b_bcount
							 + CLOFSET) / CLBYTES;
		retval = (u_long)bp->b_un.b_addr;
		break;

	case B_FILIO:					/* file-sys IO */
		/*
		 * Filesys/buffer-cache IO.  These are always cluster aligned
		 * both physically and virtually.
		 * Note: also used when to kernel memory acquired via wmemall().
		 * For example, channel configuration buffer in zdget_chancfg().
		 */
		pte = &Sysmap[btop(bp->b_un.b_addr)];
		count = (bp->b_bcount + CLOFSET) / CLBYTES;
		retval = (u_long)bp->b_un.b_addr;
		break;

	case B_PTEIO:					/* swap/page IO */
		/*
		 * Pte-based IO -- already know pte of 1st page, which
		 * is cluster aligned, and b_count is a multiple of CLBYTES.
		 */
		flush_tlb();
		pte = bp->b_un.b_pte;
		count = (bp->b_bcount + CLOFSET) / CLBYTES;
		retval = PTETOPHYS(*pte);
		break;

	case B_PTBIO:					/* Page-Table IO */
		/*
		 * Page-Table IO: like B_PTEIO, but can start/end with
		 * non-cluster aligned memory (but is always HW page
		 * aligned). Count is multiple of NBPG.
		 *
		 * Separate case for greater efficiency in B_PTEIO.
		 */
		flush_tlb();
		pte = bp->b_un.b_pte;
		retval = PTETOPHYS(*pte);
		offset = PTECLOFF(*pte);
		pte -= btop(offset);
		count = (offset + bp->b_bcount + CLOFSET) / CLBYTES;
		break;

	default:
		panic("zdfill_iovec: bad b_iotype");
                /*
                 *+ Before issuing a command to the controller,
                 *+ the DCC driver was filling out the command block
                 *+ and found that it
                 *+ had been passed an invalid buffer type as an argument.
                 */
		/*NOTREACHED*/
	}

	/*
	 * Now translate PTEs and fill-in iovectors.
	 */
	iovp = iovstart;
	while (count--) {
		*iovp++ = PTETOPHYS(*pte);
		pte += CLSIZE;
	}
	return (retval);
}

/*
 * zdstart
 *	- intitiate I/O request to controller.
 * If controller is busy just return. Otherwise stuff appropriate CB
 * with request and notify ZDC.
 *
 * Called with unit structure locked at SPLZD.
 */
zdstart(up)
	register struct zd_unit *up;
{
	register struct cb	*cbp;
	register struct buf	*bp;
	register struct zdc_ctlr *ctlrp;
	spl_t	s_ipl;

#ifdef	DEBUG
	if (zddebug)
		printf("S");
#endif	DEBUG
	bp = up->zu_bhead.av_forw;
	ctlrp = &zdctrlr[up->zu_ctrlr];
	cbp = up->zu_cbptr;
	if (cbp->cb_bp == NULL && cbp[1].cb_bp == NULL) {
		/*
		 * Drive is idle.
		 * If fp_lights - turn activity light on.
		 * Get starting time.
		 */
		if (fp_lights) {
			s_ipl = splhi();
			FP_IO_ACTIVE;
			splx(s_ipl);
		}
		up->zu_starttime = time;
	}
	if (cbp->cb_bp == NULL || cbp[1].cb_bp == NULL) {
		/*
		 * Fill CB.
		 */
		if (cbp->cb_bp)
			++cbp;
		if (bp->b_flags & B_IOCTL) {
			/*
			 * copy 1st half of cb (what fw will see)
			 */
			bcopy((caddr_t)&up->zu_ioctlcb, (caddr_t)cbp, FWCBSIZE);
			if (cbp->cb_cmd != ZDC_READ_LRAM
			&&  cbp->cb_cmd != ZDC_WRITE_LRAM)
				cbp->cb_addr = zdfill_iovec(bp, cbp->cb_iovstart);
			cbp->cb_mod |= up->zu_mod;
		} else {
			*(long *)&cbp->cb_diskaddr = bp->b_diskaddr;
			cbp->cb_psect = (u_char)bp->b_psect;
			cbp->cb_count = bp->b_bcount;
			cbp->cb_mod = up->zu_mod;
			cbp->cb_cmd = (bp->b_flags & B_READ) ? ZDC_READ : ZDC_WRITE;
			cbp->cb_addr = zdfill_iovec(bp, cbp->cb_iovstart);
		}
		bp->b_resid = bp->b_bcount;
		cbp->cb_transfrd = 0;
		cbp->cb_bp = bp;
		cbp->cb_errcnt = 0;
		cbp->cb_state = ZD_NORMAL;
		cbp->cb_iovec = cbp->cb_iovstart;
		/*
		 * Notify ZDC of job request.
		 */
#ifdef	DEBUG
		if (zddebug > 2)
			zddumpcb(cbp);
#endif	DEBUG
		NUDGE_ZDC(ctlrp, cbp, s_ipl);
		up->zu_bhead.av_forw = bp->av_forw;
	}
}

/*
 * zdintr
 *	Normal request completion interrupt handler.
 */
zdintr(vector)
	u_char	vector;
{
	register struct cb	 *cbp;
	register struct	zd_unit  *up;
	register struct	buf	 *donebp;
	register struct	zdc_ctlr *ctlrp;
	struct	zdcdd *dd;
	int	zdcvec;
	daddr_t	blkno;
	int	i;
	int	part;
	u_char	val;
	spl_t	s_ipl;
	int	start;
	dev_t	dev;

#ifdef	DEBUG
	if (zddebug)
		printf("I");
#endif	DEBUG
	zdcvec = vector - base_cb_intr;
	ctlrp = &zdctrlr[zdcvec >> NCBZDCSHFT];
	cbp = ctlrp->zdc_cbp + (zdcvec & (NCBPERZDC-1));
	up = &zdunit[cbp->cb_unit];
	if (cbp->cb_bp == NULL) {
		if (ctlrp->zdc_state == ZDC_DEAD) {
			printf("zdc%d drive %d: Spurious interrupt from dead controller.\n",
				ctlrp - zdctrlr,
				(cbp - ctlrp->zdc_cbp) / NCBPERDRIVE);
                        /*
                         *+ The system received an interrupt from a DCC
                         *+ controller that was marked as being dead.
                         */
			return;
		}
		if (cbp->cb_unit < 0) {
			printf("zdc%d drive %d: Interrupt from unknown unit.\n",
				ctlrp - zdctrlr,
				(cbp - ctlrp->zdc_cbp) / NCBPERDRIVE);
                        /*
                         *+ The system received an interrupt from an unknown 
			 *+ unit on a DCC controller.
                         */
			return;
		}
		printf("zd%d: Spurious interrupt.\n", cbp->cb_unit);
                /*
                 *+ The system received a spurious interrupt from a 
		 *+ DCC controller.
                 */
		return;
	}
	dev = cbp->cb_bp->b_dev;
	part = VPART(dev);

#ifdef MIRRORTEST
	s_ipl = p_lock(&zdfaillock, SPLZD);
	if (up == zdtofail) {
		cbp->cb_compcode = zdfailcode;
		zdfailcode = 0;
		zdtofail = (struct zd_unit *)0;
	}
	v_lock(&zdfaillock, s_ipl);
#endif /* MIRRORTEST */

	/*
	 * Record completion code to save possible error for user to
	 * get through ioctl ZIOGERR.
	 */
	up->zu_l_compcode = cbp->cb_compcode;

	/*
	 * Separate normal case from error cases for performance.
	 */
	if (cbp->cb_compcode == ZDC_DONE) {
		if (cbp->cb_state == ZD_NORMAL) {
			donebp = cbp->cb_bp;
			donebp->b_resid = 0;
			goto donext;
		}
		if (cbp->cb_state & ZD_RESET) {
			/*
			 * Reset completed.
			 */
			cbp->cb_state &= ~ZD_RESET;
			if (cbp->cb_errcnt++ < zdcretry) {
				zdretry(cbp, up, ctlrp);
				return;
			}
			/*
			 * Fail I/O request.
			 */
			donebp = cbp->cb_bp;
			donebp->b_flags |= B_ERROR;
			donebp->b_error = EIO;
			goto donext;
		}
		if (cbp->cb_state & ZD_REVECTOR) {
			/*
			 * Completed revector. Continue with rest of transfer.
			 */
			zdcontinue(cbp, up, ctlrp);
			return;
		}
	}

	/*
	 * E R R O R  O C C U R R E D !
	 */
	if (cbp->cb_cmd == ZDC_READ_HDRS) {
#ifdef DEBUG
		if (zddebug) {
			printf("zd%d: ZDC_READ_HDRS failure.\n", cbp->cb_unit);
			zddumpcb(cbp);
		}
#endif DEBUG
		donebp = cbp->cb_bp;
		donebp->b_flags |= B_ERROR;
		donebp->b_error = EIO;
		goto donext;
	}

	/*
	 * If not doing retry reset or revectoring, update b_resid.
	 */
	if (cbp->cb_state == ZD_NORMAL)
		cbp->cb_bp->b_resid = cbp->cb_count;

	blkno = ((cbp->cb_bp->b_bcount - cbp->cb_bp->b_resid) >> DEV_BSHIFT)
		+ cbp->cb_bp->b_blkno;

	if (cbp->cb_state & ZD_RESET) {
		/*
		 * Error occurred during attempted reset for previous
		 * error. Give up and shutdown the drive.
		 */
		s_ipl = p_lock(&zdcprlock, SPLZD);
		printf("zd%d: Reset Failed.\n", cbp->cb_unit);
                /*
                 *+ An error occurred when the DCC driver attempted to reset
                 *+ the controller to recover from a previous error.
                 */
		if (cbp->cb_compcode >= zdncompcodes) {
			printf("zd%d: Bad cb_compcode 0x%x.\n",
				 cbp->cb_unit, cbp->cb_compcode);
                        /*
                         *+ While attempting to reset the controller
                         *+ to recover from a previous error,
                         *+ the DCC driver received an invalid
                         *+ completion code from the DCC firmware.
                         */

			cbp->cb_compcode = zdncompcodes - 1;
		}
		hard_error(cbp, blkno);
		zddumpstatus(cbp);
		v_lock(&zdcprlock, s_ipl);
		zdshutdrive(up);
	} else switch (cbp->cb_compcode) {

	/*
	 * Command completed successfully but an ecc error occurred.
	 */
	case ZDC_SOFTECC:
	case ZDC_CORRECC:
		/*
		 * Corrected or Soft ECC error.
		 */
		dd = (up->zu_drive & 1) ? &ctlrp->zdc_chanB : &ctlrp->zdc_chanA;
		start = V_ALL(dev) ? 0 : up->zu_part->v_part[part].p_start;
		blkno = cbp->cb_cyl -
			(start / (dd->zdd_sectors * dd->zdd_tracks));
		blkno *= (dd->zdd_sectors * dd->zdd_tracks);
		blkno += (cbp->cb_head * dd->zdd_sectors);
		blkno += cbp->cb_sect;
		s_ipl = p_lock(&zdcprlock, SPLZD);
		printf("zd%d%s: %s at (%d, %d, %d).\n",
			cbp->cb_unit, PARTCHR(cbp->cb_bp->b_dev),
			zd_compcodes[cbp->cb_compcode],
			cbp->cb_cyl, cbp->cb_head, cbp->cb_sect);
		printf("zd%d%s: Filesystem blkno = %d.\n", cbp->cb_unit,
			PARTCHR(cbp->cb_bp->b_dev), blkno);
                /*
                 *+ The DCC driver received the specified error
                 *+ completion status from the DCC firmware for the
                 *+ indicated drive partition.  The '%s' translates
                 *+ to the error code received.  Information about
                 *+ that error appears in this chapter.
                 *+ The final '(%d, %d, %d)' indicates the cylinder,
                 *+ head, and sector in the failing command block.
                 */
		zddumpstatus(cbp);
		v_lock(&zdcprlock, s_ipl);
		if (cbp->cb_state & ZD_REVECTOR) {
			/*
			 * Completed revector. Continue with rest of transfer.
			 */
			zdcontinue(cbp, up, ctlrp);
			return;
		}
		donebp = cbp->cb_bp;
		goto donext;		/* Not fatal */

	/*
	 * Fail the job immediately - no retries.
	 */
	case ZDC_DRVPROT:
	case ZDC_ECC:
		if (cbp->cb_cmd == ZDC_REC_DATA) {
			donebp = cbp->cb_bp;
			goto donext;		/* Not an error */
		}
		s_ipl = p_lock(&zdcprlock, SPLZD);
		hard_error(cbp, blkno);
		zddumpstatus(cbp);
		v_lock(&zdcprlock, s_ipl);
		break;

	case ZDC_CH_RESET:		/* Shutdown drive */
	case ZDC_BADDRV:
	case ZDC_DDC_STAT:
		s_ipl = p_lock(&zdcprlock, SPLZD);
		hard_error(cbp, blkno);
		zddumpstatus(cbp);
		v_lock(&zdcprlock, s_ipl);
		zdshutdrive(up);
		break;

	case ZDC_DMA_TO:		/* Shutdown Channel */
		s_ipl = p_lock(&zdcprlock, SPLZD);
		hard_error(cbp, blkno);
		v_lock(&zdcprlock, s_ipl);
		zdshutchan(ctlrp, up->zu_drive & 1);
		break;

	case ZDC_NOCFG:
		if (cbp->cb_cmd == ZDC_GET_CHANCFG) {
			donebp = cbp->cb_bp;
			goto donext;		/* Not an error */
		}
		/*
		 * FW assumed insane...
		 */
	case ZDC_REVECT:		/* FW hosed - shutdown controller */
	case ZDC_ILLVECIO:
	case ZDC_ILLPGSZ:
	case ZDC_ILLDUMPADR:
		s_ipl = p_lock(&zdcprlock, SPLZD);
		hard_error(cbp, blkno);
		zddumpstatus(cbp);	/* for ZDC_REVECT */
		v_lock(&zdcprlock, s_ipl);
		zdshutctlr(ctlrp);
		break;

	default:
		/*
		 * Unknown completion code - controller bad?
		 */
		s_ipl = p_lock(&zdcprlock, SPLZD);
		printf("zd%d: Bad cb_compcode 0x%x.\n",
			cbp->cb_unit, cbp->cb_compcode);
                /*
                 *+ The DCC interrupted the system with an illegal 
		 *+ completion code.
                 */
		cbp->cb_compcode = zdncompcodes - 1;	/* nice message */
		hard_error(cbp, blkno);
		v_lock(&zdcprlock, s_ipl);
		zdshutctlr(ctlrp);
		break;

	case ZDC_ILLCMD:
		s_ipl = p_lock(&zdcprlock, SPLZD);
		hard_error(cbp, blkno);
		if (cbp->cb_cmd == 0 || cbp->cb_cmd > ZDC_MAXCMD) {
			CPRINTF("zd%d: cb_cmd 0x%x corrupted.\n",
				cbp->cb_unit, cbp->cb_cmd);
			v_lock(&zdcprlock, s_ipl);
			panic("zdintr: cb corrupted");
                        /*
                         *+ The DCC interrupted the system with an illegal
                         *+ command error.  The command in the command block
                         *+ passed to the controller was
                         *+ corrupted.
                         */
		}
		v_lock(&zdcprlock, s_ipl);
		/*
		 * Cb looks ok - assume controller unreliable
		 */
		zdshutctlr(ctlrp);
		break;

	case ZDC_ILLMOD:
		s_ipl = p_lock(&zdcprlock, SPLZD);
		hard_error(cbp, blkno);
		switch(cbp->cb_cmd) {

		case ZDC_READ:
		case ZDC_READ_SS:
		case ZDC_READ_HDRS:
		case ZDC_LONG_READ:
		case ZDC_REC_DATA:
			if (cbp->cb_mod != up->zu_ioctlcb.cb_mod) {
				CPRINTF("zd%d: cb_mod 0x%x corrupted.\n",
					cbp->cb_unit, cbp->cb_mod);
				v_lock(&zdcprlock, s_ipl);
				panic("zdintr: cb corrupted");
                                /*
                                 *+ The DCC interrupted the system with an 
				 *+ illegal modifier error.  The modifier in 
				 *+ the command block passed to the controller
				 *+ was corrupted.
                                 */
			}
		}
		v_lock(&zdcprlock, s_ipl);
		/*
		 * Cb looks ok - assume controller unreliable
		 */
		zdshutctlr(ctlrp);
		break;

	case ZDC_ILLALIGN:
		s_ipl = p_lock(&zdcprlock, SPLZD);
		hard_error(cbp, blkno);
		if ((cbp->cb_addr & (ADDRALIGN - 1)) != 0) {
			CPRINTF("zd%d: cb_addr 0x%x corrupted.\n",
				cbp->cb_unit, cbp->cb_addr);
			v_lock(&zdcprlock, s_ipl);
			panic("zdintr: cb corrupted");
                        /*
                         *+ The DCC interrupted the system with an illegal
                         *+ alignment error.  The command block passed to the
                         *+ controller had incorrectly aligned addresses.
                         */
		}
		v_lock(&zdcprlock, s_ipl);
		/*
		 * Cb looks ok - assume controller unreliable
		 */
		zdshutctlr(ctlrp);
		break;

	case ZDC_ILLCNT:
		s_ipl = p_lock(&zdcprlock, SPLZD);
		hard_error(cbp, blkno);
		if (cbp->cb_count == 0 || (cbp->cb_count & (CNTMULT-1)) != 0) {
			CPRINTF("zd%d: cb_count 0x%x corrupted.\n",
				cbp->cb_unit, cbp->cb_count);
			v_lock(&zdcprlock, s_ipl);
			panic("zdintr: cb corrupted");
                        /*
                         *+ The DCC interrupted the system with an illegal
                         *+ count error.  The command block passed to the
                         *+ controller had an invalid transfer count.
                         */
		}
		v_lock(&zdcprlock, s_ipl);
		/*
		 * Cb looks ok - assume controller unreliable
		 */
		zdshutctlr(ctlrp);
		break;

	case ZDC_ILLIOV:
		s_ipl = p_lock(&zdcprlock, SPLZD);
		hard_error(cbp, blkno);
		if (((u_int)cbp->cb_iovec & (IOVALIGN - 1)) != 0) {
			CPRINTF("zd%d: cb_iovec 0x%x corrupted.\n",
				cbp->cb_unit, cbp->cb_iovec);
			v_lock(&zdcprlock, s_ipl);
			panic("zdintr: cb corrupted");
                        /*
                         *+ The DCC interrupted the system with an illegal
                         *+ iovec error.  The command block passed to the
                         *+ controller had a corrupted iovec structure.
                         */
		}
		v_lock(&zdcprlock, s_ipl);
		/*
		 * Cb looks ok - assume controller unreliable
		 */
		zdshutctlr(ctlrp);
		break;

	case ZDC_ILLCHS:
		s_ipl = p_lock(&zdcprlock, SPLZD);
		hard_error(cbp, blkno);
		dd = (up->zu_drive & 1) ? &ctlrp->zdc_chanB : &ctlrp->zdc_chanA;
		if (cbp->cb_cyl >= dd->zdd_cyls ||
		    cbp->cb_head >= dd->zdd_tracks ||
		    cbp->cb_sect >= dd->zdd_sectors + dd->zdd_spare) {
			CPRINTF("zd%d: cb_diskaddr (%d, %d, %d) corrupted.\n",
				cbp->cb_unit,
				cbp->cb_cyl, cbp->cb_head, cbp->cb_sect);
			v_lock(&zdcprlock, s_ipl);
			panic("zdintr: cb corrupted");
                        /*
                         *+ The DCC interrupted the system with an illegal
                         *+ disk address error.  The command block passed to
                         *+ the controller had an invalid disk address.
                         */
		}
		v_lock(&zdcprlock, s_ipl);
		/*
		 * Cb looks ok - assume controller unreliable
		 */
		zdshutctlr(ctlrp);
		break;

	case ZDC_CBREUSE:
		printf("zd%d: CBREUSE error cb = 0x%x\n", cbp->cb_unit, cbp);
		panic("zdc: ZDC_CBREUSE error");
                /*
                 *+ The DCC interrupted the system with an error.  A command
                 *+ block that was already in use was reused before the previous
                 *+ command completed.
                 */

	case ZDC_ACCERR:
		s_ipl = splhi();
		val = rdslave(ctlrp->zdc_slicaddr,
			      (u_char)((up->zu_drive & 1) ? SL_G_ACCERR1
							  : SL_G_ACCERR0));
		CPRINTF("zd%d: Access error on transfer starting at physical address 0x%x.\n",
				cbp->cb_unit, cbp->cb_addr);
		access_error(val);
		val = ~val;
		if (((val & SLB_ATMSK) == SLB_AEFATAL) &&
		    ((val & SLB_AEIO) != SLB_AEIO)) {
			/*
			 * Uncorrectable memory error!
			 * Clear access error to restart controller and
			 * panic the system.
			 */
			wrslave(ctlrp->zdc_slicaddr,
				(u_char)((up->zu_drive & 1) ? SL_G_ACCERR1
							    : SL_G_ACCERR0),
				(u_char)0xbb);
			panic("zdc access error");
                        /*
                         *+ The DCC had a fatal access error and was unable
                         *+ to recover.
                         */
		}
		splx(s_ipl);

		/*
		 * Shutdown the channel and restart the controller.
		 */
		zdshutchan(ctlrp, up->zu_drive & 1);
		s_ipl = splhi();
		wrslave(ctlrp->zdc_slicaddr,
			(u_char)((up->zu_drive & 1) ? SL_G_ACCERR1
						    : SL_G_ACCERR0),
			(u_char)0xbb);
		splx(s_ipl);
		break;

	case ZDC_CH_TO:			/* Retry without reset */
	case ZDC_FDL:
		s_ipl = p_lock(&zdcprlock, SPLZD);
		if (cbp->cb_errcnt++ < zdcretry) {
#ifdef	DEBUG
			zddumpcb(cbp);
#endif	DEBUG
			printf("zd%d%s: Error (%s); cmd 0x%x at (%d, %d, %d).\n",
				cbp->cb_unit, PARTCHR(cbp->cb_bp->b_dev),
				zd_compcodes[cbp->cb_compcode],
				cbp->cb_cmd, cbp->cb_cyl, cbp->cb_head,
				cbp->cb_sect);
			printf("zd%d%s: Filesystem blkno = %d.\n", cbp->cb_unit,
				PARTCHR(cbp->cb_bp->b_dev), blkno);
                        /*
                         *+ The DCC driver received the specified error
                         *+ completion status from the DCC firmware for the
                         *+ indicated drive partition.  The '%s' translates
                         *+ to the error code received.  Information about
                         *+ that error appears in this chapter.
                         *+ The final '(%d, %d, %d)' indicates the cylinder,
                         *+ head, and sector in the failing command block.
                         */
			zddumpstatus(cbp);
			v_lock(&zdcprlock, s_ipl);
			zdretry(cbp, up, ctlrp);
			return;
		}
		/*
		 * Exceeded retry count.
		 * Shutdown the channel.
		 */
		hard_error(cbp, blkno);
		zddumpstatus(cbp);
		v_lock(&zdcprlock, s_ipl);
		zdshutchan(ctlrp, up->zu_drive & 1);
		break;

	case ZDC_DRVFLT:
		/*
		 * cb_count and cb_diskaddr are unreliable on drive fault
		 * so retry entire request.
		 */
		cbp->cb_bp->b_resid = cbp->cb_bp->b_bcount - cbp->cb_transfrd;
		blkno =	((cbp->cb_bp->b_bcount - cbp->cb_bp->b_resid)
				>> DEV_BSHIFT) + cbp->cb_bp->b_blkno;
		dd = (up->zu_drive & 1) ? &ctlrp->zdc_chanB : &ctlrp->zdc_chanA;
		i = blkno;
		start = V_ALL(dev) ? 0 : up->zu_part->v_part[part].p_start;
		cbp->cb_diskaddr.da_cyl =
			i / (dd->zdd_sectors * dd->zdd_tracks) +
		        (start / (dd->zdd_sectors * dd->zdd_tracks));
		i %= (dd->zdd_sectors * dd->zdd_tracks);
		cbp->cb_diskaddr.da_head = i / dd->zdd_sectors;
		cbp->cb_diskaddr.da_sect = i % dd->zdd_sectors;
		/* Fall into */
	case ZDC_SEEKERR:	/* reset and retry */
	case ZDC_SEEK_TO:
		s_ipl = p_lock(&zdcprlock, SPLZD);
		if (cbp->cb_errcnt < zdcretry) {
#ifdef	DEBUG
			zddumpcb(cbp);
#endif	DEBUG
			printf("zd%d%s: Error (%s); cmd 0x%x at (%d, %d, %d).\n",
				cbp->cb_unit, PARTCHR(cbp->cb_bp->b_dev),
				zd_compcodes[cbp->cb_compcode],
				cbp->cb_cmd, cbp->cb_cyl, cbp->cb_head,
				cbp->cb_sect);
			printf("zd%d%s: Filesystem blkno = %d.\n", cbp->cb_unit,
				PARTCHR(cbp->cb_bp->b_dev), blkno);
                        /*
                         *+ The DCC driver received the specified error
                         *+ completion status from the DCC firmware for the
                         *+ indicated drive partition.  The '%s' translates
                         *+ to the error code received.  Information about
                         *+ that error appears in this chapter.
                         *+ The final '(%d, %d, %d)' indicates the cylinder,
                         *+ head, and sector in the failing command block.
                         */
			zddumpstatus(cbp);
			v_lock(&zdcprlock, s_ipl);
			/*
			 * reset drive
			 */
			cbp->cb_state |= ZD_RESET;
			cbp->cb_cmd = ZDC_RESET;
			NUDGE_ZDC(ctlrp, cbp, s_ipl);
			return;
		}

		/*
		 * Exceeded retry count.
		 * Shutdown the drive.
		 */
		hard_error(cbp, blkno);
		zddumpstatus(cbp);
		v_lock(&zdcprlock, s_ipl);
		zdshutdrive(up);
		break;

	case ZDC_SNF:			/* Revector request */
		if (zdrevector(cbp, up, ctlrp))
			return;
		if (cbp->cb_cmd == ZDC_REC_DATA) {
			donebp = cbp->cb_bp;
			goto donext;		/* Not an error */
		}
		goto bad;
	case ZDC_HDR_ECC:		/* Reset and retry */
		if ((up->zu_mod & ZDC_NOECC) == 0) {
			if (zdrevector(cbp, up, ctlrp))
				return;
		}
		if (cbp->cb_cmd == ZDC_REC_DATA) {
			donebp = cbp->cb_bp;
			goto donext;		/* Not an error */
		}
		/* else fall into... */
	case ZDC_SO:
	case ZDC_NDS:
bad:
		s_ipl = p_lock(&zdcprlock, SPLZD);
		if ((cbp->cb_errcnt == zdcretry) || (up->zu_mod & ZDC_NOECC)) {
			hard_error(cbp, blkno);
			v_lock(&zdcprlock, s_ipl);
			break;
		} else {
#ifdef	DEBUG
			zddumpcb(cbp);
#endif	DEBUG
			printf("zd%d%s: Error (%s); cmd 0x%x at (%d, %d, %d).\n",
				cbp->cb_unit, PARTCHR(cbp->cb_bp->b_dev),
				zd_compcodes[cbp->cb_compcode],
				cbp->cb_cmd, cbp->cb_cyl, cbp->cb_head,
				cbp->cb_sect);
			printf("zd%d%s: Filesystem blkno = %d.\n", cbp->cb_unit,
				PARTCHR(cbp->cb_bp->b_dev), blkno);
                        /*
                         *+ The DCC driver received the specified error
                         *+ completion status from the DCC firmware for the
                         *+ indicated drive partition.  The '%s' translates
                         *+ to the error code received.  Information about
                         *+ that error appears in this chapter.
                         *+ The final '(%d, %d, %d)' indicates the cylinder,
                         *+ head, and sector in the failing command block.
                         */
		}
		zddumpstatus(cbp);
		v_lock(&zdcprlock, s_ipl);
		cbp->cb_state |= ZD_RESET;
		cbp->cb_cmd = ZDC_RESET;
		NUDGE_ZDC(ctlrp, cbp, s_ipl);
		return;

	} /* end of switch */

	/*
	 * Fail this I/O request
	 */
	donebp = cbp->cb_bp;
	donebp->b_flags |= B_ERROR;
	donebp->b_error = EIO;

donext:
#ifdef	DEBUG
	if (zddebug > 2)
		zddumpcb(cbp);
#endif	DEBUG
	if (donebp->b_flags & B_IOCTL) {
		bcopy((caddr_t)cbp, (caddr_t)&up->zu_ioctlcb, FWCBSIZE);
	}
	/*
	 * If more requests - start them.
	 */
	s_ipl = p_lock(&up->zu_lock, SPLZD);
	cbp->cb_bp = NULL;
	if (up->zu_bhead.av_forw != NULL) {
		zdstart(up);
	} else {
		if (up->zu_cbptr->cb_bp == NULL
		&&  up->zu_cbptr[1].cb_bp == NULL) {
			/*
			 * Going idle.
			 * Get elapsed time and decrement I/O activity.
			 */
			if (up->zu_dkstats) {
				struct timeval elapsed;

				elapsed = time;
				timevalsub(&elapsed, &up->zu_starttime);
				timevaladd(&up->zu_dkstats->dk_time, &elapsed);
			}
			if (fp_lights) {
				s_ipl = splhi();
				FP_IO_INACTIVE;
				splx(s_ipl);
			}
		}
	}
	v_lock(&up->zu_lock, s_ipl);

	/*
	 * Gather stats
	 * Note: no attempt to mutex...
	 */
	if (up->zu_dkstats) {
		up->zu_dkstats->dk_xfer++;
		up->zu_dkstats->dk_blks +=
			(donebp->b_bcount - donebp->b_resid) >> DEV_BSHIFT;
	}
	biodone(donebp);
}

/*
 * hard_error
 *	report hard error
 */
static
hard_error(cbp, blkno)
	register struct cb *cbp;
	daddr_t	blkno;
{
#ifdef	DEBUG
	zddumpcb(cbp);
#endif	DEBUG
	printf("zd%d%s: Hard Error (%s); cmd 0x%x at (%d, %d, %d).\n",
		cbp->cb_unit, PARTCHR(cbp->cb_bp->b_dev),
		zd_compcodes[cbp->cb_compcode], cbp->cb_cmd,
		cbp->cb_cyl, cbp->cb_head, cbp->cb_sect);
	printf("zd%d%s: Filesystem blkno = %d.\n", cbp->cb_unit,
		PARTCHR(cbp->cb_bp->b_dev), blkno);
        /*
         *+ The DCC driver received an error completion status
         *+ from the DCC firmware for the indicated drive partition
         *+ and is treating it as a hard error.  This can be due to the
         *+ command reaching its retry limit or to the particular status.
         *+ The '%s' translates
         *+ to the error code received.  Information about
         *+ that error appears in this chapter.
         *+ The final '(%d, %d, %d)' indicates the cylinder,
         *+ head, and sector in the failing command block.
         */
}

/*
 * zdgetrpl()
 *	Search bad block list for replacement sector for bad sector (SNF).
 *
 * Return:
 *	pointer to replacement sector
 *	otherwise NULL pointer if no replacement sector found.
 *	
 */
static struct diskaddr *
zdgetrpl(badsect, zdp)
	register struct diskaddr *badsect;
	register struct zdbad	 *zdp;
{
	register struct bz_bad	 *bb;

	if (zdp) {
		bb = zdp->bz_bad;
		for (bb = zdp->bz_bad; bb < &zdp->bz_bad[zdp->bz_nsnf]; bb++) {
			if (bb->bz_cyl < badsect->da_cyl)
				continue;
			if (bb->bz_cyl > badsect->da_cyl)
				break;
			/* cylinder matched */
			if (bb->bz_head < badsect->da_head)
				continue;
			if (bb->bz_head > badsect->da_head)
				break;
			/* head matched */
			if (bb->bz_sect == badsect->da_sect)
				return (&bb->bz_rpladdr);
		}
	}
	return (NULL);
}

/*
 * zdrevector
 *	Attempt to revector the sector-not-found.
 * If the sector-not-found is in the bad block list perform the I/O on
 * its replacement sector. Then continue with the following sector if
 * necessary.
 * 
 * Return values:
 *	 0 - No replacement sector in bad block list.
 *	 1 - Replacement sector found in bad block list.
 */
static int
zdrevector(cbp, up, ctlrp)
	register struct cb *cbp;
	struct	zd_unit	*up;
	struct zdc_ctlr *ctlrp;
{
	register struct zdcdd	 *dd;		/* disk description */
	register struct diskaddr *replcmnt;	/* replacement address */
	register int	count;			/* # bytes of iovecs */
	int	transfrd;			/* bytes already transferred */
	int	cbcount;
	u_long	*from;
	spl_t	s_ipl;

#ifdef	DEBUG
	if (zddebug)
		printf("zd%d: revectoring (%d, %d, %d)", cbp->cb_unit,
			cbp->cb_cyl, cbp->cb_head, cbp->cb_sect);
#endif	DEBUG
	/*
	 * Determine if bad block is in bad block list.
	 */
	replcmnt = zdgetrpl(&cbp->cb_diskaddr, up->zu_zdbad);
	if (replcmnt == NULL) {
		/*
		 * Not in bad block list. Caller should retry
		 * bad block.
		 */
#ifdef	DEBUG
		if (zddebug)
			printf(": not in table.\n");
#endif	DEBUG
		return (0);
	}

	/*
	 * Save state of current request, do new request for replacement.
	 */
	transfrd = cbp->cb_bp->b_bcount - cbp->cb_bp->b_resid;
	/* adjust for previously transferred sectors */
	transfrd -= cbp->cb_transfrd;
	from = cbp->cb_iovstart
			+ (((cbp->cb_addr & CLOFSET) + transfrd) >> CLSHIFT);
	cbp->cb_transfrd += transfrd;
	cbp->cb_addr += transfrd;

	/*
	 * figure number of iovecs to copy down.
	 */
	count = 0;
	cbcount = cbp->cb_count;
	if (cbp->cb_addr & CLOFSET) {
		++count;
		cbcount -= CLBYTES - (cbp->cb_addr & CLOFSET);
	}
	count += (cbcount + CLOFSET) >> CLSHIFT;
	count *= sizeof(u_long *);
	cbp->cb_iovec = cbp->cb_iovstart;
	bcopy((caddr_t)from, (caddr_t)cbp->cb_iovstart, (unsigned)count);

	dd = (up->zu_drive & 1) ? &ctlrp->zdc_chanB : &ctlrp->zdc_chanA;
	if (cbp->cb_count != DEV_BSIZE) {
		cbp->cb_state |= ZD_REVECTOR;
		if (++cbp->cb_sect == dd->zdd_sectors) {
			cbp->cb_sect = 0;
			if (++cbp->cb_head == dd->zdd_tracks) {
				cbp->cb_head = 0;
				cbp->cb_cyl++;
			}
		}
		*(int *)&cbp->cb_contaddr = *(int *)&cbp->cb_diskaddr;
		cbp->cb_contiovsz = count;
	}
	cbp->cb_count = DEV_BSIZE;
	cbp->cb_diskaddr = *replcmnt;
	cbp->cb_psect = zdgetpsect(&cbp->cb_diskaddr, dd);
#ifdef	DEBUG
	if (zddebug)
		printf(" to (%d, %d, %d).\n",
			cbp->cb_cyl, cbp->cb_head, cbp->cb_sect);
#endif	DEBUG

	/*
	 * Nudge controller.
	 */
	NUDGE_ZDC(ctlrp, cbp, s_ipl);
	return (1);
}

/*
 * zdcontinue
 *	Continue rest of I/O request after the revectored sector.
 */
static
zdcontinue(cbp, up, ctlrp)
	register struct cb	 *cbp;
	struct zd_unit *up;
	register struct	zdc_ctlr *ctlrp;
{
	u_long	*from;
	spl_t	s_ipl;

	/*
	 * Revectoring completed successfully.
	 * Restart rest of I/O request.
	 */
	cbp->cb_state &= ~ZD_REVECTOR;
	cbp->cb_bp->b_resid -= DEV_BSIZE;
	cbp->cb_transfrd += DEV_BSIZE;
	cbp->cb_errcnt = 0;
	cbp->cb_count = cbp->cb_bp->b_resid;
	cbp->cb_iovec = cbp->cb_iovstart;
	from = cbp->cb_iovstart
			+ (((cbp->cb_addr & CLOFSET) + DEV_BSIZE) >> CLSHIFT);
	cbp->cb_addr += DEV_BSIZE;
	if (from != cbp->cb_iovstart) {
		/*
		 * Must start with next iovector.
		 */
		bcopy((caddr_t)from, (caddr_t)cbp->cb_iovstart,
			(unsigned)(cbp->cb_contiovsz - sizeof(u_long *)));
	}
	*(int *)&cbp->cb_diskaddr = *(int *)&cbp->cb_contaddr;
	cbp->cb_psect = zdgetpsect(&cbp->cb_diskaddr,
					(up->zu_drive & 1) ? &ctlrp->zdc_chanB
							   : &ctlrp->zdc_chanA);
	/*
	 * Nudge controller.
	 */
	NUDGE_ZDC(ctlrp, cbp, s_ipl);
}

/*
 * zdretry
 *	Retry the request.
 * If the request is a normal read/write request then retry the command
 * from where the error occurred. If B_IOCTL request, retry from the
 * initial CB.
 */
static
zdretry(cbp, up, ctlrp)
	register struct cb *cbp;
	struct	zd_unit  *up;
	struct	zdc_ctlr *ctlrp;
{
	register int count;		/* # iovecs remaining */
	register int transfrd;		/* # bytes transferred */
	register int cbcount;
	u_long	*from;			/* 1st retry iovec */
	spl_t	s_ipl;

	/*
	 * Retry job
	 */
	if (cbp->cb_bp->b_flags & B_IOCTL) {
		/*
		 * copy 1st half of cb (what fw will see)
		 */
		bcopy((caddr_t)&up->zu_ioctlcb, (caddr_t)cbp, FWCBSIZE);
	} else {
		cbp->cb_count = cbp->cb_bp->b_resid;
		cbp->cb_cmd = (cbp->cb_bp->b_flags & B_READ) ? ZDC_READ : ZDC_WRITE;
		cbp->cb_sect &= ~ZD_AUTOBIT;	 /* drop Auto-revetor bit */
		cbp->cb_psect = zdgetpsect(&cbp->cb_diskaddr,
					(up->zu_drive & 1) ? &ctlrp->zdc_chanB
							   : &ctlrp->zdc_chanA);
		transfrd = cbp->cb_bp->b_bcount - cbp->cb_count;
		transfrd -= cbp->cb_transfrd;
		from = cbp->cb_iovstart
			+ (((cbp->cb_addr & CLOFSET) + transfrd) >> CLSHIFT);
		cbp->cb_transfrd += transfrd;
		cbp->cb_addr += transfrd;

		/*
		 * determine number of iovecs remaining.
		 */
		count = 0;
		cbcount = cbp->cb_count;
		if (cbp->cb_addr & CLOFSET) {
			++count;
			cbcount -= CLBYTES - (cbp->cb_addr & CLOFSET);
		}
		count += (cbcount + CLOFSET) >> CLSHIFT;

		/*
		 * Copy down iovecs to 32 byte aligned boundary.
		 * That is, cb_iovstart.
		 */
		bcopy((caddr_t)from, (caddr_t)cbp->cb_iovstart,
			(unsigned)(count * sizeof(u_long *)));
	}
	cbp->cb_iovec = cbp->cb_iovstart;

	/*
	 * Nudge controller.
	 */
	NUDGE_ZDC(ctlrp, cbp, s_ipl);
}

/*
 * zdc_cb_cleanup
 *	Cleanup (cancel) jobs active on disabled controller.
 * Called as timeout routine to allow completed CB's to drain.
 * That is, those for which a completion interrupt was sent just
 * before the error interrupt occurred.
 */
zdc_cb_cleanup(ctlrp)
	register struct zdc_ctlr *ctlrp;
{
	register struct cb	 *cbp;
	register struct buf	 *bp;
	spl_t	s_ipl;

	/*
	 * Controller dead. Fail all active I/O requests.
	 */
	cbp = ctlrp->zdc_cbp;
	for (; cbp < &ctlrp->zdc_cbp[NCBPERZDC]; cbp++) {
		if (cbp->cb_bp != NULL) {
			bp = cbp->cb_bp;
			bp->b_flags |= B_ERROR;
			bp->b_error = EIO;
			bp->b_resid = bp->b_bcount;
			biodone(bp);
			bp = NULL;
			/*
			 * Decrement I/O activity
			 */
			if (fp_lights) {
				s_ipl = splhi();
				FP_IO_INACTIVE;
				splx(s_ipl);
			}
		}
	}
}

/*
 * zdc_error
 *	Controller error interrupt.
 */
zdc_error(vector)
	u_char	vector;
{
	register int	val;
	register struct zdc_ctlr *ctlrp;
	spl_t	s_ipl;

	ctlrp = &zdctrlr[vector - base_err_intr];
	s_ipl = splhi();	/* In case zdc_err_bin not splhi() */
	val = rdslave(ctlrp->zdc_slicaddr, SL_Z_STATUS);
	splx(s_ipl);
	if (((val & SLB_ZPARERR) != SLB_ZPARERR) &&
	    ((val & ZDC_READY) == ZDC_READY)) {
		printf("zdc%d - stray error interrupt!\n", ctlrp - zdctrlr);
                /*
                 *+ The DCC driver received a stray controller
                 *+ error interrupt.  The interrupt is ignored.
                 */
		return;
	}
	printf("zdc%d: controller interrupt - SL_Z_STATUS == 0x%x.\n",
		ctlrp - zdctrlr, val);
        /*
         *+ The DCC driver received a controller error interrupt.
         *+ The driver will shut down the controller.
         */

	if ((val & ZDC_ERRMASK) == ZDC_OBCB) {
                /*
                 * Assume stray interrupt happened and tell fw to continue.
                 */
		s_ipl = splhi();	/* in case zdc_err_bin not splhi() */
		mIntr(ctlrp->zdc_slicaddr, CLRERRBIN, 0xbb);
		splx(s_ipl);
		return;
	}

	splx(SPLZD);		/* Do the rest at SPLZD */

	/*
	 * Fail all queued I/O requests.
	 */
	zdshutctlr(ctlrp);

	/*
	 * Allow completed CBs to drain then
	 * fail all other "active" I/O requests.
	 */
	timeout(zdc_cb_cleanup, (caddr_t)ctlrp, 5 * hz);
}

/*
 * zdshutctlr
 *	Fail all I/O requests queued to the drives on this controller.
 *	Currently active I/O requests will finish asyncronously.
 *	The controller is marked as ZDC_DEAD and all units on the
 *	controller are marked as ZU_BAD.
 */
static
zdshutctlr(ctlrp)
	register struct zdc_ctlr *ctlrp;
{
	register struct cb *cbp;

	printf("zdc%d: Controller disabled.\n", ctlrp - zdctrlr);
        /*
         *+ The DCC driver is disabling the specified controller.
         */

	cbp = ctlrp->zdc_cbp;
	for (; cbp < &ctlrp->zdc_cbp[NCBPERZDC]; cbp += NCBPERDRIVE) {
		if (cbp->cb_unit >= 0)
			zdshutdrive(&zdunit[cbp->cb_unit]);
	}
	ctlrp->zdc_state = ZDC_DEAD;
}

/*
 * zdshutchan
 *	Fail all I/O requests queued to the drives on this channel.
 *	Currently active I/O requests will finish asyncronously.
 *	All units on the channel are marked as ZU_BAD.
 */
static
zdshutchan(ctlrp, channel)
	register struct zdc_ctlr *ctlrp;
	int	channel;		/* 0 == Channel A, 1 == channel B */
{
	register struct cb *cbp;

	printf("zdc%d: Channel %c disabled.\n", ctlrp - zdctrlr,
			(channel == 0) ? 'A' : 'B');
        /*
         *+ The DCC driver is disabling the specified channel.
         */

	cbp = &ctlrp->zdc_cbp[channel * NCBPERDRIVE];
	for (; cbp < &ctlrp->zdc_cbp[NCBPERZDC]; cbp += 2 * NCBPERDRIVE) {
		if (cbp->cb_unit >= 0)
			zdshutdrive(&zdunit[cbp->cb_unit]);
	}
}

/*
 * zdshutdrive
 *	Fail all I/O requests queued to the drive.
 *	Currently active I/O requests will finish asyncronously.
 *	The drive is marked as ZU_BAD.
 */
static
zdshutdrive(up)
	register struct	zd_unit *up;
{
	register struct	buf	*bp, *nextbp;
	spl_t	s_ipl;

	/*
	 * Lock the device and fail all jobs currently queued.
	 */
	s_ipl = p_lock(&up->zu_lock, SPLZD);
	if (up->zu_state == ZU_BAD) {
		v_lock(&up->zu_lock, s_ipl);
		return;
	}
	printf("zd%d: Drive disabled.\n", up - zdunit);
        /*
         *+ The DCC driver is disabling the specified drive.
         */
	bp = up->zu_bhead.av_forw;
	while (bp != NULL) {
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		bp->b_resid = bp->b_bcount;
		nextbp = bp->av_forw;
		biodone(bp);
		bp = nextbp;
	}
	up->zu_bhead.av_forw = NULL;
	up->zu_state = ZU_BAD;
	v_lock(&up->zu_lock, s_ipl);
	/*
	 * Turn on error light on front panel.
	 */
	if (fp_lights) {
		s_ipl = splhi();
		FP_IO_ERROR;
		splx(s_ipl);
	}
	/*
	 * It had been online and usable.
	 */
	disk_offline();
}

/*
 * zdminphys - correct for too large a request.
 *
 * Note correction for non-cluster-aligned transfers.
 */
zdminphys(bp)
	struct buf *bp;
{
	if (bp->b_bcount > ((zdc_iovpercb - 1) * CLBYTES))
		bp->b_bcount = (zdc_iovpercb - 1) * CLBYTES;
}

zdread(dev, uio)
	dev_t	dev;
	struct uio *uio;
{
	register struct zd_unit *up;	/* unit structure */
	int err, diff;
	off_t lim;

	up = &zdunit[VUNIT(dev)];
	if ((up->zu_flags & ZUF_FORMATTED) || V_ALL(dev))
		return (physio(zdcstrat, (struct buf *)0, dev, B_READ, zdminphys, uio));
	lim = up->zu_part->v_part[VPART(dev)].p_size;
	lim <<= DEV_BSHIFT;
	err = physck(lim, uio, B_READ, &diff);
	if (err != 0) {
		if (err == -1)	/* not an error, but request of 0 bytes */
			err = 0;
		return (err);
	}
	err = physio(zdstrat, (struct buf *)0, dev, B_READ, zdminphys, uio);
	uio->uio_resid += diff;
	return (err);
}

zdwrite(dev, uio)
	dev_t	dev;
	struct uio *uio;
{
	register struct zd_unit *up;	/* unit structure */
	int err, diff;
	off_t lim;

	up = &zdunit[VUNIT(dev)];
	if ((up->zu_flags & ZUF_FORMATTED) || V_ALL(dev))
		return (physio(zdcstrat, (struct buf *)0, dev, B_WRITE, zdminphys, uio));
	lim = up->zu_part->v_part[VPART(dev)].p_size;
	lim <<= DEV_BSHIFT;
	err = physck(lim, uio, B_WRITE, &diff);
	if (err != 0) {
		if (err == -1)	/* not an error, but request of 0 bytes */
			err = 0;
		return (err);
	}
	err = physio(zdstrat, (struct buf *)0, dev, B_WRITE, zdminphys, uio);
	uio->uio_resid += diff;
	return (err);
}

/*
 * zdcstrat
 * 	zd whole disk read/write routine.
 * queue request and call start routine to
 * initiate I/O to the device.
 */
zdcstrat(bp)
	register struct buf *bp;
{
	register struct zd_unit *up;
	register struct zdcdd	*dd;		/* channel configuration */
	register int sector;
	register int nspc;
	struct	diskaddr diskaddress;
	spl_t s_ipl;

#ifdef	DEBUG
	if (zddebug > 1)
		printf("zdcstrat(%c): bp=%x, dev=%x, cnt=%d, blk=%x, vaddr=%x\n",
			(bp->b_flags & B_READ) ? 'R' : 'W',
			bp, bp->b_dev, bp->b_bcount, bp->b_blkno,
			bp->b_un.b_addr);
	else if (zddebug)
			printf("%c", (bp->b_flags & B_READ) ? 'R' : 'W');
#endif	DEBUG

	up = &zdunit[VUNIT(bp->b_dev)];
	/*
	 * Error if NO_RW operations are permitted.
	 */
	if ((up->zu_state == ZU_NO_RW) && !(bp->b_flags & B_IOCTL)) {
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		bp->b_resid = bp->b_bcount;
		biodone(bp);
		return;
	}
	dd = (up->zu_drive & 1)	? &zdctrlr[up->zu_ctrlr].zdc_chanB
				: &zdctrlr[up->zu_ctrlr].zdc_chanA;
	nspc = dd->zdd_sectors * dd->zdd_tracks;

	/*
	 * Fail request if bogus byte count, if address not aligned to
	 * ADDRALIGN boundary
	 */
	if (bp->b_bcount <= 0
	||  ((bp->b_bcount & (DEV_BSIZE -1)) != 0)		/* size */
	||  ((bp->b_iotype == B_RAWIO) &&
	     (((int)bp->b_un.b_addr & (ADDRALIGN - 1)) != 0))	/* alignment */
	||  (bp->b_blkno < 0)
	||  (bp->b_blkno >= up->zu_drive_size)) {		/* partition */ 
		bp->b_flags |= B_ERROR;
		bp->b_error = EINVAL;
		bp->b_resid = bp->b_bcount;
		biodone(bp);
		return;
	}

	sector = bp->b_blkno;
	diskaddress.da_cyl = sector / nspc ;
	sector %= nspc;
	diskaddress.da_head = sector / dd->zdd_sectors;
	diskaddress.da_sect = sector % dd->zdd_sectors;
	bp->b_diskaddr = *(long *)&diskaddress;
	bp->b_psect = zdgetpsect(&diskaddress, dd);

	s_ipl = p_lock(&up->zu_lock, SPLZD);
	if (up->zu_state == ZU_BAD) {
		v_lock(&up->zu_lock, s_ipl);
		/*
		 * Controller/channel/Drive has gone bad!
		 */
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		bp->b_resid = bp->b_bcount;
		biodone(bp);
		return;
	}
	disksort(&up->zu_bhead, bp);
	up->zu_flags |= ZUF_ACTIVE;
	zdstart(up);
	v_lock(&up->zu_lock, s_ipl);
}

/*
 * zdioctl
 *	This routine provides support for various IOCTLs,
 *	most of which are used to support online formatting.
 *	Those which change the configuration or state of the device are only 
 *	accessible to the superuser.
 */
/*ARGSUSED*/
zdioctl(dev, cmd, data, flag)
	dev_t	dev;
	int	cmd;
	caddr_t data;
	int	flag;
{
	register struct cb *cbp;	/* cb argument */
	register struct buf *bp;	/* ioctl buffer */
	register struct zd_unit *up;	/* unit structure */
	register struct zd_unit *tup;	/* temporary unit structure address */
	struct zddev *zddev;
	int	error;
	u_char ustate;
	struct zdcdd *dd;
	struct zdc_ctlr *ctlrp;
	int setokay = 0;
	int chan, i;
	spl_t	s_ipl;
	struct vtoc *vtp;

	up = &zdunit[VUNIT(dev)];
	bp = &up->zu_ioctl;
	dd = (up->zu_drive & 1) ? &zdctrlr[up->zu_ctrlr].zdc_chanB
				: &zdctrlr[up->zu_ctrlr].zdc_chanA;

	switch(cmd) {

	case RIOFIRSTSECT:	/* get first user-usable sector address */
		if ((up->zu_state != ZU_GOOD)
		||  ((up->zu_cfg & (ZD_FORMATTED|ZD_MATCH)) != (ZD_FORMATTED|ZD_MATCH))
		||  up->zu_firstsect == 0)
			return(EIO);
		*(int *)data = (int) up->zu_firstsect;
		return(0);

	case RIODRIVER:		/* success indicates we're a driver! */
		return(0);

	case V_READ:		/* Read VTOC info to user */
		vtp = *(struct vtoc **)data;
#ifdef DEBUG
		if (zddebug)
			printf("copying out to user:0x%x\n", vtp);
#endif
		if (!readdisklabel(dev, zdcstrat, up->zu_part, 
				 (struct cmptsize *)0, up->zu_firstsect))
			return(EINVAL);
		return( copyout((caddr_t)up->zu_part, (caddr_t)vtp,
						sizeof(struct vtoc)));

	case V_WRITE:		/* Write user VTOC to disk */
		vtp = (struct vtoc *)wmemall(sizeof(struct vtoc), 1);
		if (copyin(*((caddr_t *)data), (caddr_t)vtp, 
							  sizeof(struct vtoc))) {
			wmemfree((caddr_t)vtp, sizeof(struct vtoc));
			return(EFAULT);
		}
		if (!readdisklabel(dev, zdcstrat, up->zu_part, 
			   zdparts[up->zu_drive_type], up->zu_firstsect)) {
			wmemfree((caddr_t)vtp, sizeof(struct vtoc));
			return(EINVAL);
		}
		if (error = setdisklabel(up->zu_part, vtp, up->zu_opens)) {
			wmemfree((caddr_t)vtp, sizeof(struct vtoc));
			return ((error > 0) ? error : EINVAL);
		}
		wmemfree((caddr_t)vtp, sizeof(struct vtoc));
		return(writedisklabel(dev, zdcstrat, up->zu_part, 
						up->zu_firstsect));

	case V_PART:		/* Read partition info to user */
		vtp = *(struct vtoc **)data;
		if (!readdisklabel(dev, zdcstrat, up->zu_part, 
				 zdparts[up->zu_drive_type], up->zu_firstsect))
			return(EINVAL);
		return( copyout((caddr_t)up->zu_part, (caddr_t)vtp,
						sizeof(struct vtoc)));

	case ZIOSEVERE:			/* turn on severe burn-in mode */
#ifdef DEBUG
		if (zddebug)
			printf("zdioctl: ZIOSEVERE\n");
#endif DEBUG
		if (!suser() || !(up->zu_flags & ZUF_EXCLUSIVE))
			return(EACCES);
		/*
		 * Inhibit ZDC ECC correction
		 */
		up->zu_mod |= ZDC_NOECC;
		return(0);

	case ZIONSEVERE:		/* turn off severe burn-in mode */
#ifdef DEBUG
		if (zddebug)
			printf("zdioctl: ZIONSEVERE\n");
#endif DEBUG
		if (!suser())
			return(EACCES);
		up->zu_mod &= ~ZDC_NOECC;
		return(0);
	
	case ZIODEVDATA:		/* return device-specific data */
#ifdef DEBUG
		if (zddebug)
			printf("zdioctl: ZIODEVDATA\n");
#endif DEBUG
		zddev = (struct zddev *) data;
		zddev->zd_drive = up->zu_drive;
		zddev->zd_ctlr = up->zu_ctrlr;
		zddev->zd_cfg = up->zu_cfg;
		zddev->zd_state = up->zu_state;
		return(0);

	case ZIOSETSTATE:		/* set device state */
#ifdef DEBUG
		if (zddebug)
			printf("zdioctl: ZIOSETSTATE\n");
#endif DEBUG
		if (!suser() || !(up->zu_flags & ZUF_EXCLUSIVE))
			return(EACCES);
		ustate = *data;
		if ((int)ustate < 0 || (int)ustate > ZUMAXSTATE) {
#ifdef DEBUG
			if (zddebug)
				printf("zdioctl: bad state %d\n", ustate);
#endif DEBUG
			return(EIO);
		}
		up->zu_state = ustate;
		return(0);
		
	case ZIOGERR:			/* return last completion status */
#ifdef DEBUG
		if (zddebug)
			printf("zdioctl: ZIOGERR\n");
#endif DEBUG
		*data = up->zu_l_compcode;
		return(0);

	case ZIOSETBBL:			/* get bad block list from userland */
		{
		struct zdbad *zdp, *tzdp;
		struct bz_bad *fbzp, *tbzp;
		int size, zdpsize;
		caddr_t addr;

#ifdef DEBUG
		if (zddebug)
			printf("zdioctl: ZIOSETBBL\n");
#endif DEBUG
		if (!suser() || !(up->zu_flags & ZUF_EXCLUSIVE))
			return(EACCES);
		/*
		 * First get just first block to determine how much
		 * space we need.
		 */
		zdp = (struct zdbad *)wmemall(DEV_BSIZE, 1);
		if (copyin(*((caddr_t *)data), (caddr_t)zdp, DEV_BSIZE)) {
			wmemfree((caddr_t)zdp, DEV_BSIZE);
			return(EFAULT);
		}
		size = (zdp->bz_nelem * sizeof(struct bz_bad))
			+ sizeof(struct zdbad) - sizeof(struct bz_bad);
		size = howmany(size, DEV_BSIZE);
		if (size > ((dd->zdd_sectors - ZDD_NDDSECTORS) >> 1)) {
			/*
			 * Bad block list too big -- must be invalid! 
			 */
#ifdef DEBUG
			if (zddebug)
				printf("zdioctl: SETBBL bbl too big\n");
#endif /* DEBUG */
			wmemfree((caddr_t)zdp, DEV_BSIZE);
			return(EIO);
		}
		/*
		 * Allocate full size, copy 1st block to it, then
		 * copy remainder from userland.
		 */
		zdpsize = size * DEV_BSIZE;
		addr = wmemall(zdpsize, 1);
		bcopy((caddr_t)zdp, addr, (unsigned)DEV_BSIZE);
		wmemfree((caddr_t)zdp, DEV_BSIZE);
		zdp = (struct zdbad *)addr;
		if (copyin(*((caddr_t *)data)+DEV_BSIZE, (caddr_t)addr+DEV_BSIZE,
			   (u_int)(zdpsize-DEV_BSIZE))) {
			wmemfree((caddr_t)zdp, zdpsize);
			return(EFAULT);
		}
		/*
		 * Confirm data integrity via checksum.
		 */
		size = (zdp->bz_nelem * sizeof(struct bz_bad)) / sizeof(long);
		if (zdp->bz_csn != (i=getchksum((long *)zdp->bz_bad, size,
				(long)(zdp->bz_nelem ^ zdp->bz_nsnf)))) {
#ifdef DEBUG
			if (zddebug)
				printf("zdioctl: SETBBL bad checksum 0x%x 0x%x\n",
				zdp->bz_csn,i);
#endif /* DEBUG */
			wmemfree((caddr_t)zdp, zdpsize);
			return(EIO);
		}
		/*
		 * Copy only BZ_SNF entries into unit's bad block list.
		 */
		size = (zdp->bz_nsnf * sizeof(struct bz_bad))
			+ sizeof(struct zdbad) - sizeof(struct bz_bad);
		size = roundup(size, DEV_BSIZE);
		tzdp = (struct zdbad *)wmemall(size, 1);
#ifdef DEBUG
		bzero((caddr_t)tzdp, (u_int)size);
#endif DEBUG
		*tzdp = *zdp;
		tbzp = tzdp->bz_bad;
		for (fbzp = zdp->bz_bad; fbzp < &zdp->bz_bad[zdp->bz_nelem];
		     fbzp++) {
			if (fbzp->bz_rtype == BZ_SNF)
				*tbzp++ = *fbzp;
		}
#ifdef DEBUG
		if (zddebug) {
			printf("SETBBL %d entries passed allocating %d bytes\n",zdp->bz_nsnf,size);
			printf("SETBBL old bbl\n");
			zd_printbbl(up);
		}
#endif /* DEBUG */
		wmemfree((caddr_t)zdp, zdpsize);
		if (up->zu_zdbad) {
			/*
			 * Free memory used for former bad block list.
			 */
			zdp = up->zu_zdbad;
			size = (zdp->bz_nsnf * sizeof(struct bz_bad))
				+ sizeof(struct zdbad) - sizeof(struct bz_bad);
			size += roundup(size, DEV_BSIZE);
			wmemfree((caddr_t)zdp, size);
		}

		up->zu_zdbad = tzdp;
#ifdef DEBUG
		if (zddebug) {
			printf("SETBBL new bbl\n");
			zd_printbbl(up);
		}
#endif /* DEBUG */
		return(0);
		}
	
	case ZIOCBCMD:
		cbp = (struct cb *)data;
		switch(cbp->cb_cmd) {

		case ZDC_SET_CHANCFG:	/* set disk channel configuration */
#ifdef DEBUG
			if (zddebug)
				printf("zdioctl: ZDC_SET_CHANCFG\n");
#endif DEBUG
			if (!suser() || !(up->zu_flags & ZUF_EXCLUSIVE))
				return(EACCES);
			/*
			 * Grab lock and determine if SET_CHANCFG is
			 * legal.  It is only allowed if the channel
			 * configuration is not set, or if this drive
			 * is the only formatted drive on the
			 * channel.
			 */
			ctlrp = &zdctrlr[up->zu_ctrlr];
			s_ipl = p_lock(&ctlrp->zdc_ctlrlock, SPLHI);

			/*
			 * setokay is both a flag and a state mechanism.
			 * As a flag, 0 means SET_CHANCFG not permitted,
			 * non-zero means permitted.
			 * As state:
			 *	0 - SET_CHANCFG not done
			 *	1 - SET_CHANCFG done, chancfg previously set
			 *     -1 - SET_CHANCFG done, chancfg not previously set
			 */
			setokay = 1;
			chan = up->zu_drive & 1;
			if (ctlrp->zdc_chan_owner[chan] == up) {
				/*
				 * Channel has been reserved by
				 * this drive for exclusive use.
				 * Let it go.  It must also take
				 * responsibility for restoring.
				 */
				dd->zdd_sectors = -1;
				setokay = -1;
			} else if (dd->zdd_sectors == 0) {
				/*
				 * Channel configuration isn't set.
				 * Use zdd_sectors as a flag to lock
				 * out any other device attempting to
				 * SET_CHANCFG while this one is in
				 * progress.
				 */
				dd->zdd_sectors = -1;
				setokay = -1;
			} else if (dd->zdd_sectors == (unsigned) -1) {
				setokay = 0;
			} else if ((up->zu_cfg & (ZD_FORMATTED|ZD_MATCH))
				 != (ZD_FORMATTED|ZD_MATCH)) {
				setokay = 0;
			} else {
				for (i = (up->zu_drive+2)%(ZDC_MAXDRIVES/2);
				     i != up->zu_drive;
				     i = (i + 2)%(ZDC_MAXDRIVES/2)) {
					if ((ctlrp->zdc_drivecfg[i]
					    & (ZD_FORMATTED|ZD_MATCH))
					    == (ZD_FORMATTED|ZD_MATCH)) {
						setokay = 0;
					}
				}
			}
			v_lock(&ctlrp->zdc_ctlrlock, s_ipl);
			if (!setokay)
				return(EACCES);
			/* fall through */
		case ZDC_GET_CHANCFG:
			if (cbp->cb_count != sizeof(struct zdcdd) ||
			    (cbp->cb_addr & (ADDRALIGN-1)) != 0) {
				if (setokay < 0)
					dd->zdd_sectors = 0;
				return (EINVAL);
			}
			if (!useracc((char *)cbp->cb_addr,
						(u_int)cbp->cb_count, B_WRITE))
				return (EFAULT);
			bufalloc(bp);
			bp->b_flags = B_READ;
			bp->b_bcount = cbp->cb_count;
			bp->b_un.b_addr = (caddr_t)cbp->cb_addr;
			break;

		case ZDC_WRITE_SS:	/* write special sector */
#ifdef DEBUG
			if (zddebug)
				printf("zdioctl: ZDC_WRITE_SS\n");
#endif DEBUG
			if (!suser() || !(up->zu_flags & ZUF_EXCLUSIVE))
				return(EACCES);
			/* fall through */
		case ZDC_READ_SS:	/* read special sector */
#ifdef DEBUG
			if (zddebug && (cbp->cb_cmd == ZDC_READ_SS))
				printf("zdioctl: ZDC_READ_SS\n");
#endif DEBUG
			if (dd->zdd_sectors == 0) {
#ifdef DEBUG
				if (zddebug)
					printf("no READ_SS - zdd_sectors is 0\n");
#endif DEBUG
				return(EACCES);
			}
			if (cbp->cb_count != ZDD_SS_SIZE ||
			    (cbp->cb_addr & (ADDRALIGN-1)) != 0 ||
			    cbp->cb_diskaddr.da_cyl != 0 ||
			    cbp->cb_diskaddr.da_head != 0 ||
			    cbp->cb_diskaddr.da_sect >= ZDD_NDDSECTORS)
				return (EINVAL);
			if (!useracc((char *)cbp->cb_addr,
						(u_int)cbp->cb_count, B_WRITE))
				return (EFAULT);
			bufalloc(bp);
			bp->b_flags = (cbp->cb_cmd == ZDC_READ_SS) ? B_READ
					: B_WRITE;
			bp->b_bcount = cbp->cb_count;
			bp->b_un.b_addr = (caddr_t)cbp->cb_addr;
			break;

		case ZDC_FMT_SS:	/* format special sector */
#ifdef DEBUG
			if (zddebug)
				printf("zdioctl: ZDC_FMT_SS\n");
#endif DEBUG
			if (!suser() || !(up->zu_flags & ZUF_EXCLUSIVE)
				     || dd->zdd_sectors == 0) {
#ifdef DEBUG
				if (zddebug && (dd->zdd_sectors == 0))
					printf("FMT_SS err: zdd_sectors is 0\n");
#endif DEBUG
				return(EACCES);
			}	
			if (cbp->cb_count != CNTMULT 
			    || (cbp->cb_addr & (ADDRALIGN-1)) != 0
			    || cbp->cb_diskaddr.da_cyl != 0
			    || cbp->cb_diskaddr.da_head != 0
			    || cbp->cb_diskaddr.da_sect >= ZDD_NDDSECTORS
			    || cbp->cb_iovec !=0)
				return (EINVAL);
			if (!useracc((char *)cbp->cb_addr,
						(u_int)cbp->cb_count, B_WRITE))
				return (EFAULT);
			bufalloc(bp);
			bp->b_flags = B_WRITE;
			bp->b_bcount = cbp->cb_count;
			bp->b_un.b_addr = (caddr_t)cbp->cb_addr;
			break;

		case ZDC_WHDR_WDATA:	/* write sector header data */
		case ZDC_REC_DATA:	/* recover data from sector */
		case ZDC_READ:		/* (normal) read */
		case ZDC_WRITE:		/* (normal) write */
			if (!suser() || !(up->zu_flags & ZUF_EXCLUSIVE)
				     || dd->zdd_sectors == 0) {
#ifdef DEBUG
				if (zddebug && (dd->zdd_sectors == 0))
					printf("WHDR (cmd = 0x%x) - zdd_sectors is 0\n",
						cbp->cb_cmd);
#endif DEBUG
				return(EACCES);
			}
			if (cbp->cb_cmd == ZDC_WHDR_WDATA) {
				if (cbp->cb_count != (CNTMULT+dd->zdd_ddc_regs.dr_sector_bc))
					return(EINVAL);
			} else {
				if (cbp->cb_count & (DEV_BSIZE-1))
					return(EINVAL);
			}
			if ((cbp->cb_addr & (ADDRALIGN-1))
			    || cbp->cb_cyl >= dd->zdd_cyls
			    || cbp->cb_head >= dd->zdd_tracks
			    || cbp->cb_sect >= (dd->zdd_sectors + dd->zdd_spare)
			    || cbp->cb_iovec != 0) {
#ifdef DEBUG
				if (zddebug)
					printf("bad READ/WRITE/REC_DATA\n");
#endif DEBUG
				return(EINVAL);
			}
			if (!useracc((caddr_t)cbp->cb_addr,
				     (u_int)cbp->cb_count, B_WRITE))
				return(EFAULT);
			bufalloc(bp);
			if (cbp->cb_cmd == ZDC_WRITE 
			    || cbp->cb_cmd == ZDC_WHDR_WDATA)
				bp->b_flags = B_WRITE;
			else
				bp->b_flags = B_READ;
			bp->b_bcount = cbp->cb_count;
			bp->b_un.b_addr = (caddr_t)cbp->cb_addr;
			break;

		case ZDC_READ_HDRS:	/* read all sector headers */
			if (!suser())
				return(EACCES);
			else if (dd->zdd_sectors == 0) {
#ifdef DEBUG
				if (zddebug)
					printf("READ_HDR - zdd_sectors is 0\n");
#endif DEBUG
				return(EINVAL);
			}
			if (cbp->cb_count % CNTMULT
			    || (cbp->cb_addr & (ADDRALIGN-1)) != 0
			    || cbp->cb_iovec !=0)
				return (EINVAL);
			if (!useracc((char *)cbp->cb_addr,
						(u_int)cbp->cb_count, B_WRITE))
				return (EFAULT);
			bufalloc(bp);
			bp->b_flags = B_READ;
			bp->b_bcount = cbp->cb_count;
			bp->b_un.b_addr = (caddr_t)cbp->cb_addr;
			break;
			
		case ZDC_FMTTRK:	/* format specified track */
			if (!suser() || !(up->zu_flags & ZUF_EXCLUSIVE)
				     || dd->zdd_sectors == 0) {
#ifdef DEBUG
				if (zddebug && (dd->zdd_sectors == 0))
					printf("FMTTRK - zdd_sectors is 0\n");
#endif DEBUG
				return(EACCES);
			}
			if (cbp->cb_count % CNTMULT
			    || (cbp->cb_addr & (ADDRALIGN-1)) != 0
			    || cbp->cb_iovec !=0) {
#ifdef DEBUG
				if (zddebug)
					printf("FMTTRK - invalid alignment\n");
#endif DEBUG
				return (EINVAL);
			}
			if (!useracc((char *)cbp->cb_addr,
					(u_int)cbp->cb_count, B_WRITE)) {
#ifdef DEBUG
				if (zddebug)
					printf("FMTTRK - page not writable\n");
#endif DEBUG
				return (EFAULT);
			}
			bufalloc(bp);
			bp->b_flags = B_WRITE;
			bp->b_bcount = cbp->cb_count;
			bp->b_un.b_addr = (caddr_t)cbp->cb_addr;
			break;
			
		case ZDC_WRITE_LRAM:
			if (!suser())
				return (EPERM);
			/* Fall into */
		case ZDC_READ_LRAM:
			/*
			 * Check if LRAM address reasonable.
			 */
			if (cbp->cb_addr >= (ZDC_LRAMSZ / sizeof(int)))
				return (EINVAL);
			bufalloc(bp);
			bp->b_flags = (cbp->cb_cmd == ZDC_READ_LRAM) ? B_READ
								     : B_WRITE;
			bp->b_bcount = 0;		/* data in CB */
			bp->b_un.b_addr = NULL;
			break;

		default:
			return (EINVAL);
		}

		/*
		 * Do ioctl.
		 */
		bcopy((caddr_t)cbp, (caddr_t)&up->zu_ioctlcb, FWCBSIZE);
		bp->b_flags |= B_IOCTL;
		bp->b_dev = dev;
		bp->b_blkno = 0;
		bp->b_error = 0;
		bp->b_proc = u.u_procp;
		bp->b_iotype = B_RAWIO;
		BIODONE(bp) = 0;
		++u.u_procp->p_noswap;
		if (bp->b_bcount > 0) {
			/*
			 * lock down pages
			 */
			vslock(bp->b_un.b_addr, (int)bp->b_bcount,
				(bool_t)(bp->b_flags & B_READ));
		}
		s_ipl = p_lock(&up->zu_lock, SPLZD);
		if (up->zu_state == ZU_BAD) {
			v_lock(&up->zu_lock, s_ipl);
			/*
			 * Controller/channel/Drive has gone bad!
			 */
			buffree(bp);
			--u.u_procp->p_noswap;
			return (EIO);
		}
		/*
		 * Insert at head of list and zdstart!
		 */
		bp->av_forw = up->zu_bhead.av_forw;
		up->zu_bhead.av_forw = bp;
		zdstart(up);
		v_lock(&up->zu_lock, s_ipl);
		biowait(bp);
		--u.u_procp->p_noswap;			/* re-swappable */
		error = geterror(bp);
		bcopy((caddr_t)&up->zu_ioctlcb, (caddr_t)cbp, FWCBSIZE);
		buffree(bp);
		if (setokay < 0)
			dd->zdd_sectors = 0;
		if (cbp->cb_cmd == ZDC_SET_CHANCFG) {
			/*
			 * Copy in the Channel Configuration to
			 * make sure it's set before anything else
			 * is done, and set drive size, etc.
			 */
			if (copyin((caddr_t)cbp->cb_addr, (caddr_t)dd, 
			    	sizeof(struct zdcdd)))
				return(EFAULT);
			up->zu_drive_size = dd->zdd_sectors * dd->zdd_tracks
					    * dd->zdd_cyls;
			up->zu_firstsect = dd->zdd_sectors * dd->zdd_tracks;
		}	
		return (error);
	case ZIOFORMATF:
		if (!suser() || !(up->zu_flags & ZUF_EXCLUSIVE))
			return(EACCES);
		s_ipl = p_lock(&up->zu_lock, SPLZD);
		up->zu_flags |= ZUF_FORMATTED;
		up->zu_state = ZU_GOOD;		/* make good */
		v_lock(&up->zu_lock, s_ipl);
		return (0);
	case ZIORESERVE:
		/*
		 * Reserve this channel for exclusive use.
		 * Must be super user, have exclusive access
		 * to the current device, and it must be the
		 * only open device on the channel. It will
		 * inihibit further opens on the channel.
		 */
		if (!suser() || !(up->zu_flags & ZUF_EXCLUSIVE))
			return(EACCES);
		chan = up->zu_drive & 1;
		ctlrp = &zdctrlr[up->zu_ctrlr];
		s_ipl = p_lock(&ctlrp->zdc_ctlrlock, SPLHI);
		for (i=0, tup = &zdunit[0]; i < zdc_conf->zc_nent; i++, tup++) {
			if (tup != up && tup->zu_ctrlr == up->zu_ctrlr
			&&  (tup->zu_drive & 1) == chan
			&&  tup->zu_nopen) {
				v_lock(&ctlrp->zdc_ctlrlock, s_ipl);
				return(EACCES);
			}
		}
		if (ctlrp->zdc_chan_owner[chan]) {
			v_lock(&ctlrp->zdc_ctlrlock, s_ipl);
			return(EACCES);
		}
		ctlrp->zdc_chan_owner[chan] = up;
		v_lock(&ctlrp->zdc_ctlrlock, s_ipl);
		return (0);
	case ZIORELEASE:
		/*
		 * Converse of ZIORESERVE.
		 */
		if (!suser() || !(up->zu_flags & ZUF_EXCLUSIVE))
			return(EACCES);
		chan = up->zu_drive & 1;
		ctlrp = &zdctrlr[up->zu_ctrlr];
		if (ctlrp->zdc_chan_owner[chan] != up) {
			return(EACCES);
		}
		ctlrp->zdc_chan_owner[chan] = (struct zd_unit *)NULL;
		return (0);

#ifdef MIRRORTEST
	case ZIOCTESTFAIL:
		s_ipl = p_lock(&zdfaillock, SPLZD);
		zdtofail = up;
		bcopy(data, (caddr_t)&zdfailcode, sizeof zdfailcode);
		v_lock(&zdfaillock, s_ipl);
		return( 0 );
#endif /* MIRRORTEST */
	default:
		return (EINVAL);
	}
}

/*
 * zdsize()
 *	Used for swap-space partition calculation.
 */
zdsize(dev)
	register dev_t dev;
{
	register struct zd_unit *up;

	up = &zdunit[VUNIT(dev)];

	if (VUNIT(dev) >= zdc_conf->zc_nent
	||  up->zu_state != ZU_GOOD
	||  ((up->zu_cfg & (ZD_FORMATTED|ZD_MATCH)) != (ZD_FORMATTED|ZD_MATCH))
	||  up->zu_part->v_part[VPART(dev)].p_size == 0)
			return (-1);
	return (up->zu_part->v_part[VPART(dev)].p_size);
}

/*
 * Print out status bytes when appropriate.
 */
zddumpstatus(cbp)
	register struct cb *cbp;
{
	register int i;

	if (cbp->cb_cmd == ZDC_INIT || cbp->cb_cmd == ZDC_PROBE ||
	    cbp->cb_cmd == ZDC_PROBEDRIVE)
		return;

	switch(cbp->cb_compcode) {

	case ZDC_DRVPROT:
	case ZDC_DRVFLT:
	case ZDC_SEEKERR:
	case ZDC_HDR_ECC:
	case ZDC_SOFTECC:
	case ZDC_CORRECC:
	case ZDC_ECC:
	case ZDC_SNF:
	case ZDC_REVECT:
	case ZDC_SO:
	case ZDC_NDS:
	case ZDC_FDL:
	case ZDC_DDC_STAT:
		break;

	default:
		if (cbp->cb_cmd == ZDC_READ_LRAM || cbp->cb_cmd == ZDC_WRITE_LRAM)
			break;
		return;
	}
	if (cbp->cb_bp != NULL)
		CPRINTF("zd%d%s: cb_status:", cbp->cb_unit,
			PARTCHR(cbp->cb_bp->b_dev));
	else
		CPRINTF("zd%d: cbstatus:", cbp->cb_unit);
	for (i=0; i < NSTATBYTES; i++)
		CPRINTF(" 0x%x", cbp->cb_status[i]);
	CPRINTF("\n");
}


#ifdef	DEBUG
/*
 * Print out the contents of a cb.
 */
zddumpcb(cbp)
	register struct cb *cbp;
{
	register struct init_cb *icbp;
	register int i;
	register int count;
	int	cbcount;

	CPRINTF("zd%d: cb at 0x%x, cb_cmd 0x%x, cb_compcode 0x%x\n",
		cbp->cb_unit, cbp, cbp->cb_cmd, cbp->cb_compcode);
	if (cbp->cb_cmd == ZDC_INIT) {
		icbp = (struct init_cb *)cbp;
		CPRINTF("icb_ctrl 0x%x, icb_pagesize 0x%x, icb_dumpaddr 0x%x\n",
			icbp->icb_ctrl, icbp->icb_pagesize, icbp->icb_dumpaddr);
		CPRINTF("icb_dest 0x%x, icb_bin 0x%x, icb_vecbase 0x%x\n",
			icbp->icb_dest, icbp->icb_bin, icbp->icb_vecbase);
		CPRINTF("icb_errdest 0x%x, icb_errbin 0x%x, icb_errvector 0x%x\n\n",
			icbp->icb_errdest, icbp->icb_errbin,
			icbp->icb_errvector);
		return;
	}
	if (cbp->cb_cmd == ZDC_PROBE || cbp->cb_cmd == ZDC_PROBEDRIVE) {
		CPRINTF("pcb_drivecfg:");
		for (i = 0; i < ZDC_MAXDRIVES; i++)
			CPRINTF(" 0x%x",
				 ((struct probe_cb *)cbp)->pcb_drivecfg[i]);
		CPRINTF("\n\n");
		return;
	}
	CPRINTF("cb_mod 0x%x, cb_diskaddr (%d, %d, %d), cb_psect 0x%x\n",
		cbp->cb_mod, cbp->cb_cyl, cbp->cb_head, cbp->cb_sect,
		cbp->cb_psect);
	CPRINTF("cb_addr 0x%x, cb_count 0x%x, cb_iovec 0x%x, cb_reqstat 0x%x\n",
		cbp->cb_addr, cbp->cb_count, cbp->cb_iovec, cbp->cb_reqstat);
	zddumpstatus(cbp);
	CPRINTF("cb_bp 0x%x, cb_errcnt 0x%x, cb_iovstart 0x%x, cb_state 0x%x\n",
		cbp->cb_bp, cbp->cb_errcnt, cbp->cb_iovstart, cbp->cb_state);
	CPRINTF("cb_contaddr 0x%x, cb_contiovsz 0x%x, cb_transfrd 0x%x\n",
		cbp->cb_contaddr, cbp->cb_contiovsz, cbp->cb_transfrd);
	if (cbp->cb_iovec != NULL) {
		count = 0;
		cbcount = cbp->cb_count;
		if (cbp->cb_addr & CLOFSET) {
			++count;
			cbcount -= CLBYTES - (cbp->cb_addr & CLOFSET);
		}
		count += (cbcount + CLOFSET) >> CLSHIFT;
		CPRINTF("iovecs:");
		for (i = 0; i < count; i++) {
			CPRINTF(" 0x%x", cbp->cb_iovstart[i]);
			if ((i % 8) == 7)
				CPRINTF("\n\t");
		}
		CPRINTF("\n");
	}
	CPRINTF("\n");
}

zd_printbbl(up)
	struct zd_unit *up;
{
	register struct zdbad	 *zdp;
	register struct bz_bad	 *bb;

	zdp = up->zu_zdbad;
	for (bb = zdp->bz_bad; bb < &zdp->bz_bad[zdp->bz_nsnf]; bb++) {
		CPRINTF("bbl#%d (%d,%d,%d)type:%d->(%d,%d,%d)\n",
			bb - zdp->bz_bad,
			bb->bz_cyl, 
			bb->bz_head, 
			bb->bz_sect,
			bb->bz_rtype,
			bb->bz_rpladdr.da_cyl,
			bb->bz_rpladdr.da_head,
			bb->bz_rpladdr.da_sect);
	}
}
#endif /*DEBUG */
