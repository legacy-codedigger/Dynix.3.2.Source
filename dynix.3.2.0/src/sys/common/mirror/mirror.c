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
static	char	rcsid[] = "$Header: mirror.c 1.5 90/09/14 $";
#endif

/*
 * Dynix/3 disk mirroring.  Symmetry only, DCC only version.
 */

/* $Log:	mirror.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/file.h"
#include "../h/buf.h"
#include "../h/systm.h"
#include "../h/time.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../ufs/mount.h"
#include "../h/uio.h"
#include "../h/ioctl.h"
#include "../balance/slic.h"
#include "../balance/slicreg.h"
#include "../balance/clkarb.h"
#include "../balance/engine.h"		  /* for use with plocal.h */
#include "../h/errno.h"
#include "../h/conf.h"
#include "../h/vm.h"			  /* for use with plocal.h */
#include "../machine/pte.h"		  /* for use with plocal.h */
#include "../machine/plocal.h"		  /* to get our slic number */
#include "../machine/intctl.h"
#include "../mirror/mirror.h"

extern caddr_t calloc();
extern gate_t mirrorgate;
extern struct timeval time;

int mirrorstrat();

static unsigned mirrorlimit;
static u_short mirror_major;
static struct mirror_info *mirrors;
static struct mirror_unit *mirror_units;
static struct mirror_label *mirror_labels;

static void update_labels();
static void  shutdown_unit();
static void shutdown_mirror();
static mirror_read_intr(), mirror_write_intr(), label_update_intr();
static busy_check();

#ifdef MIRRORTEST
long mirror_wedge = 0;			  /* test wedge */
#define MIRROR_TIMEOUT mirror_wedge
#else  /* MIRRORTEST */
#define MIRROR_TIMEOUT 0
#endif /* MIRRORTEST */

/*
 * mirrorboot()
 * Called from conf_pseudo with a single arg--the number of mirrors configured.
 * Does data structure allocation and initialization.
 */
mirrorboot( n )
	u_long n;
{
	u_long i, j;

	/* remember our block device major number */
	for ( i = 0; i < nblkdev; i++ )
		if (bdevsw[i].d_strategy == mirrorstrat) {
			mirror_major = i;
			break;
		}

	/*
	 * NOTE: each of mirrors, mirror_units and mirror_labels MUST be
	 * allocated in a solid block.  The code later depends on this in order
	 * to find the unit given the label, etc.
	 * Also, since I/O happens from the mirror label, they must be block
	 * aligned. 
	 */
	mirrors = (struct mirror_info *)
		calloc((int)n * sizeof (struct mirror_info));
	mirror_units = (struct mirror_unit *)
		calloc(UPM * (int)n * sizeof (struct mirror_unit));
	callocrnd(DEV_BSIZE);
	mirror_labels = (struct mirror_label *)calloc(UPM * (int)n * ML_BSIZE);

	mirrorlimit = n;
	for ( i = 0; i < mirrorlimit; i++ ) {
		mirrors[i].open = 0;
		mirrors[i].active = INACTIVE;
		mirrors[i].nunits = UPM;
		mirrors[i].last_read = 0;
		init_lock(&mirrors[i].lock, mirrorgate);
		mirrors[i].dolabel = 0;
		mirrors[i].start_slic = MAX_NUM_SLIC;
		mirrors[i].blk_min = 0;
		mirrors[i].blk_toobig = 0;
		mirrors[i].blk_offset = 0;
		mirrors[i].unit = mirror_units + UPM*i;
		bufinit(&mirrors[i].queue, mirrorgate);
		mirrors[i].queue.b_flags = B_HEAD;
		mirrors[i].queue.b_actf = (struct buf *)0;
		mirrors[i].queue.b_actl = (struct buf *)0;
		for ( j = 0; j < UPM; j++ ) {
			mirrors[i].unit[j].label = (struct mirror_label *)
				((char *)mirror_labels + (UPM*i + j)*ML_BSIZE);
			bufinit(&mirrors[i].unit[j].ubuf, mirrorgate);
		}
	}
}

/*
 * mirroropen()
 * block and character open.  Opens on a given mirror are exclusive until
 * mirroring has been started on that mirror.
 */
mirroropen(dev)
	dev_t dev;
{
	struct mirror_info *mi;
	spl_t ipl;

	if ( (unsigned)minor(dev) >= mirrorlimit )
		return ENXIO;

	mi = mirrors + (unsigned) minor(dev);
	ipl = p_lock(&mi->lock, MIRROR_SPL);
	
	/* mirror opens are exclusive until mirroring has started */
	if ( mi->open && mi->active!=ACTIVE ) {
		v_lock(&mi->lock, ipl);
		return EBUSY;
	}
	
	mi->open++;
	v_lock(&mi->lock, ipl);

	return(0);
}

/*
 * mirrorclose
 * called from [bc]devsw.  Balances number of opens.
 */
mirrorclose(dev)
	dev_t dev;
{
	struct mirror_info *mi;
	spl_t ipl;

	if ( (unsigned)minor(dev) >= mirrorlimit )
		return ENXIO;

	mi = mirrors + (unsigned) minor(dev);
	ipl = p_lock(&mi->lock, MIRROR_SPL);
	
	mi->open--;
	v_lock(&mi->lock, ipl);

	return(0);
}

/*
 * mirrorstrat()
 * called from bdevsw.  Puts a bp on the mirror's queue of work to do and calls
 * mirror_start if the queue was empty (mirror idle). The mirror must be active
 * to do I/O
 */
mirrorstrat(bp)
	struct buf *bp;
{
	u_short unit = minor(bp->b_dev);
	spl_t ipl;
	struct mirror_info *mi = mirrors + unit;

	ipl = p_lock(&mi->lock, MIRROR_SPL);
	if (unit >= mirrorlimit
	    || mi->active != ACTIVE) {
		v_lock(&mi->lock, ipl);
		bp->b_flags |= B_ERROR;
		bp->b_error = ENXIO;
		bp->b_resid = bp->b_bcount;
		biodone(bp);
		return;
	}

	if (bp->b_bcount <= 0
	    || (bp->b_bcount & (DEV_BSIZE-1)) != 0
	    || bp->b_iotype == B_RAWIO
	       && ((int)bp->b_un.b_addr & (ML_ADDRALIGN-1)) != 0
	    || bp->b_blkno < mi->blk_min
	    || bp->b_blkno+(bp->b_bcount >> DEV_BSHIFT) > mi->blk_toobig) {
		v_lock(&mi->lock, ipl);
		bp->b_flags |= B_ERROR;
		bp->b_error = EINVAL;
		bp->b_resid = bp->b_bcount;
		biodone(bp);
		return;
	}

	bp->b_sortkey = bp->b_blkno;
	if (!mi->unit[0].ok && !mi->unit[1].ok) {
		/* No good units! */
		v_lock(&mi->lock, ipl);
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		bp->b_resid = bp->b_bcount;
		biodone(bp);
		return;
	}


	if (mi->queue.b_actf || mi->dolabel) {
		/* queue has something on it */
		disksort(&mi->queue, bp);
	} else {
		disksort(&mi->queue, bp);
		mirror_start(mi);
	}
	v_lock(&mi->lock, ipl);
}

/*
 * mirror_start()
 * Called with the unit's lock held, and mirror ACTIVE (which guarantees at
 * least one good unit), or dolabel set. Start an I/O. May be called from both
 * top and bottom halves of the mirror driver. Around every call to the unit's
 * driver's strategy routine, record the slic of the processor we're running on
 * in this mirror's start_slic field so if the disk driver strategy routine
 * we're calling calls biodone directly (NOT via an interrupt), we can
 * recognize this and fake an interrupt with timeout, thus avoiding deadlock.
 */
static
mirror_start(mi)
	struct mirror_info *mi;
{
	struct buf *bp = mi->queue.b_actf;
	struct mirror_unit *unit;
	int i, first_checked, all_bad;

	/* enforce exclusive use of buffers */
	if ( mi->unit[0].wip || mi->unit[1].wip )
		return;

	if (mi->dolabel) {
		update_labels(mi);
	} else	if (!bp) {
		printf("mirror_start: null request\n");	
		/*
		 *+ The mirror start routine was called but no
		 *+ jobs exist on its queue.
		 */
		return;
	} else	if (bp->b_flags & B_READ) {
		/* reading */
		mi->last_read = (mi->last_read+1)%mi->nunits;
		unit = mi->unit + mi->last_read;
		first_checked = mi->last_read;
		while (!unit->ok) {
			mi->last_read = (mi->last_read+1)%mi->nunits;
			unit = mi->unit + mi->last_read;
			ASSERT( mi->last_read != first_checked,
			       "mirror_start: read: no units");
			/*
			 *+ No disk mirror devices can be found but
			 *+ a read request has been made for a disk mirror
			 *+ device.
			 */
		}
			
		unit->wip = TRUE;
		bcopy((caddr_t)bp, (caddr_t) &unit->ubuf, sizeof (struct buf));
		bufinit(&unit->ubuf, mirrorgate); /* reset semaphores */
		unit->ubuf.b_flags |= B_CALL;
		unit->ubuf.b_iodone = mirror_read_intr;
		unit->ubuf.b_dev = unit->block;
		unit->ubuf.b_blkno += mi->blk_offset;
		mi->start_slic = l.eng->e_slicaddr; /* save my slic addr. */
		(*bdevsw[major(unit->block)].d_strategy)(&unit->ubuf);
		mi->start_slic = MAX_NUM_SLIC;	  /* forget it. */
	} else {
		/* writing */
		all_bad = TRUE;
		for ( i=0; i<mi->nunits; i++ ) 
			if (mi->unit[i].ok) {
				all_bad = FALSE;
				unit = mi->unit + i;
				bcopy( (caddr_t)bp, (caddr_t)&unit->ubuf,
				      sizeof (struct buf));
				/* bufinit resets semaphores */
				bufinit(&unit->ubuf, mirrorgate);
				unit->ubuf.b_flags |= B_CALL;
				unit->ubuf.b_iodone = mirror_write_intr;
				unit->ubuf.b_dev = unit->block;
				unit->ubuf.b_blkno += mi->blk_offset;
				unit->wip = TRUE;
				/* save my slic addr. */
				mi->start_slic = l.eng->e_slicaddr;
				(*bdevsw[major(unit->block)].d_strategy)
					(&unit->ubuf);
				mi->start_slic = MAX_NUM_SLIC; /* forget it. */
			}
		ASSERT( !all_bad, "mirror_start: write: no units");
		/*
		 *+ No disk mirror devices can be found but
		 *+ a write request has been made for a disk mirror device.
		 */
	}
}

/*
 * mirrorminphys(bp)--correct for too large a request.  Mirrors can handle
 * anything any disk below them can dish out, so just call minphys of this
 * block for both units.  On writes, giving both units a chance to minphys is
 * obviously necessary, but we need to do it on reads as well, because we don't
 * know which drive will get the request (because one could go out between now
 * and start time).  Even if the device types are the same, and the partitions
 * are the same, the layout may be different, so we always need to do both
 * calls. 
 */
mirrorminphys(bp)
	struct buf *bp;
{
	u_short maj;
	struct mirror_info *mr = mirrors + (u_short)minor(bp->b_dev);
	struct mirror_unit *mu;
	int i;
	dev_t original_dev = bp->b_dev;

	for ( i = 0, mu = mr->unit; i < mr->nunits; i++, mu++ ) {
		bp->b_dev = mu->block;
		maj = major( mu->block );
		if ( maj < nblkdev )
			(*bdevsw[maj].d_minphys)(bp);
	}
		
	bp->b_dev = original_dev;
}

/*
 * mirrorsize
 * called from bdevsw if we're swapping on a mirror.  Note that the mirror must
 * be active for this to return a reasonable number.
 */
mirrorsize(dev)
	dev_t dev;
{
	u_short unit = minor(dev);
	struct mirror_info *mi = mirrors + unit;
	int size = -1;
	spl_t ipl;

	if (unit >= mirrorlimit)
		return( -1 );
	
	ipl = p_lock(&mi->lock, MIRROR_SPL);
	if (mi->active == ACTIVE)
		size = mi->blk_toobig;	  /* mirror size, when active */
	v_lock(&mi->lock, ipl);

	return size;
}

/*
 * mirroruio()
 * This routine is the common code for mirrorread and mirrorwrite.
 */
static
mirroruio(dev, uio, direction)
	dev_t	dev;
	struct	uio *uio;
	int	direction;
{
	int err, diff;
	off_t lim;

	lim = mirrors[minor(dev)].blk_toobig << DEV_BSHIFT;

	err = physck(lim, uio, direction, &diff);
	if (err != 0) {
		if (err == -1)	/* not an error, but request of 0 bytes */
			err = 0;
		return (err);
	}
	err = physio(mirrorstrat, (struct buf *)0, dev, direction,
		     mirrorminphys, uio); 
	uio->uio_resid += diff;
	return (err);
}

/*
 * mirrorread
 * called from cdevsw--raw read.
 */
mirrorread(dev, uio)
	dev_t	dev;
	struct uio *uio;
{
	return( mirroruio( dev, uio, B_READ ) );
}

/*
 * mirrorwrite()
 * called from cdevsw--raw write;
 */
mirrorwrite(dev, uio)
	dev_t	dev;
	struct uio *uio;
{
	return( mirroruio( dev, uio, B_WRITE ) );
}

/*
 * mirrorioctl
 * called from cdevsw.  This routine is used to turn mirroring on and off and
 * to get information from active mirrors.
 */

/* ARGSUSED */
mirrorioctl(dev, cmd, data, flag)
	dev_t	dev;
	int	cmd;
	caddr_t	data;
	int	flag;
{
	unsigned unit = minor(dev);
	int error = 0;
	spl_t ipl;
	struct mirror_info *mi = mirrors + unit;
	struct vnode *dev_vp;
	int damaged = 0;		  /* for starting a damaged mirror */

	if ( unit >= mirrorlimit )
		return(ENXIO);

	switch(cmd) {
	case MIOCVRFY:
		/* Do all validation for, but don't start, mirroring */

		if (!suser())	  /* permission to mirror, sir! */
			return( EPERM );

		ipl = p_lock(&mi->lock, MIRROR_SPL);
		error = busy_check(mi, (dev_t *)data);
		v_lock(&mi->lock, ipl);
		return error;
	case MIOCON:
		/* Turn on mirroring */

		if (!suser())	  /* permission to mirror, sir! */
			return( EPERM );

		ipl = p_lock(&mi->lock, MIRROR_SPL);
		error = busy_check(mi, (dev_t *)data);
				
		/*
		 * We need to drop the lock to do I/O. CHANGING keeps
		 * anyone else from messing with this unit
		 */
		if (!error)
			mi->active = CHANGING;

		v_lock(&mi->lock, ipl);

		if (error)
			return( error );
		if (mi->unit[1].raw == -1 && mi->unit[1].block == -1)
			damaged = 1;

		mi->nunits = damaged? 1 : UPM; /* needed in read_labels */

		if ( (error=read_labels( mi )) == 0 ) {
			ipl = p_lock(&mi->lock, MIRROR_SPL);
			mi->blk_min = mi->unit[0].label->first_blk;
			mi->blk_toobig = mi->unit[0].label->blk_break;
			mi->blk_offset = mi->unit[0].label->label_size
				- mi->unit[0].label->first_blk; 
			mi->unit[0].ok = TRUE;
			mi->unit[1].ok = damaged? FALSE : TRUE;
			mi->unit[0].wip = FALSE;
			mi->unit[1].wip = FALSE;
			bufinit(&mi->unit[0].ubuf, mirrorgate);
			bufinit(&mi->unit[1].ubuf, mirrorgate);
			mi->active = ACTIVE;
			update_labels(mi);
			v_lock(&mi->lock, ipl);
		} else {
			ipl = p_lock(&mi->lock, MIRROR_SPL);
			mi->active = INACTIVE;
			v_lock(&mi->lock, ipl);
		}
		return( error );

	case MIOCOFF:
		/* Turn off mirroring.  Be sure we alone have it open. */
		if (!suser())	  /* permission to unmirror, sir! */
			return( EPERM );

		ipl = p_lock(&mi->lock, MIRROR_SPL);
		if (mi->active == ACTIVE) {
			if (mi->open == 1) {
				/* flush/invalidate all block. */
				dev = makedev(mirror_major, mi - mirrors);
				DEVTOVP(dev, dev_vp);
				bflush(dev_vp);
				binval(dev_vp);
				
				mi->active = SHUTDOWN;
				if (mi->queue.b_actf) {
					/* b_actl invalid if b_actf = 0  */
					mi->queue.b_actl->b_flags &= ~B_ASYNC;
					v_lock(&mi->lock, ipl);
					biowait(mi->queue.b_actl);
					ipl = p_lock(&mi->lock, MIRROR_SPL);
				}
				/*
				 * No more I/O pending, update labels. This
				 * must be done with the lock held.  The
				 * label_update_intr will do the final
				 * shutdown. 
				 */
				update_labels(mi);
				v_lock(&mi->lock, ipl);
				return( 0 );
			} else	error = EBUSY;
		} else	error = ENXIO;
		v_lock(&mi->lock, ipl);
		return( error );

	case MIOCOINFO:
		/*
		 * returns info on *other* mirrors.  Choose which one from
		 * ((struct mioc *)data)->limit
		 */
		if ( ((struct mioc *)data)->limit >= mirrorlimit )
			return ENXIO;
		mi = mirrors + ((struct mioc *)data)->limit;
		/* FALLTHROUGH */
	case MIOCINFO:
		bzero(data, sizeof (struct mioc));
		/* return info for this mirror */
		ipl = p_lock(&mi->lock, MIRROR_SPL);
		((struct mioc *)data)->limit = mirrorlimit;
		bcopy((caddr_t)mi, (caddr_t)&((struct mioc *)data)->mirror,
		      sizeof (struct mioc_info)); 
		if (mi->active == ACTIVE) {
			/* This info only valid if we're a mirror */
			bcopy((caddr_t)&mi->unit[0],
			      (caddr_t)&((struct mioc *)data)->unit[0],
			      sizeof (struct mioc_unit)); 
			bcopy((caddr_t)&mi->unit[1],
			      (caddr_t)&((struct mioc *)data)->unit[1],
			      sizeof (struct mioc_unit)); 
			bcopy((caddr_t)mi->unit->label,
			      (caddr_t)&((struct mioc *)data)->label,
			      sizeof (struct mioc_label)); 
		}
		v_lock(&mi->lock, ipl);
		return( error );

	default:
		return( EINVAL );
	}
	/* NOTREACHED */
}

/*
 * busy_check
 * checks to be sure that the proposed units of the mirror are not in use by
 * another mirror and that the units are also not mounted.  Returns EBUSY if
 * busy, EPERM if no write access to the mirror, 0 otherwise.  Called with lock
 * to mi held! Fills in the mi->unit[*]->{raw,block} fields, too.
 */
static
busy_check(mi, devs)
	struct mirror_info *mi;
	dev_t *devs;
{
	int i, j, k, umax = UPM;
	struct mount *mp;

	if (mi->active != INACTIVE   ||   mi->open != 1)
		return( EBUSY );

	mi->unit[0].raw = devs[U0RAW];
	mi->unit[0].block = devs[U0BLOCK];
	mi->unit[1].raw = devs[U1RAW];
	mi->unit[1].block = devs[U1BLOCK];

	if (mi->unit[1].raw == -1 && mi->unit[1].block == -1)
		umax = 1;		  /* "damaged" configuration */
	
	/* Be sure the units aren't already being mirrored */
	for ( i = 0; i < mirrorlimit; i++ ) {
		/* for each mirror */
		if (mirrors+i==mi || mirrors[i].active!=ACTIVE)
			continue;

		for ( j = 0; j < UPM; j++ )
			/* for each unit */
			for ( k = 0; k < umax; k++)
			      /* for each unit passed in */
			      if (mirrors[i].unit[j].block == mi->unit[k].block
				  || mirrors[i].unit[j].raw == mi->unit[k].raw)
				      return( EBUSY );
	}
	
	/* Be sure the new units aren't mounted, either */
	for (mp = &mounttab[0]; mp < mountNMOUNT; mp++) 
		/* for each mount point */
		for ( j=0; j<umax; j++ )
			/* for each new unit */
			if (mp->m_bufp != 0 && mp->m_dev==mi->unit[j].block )
				return( EBUSY );
	return( 0 );
}

/*
 * read_labels()
 * Called when turning on mirroring from mirrorioctl(,MIOCON,)
 * Read the two labels in and compare them (passing back any errors).  If they
 * are units of the same mirror, and they are equally up-to-date, update them
 * and return 0. Otherwise, return EINVAL. 
 */
static
read_labels ( mi )
	struct mirror_info *mi;
{
	struct mirror_label *a = mi->unit[0].label, *b = mi->unit[1].label;
	int error, i, damaged = 0;

	if (mi->nunits == 1)
		damaged = 1;		  /* damaged units are special */

	for ( i=0; i < mi->nunits; i++ )
		if ( (error=read_label(mi->unit+i)) != 0)
			return( error );

	/*
	 * Check bmaps.  Must be check here because here is where we know if
	 * the mirror is damaged or not. 
	 */
	if ( damaged ) {
		/* Only label a has been read */
		if ( a->unit==0 && a->bmap0
		||   a->unit==1 && a->bmap1 )
			return EINVAL;
	} else 	if ( a->bmap0 || a->bmap1 || b->bmap0 || b->bmap1 )
		return( EINVAL );

	/* A "damaged" mirror, with only one unit, can't do compares. */
	if ( damaged )
		return( 0 );

	if ( a->label_time != b->label_time
	    || a->mminor != b->mminor
	    || a->unit == b->unit
	    || a->blk_break != b->blk_break )
		return( EINVAL );

	return( 0 );
}

/*
 * read_label()
 * Called from read_labels for each unit.
 * Read the label of the given unit, and verify that it is good.  If all goes
 * well,  return zero and leave the unit "open".  On error, return the error
 * that the user should get, and leave the unit "closed".
 */
static
read_label( unit )
	struct mirror_unit *unit;
{
	int error, size;
	dev_t	dev = unit->block;
	struct vnode *dev_vp;
	struct buf *bp;
	u_int maj = (u_int)major( dev );

	if (maj >= nblkdev)
		return( ENXIO );

	if ( (error=(*bdevsw[maj].d_open)(dev, FREAD|FWRITE)) != 0 )
		return( error );

	DEVTOVP(dev, dev_vp);
	bp = bread(dev_vp, ML_BLOCK, ML_BSIZE);
	if (bp->b_flags & B_ERROR) {
		brelse(bp);
		(void)(*bdevsw[maj].d_close)(dev, FREAD|FWRITE);
		return(EIO);
	}

	/* Get all the info in the block, even beyond the label */
	bcopy((caddr_t)bp->b_un.b_addr, (caddr_t)unit->label, ML_BSIZE);
	brelse(bp);
	binval(dev_vp);			  /* Won't be right soon anyway... */

	if ( bdevsw[maj].d_psize ) {
		size = (*bdevsw[maj].d_psize)(dev, FREAD|FWRITE);
		if ((int)size == -1 )	  /* error return from d_psize */
			size = 0;
	} else	size = unit->label->blk_break;

	if ( unit->label->mirror_magic != MIRROR_MAGIC
	    || unit->label->unit > 1
	    || unit->label->label_size != 1
	    || unit->label->first_blk > 1
	    || unit->label->blk_break > (unsigned)size
	    || unit->label->bmap_density != M_BMAP_DENSITY
	    || unit->label->nunits != UPM )
		/* NOTE: bmaps MUST be checked by caller */
	{
		(void)(*bdevsw[maj].d_close)(dev, FREAD|FWRITE);
		return( EINVAL );
	}

	return( 0 );
}

/*
 * update_labels()
 * Called with lock held! Called from mirror_ioctl when the mirror is being
 * turned off, or on startup (AFTER the mirror is marked as active) and from
 * mirror_start.  (This includes calls from the interrupt routines.)  Update
 * the mirror label on both units, even if they are bad. If the unit is already
 * bad, the error (if any) will be ignored. When we call the units' driver's
 * strategy routine, we record our slic number.  This way, if that strategy
 * routine calls biodone directly (instead of via an interrupt), we can detect
 * it and fake an interrupt (via timeout) instead of dead locking.
 */
static void
update_labels(mr)
	struct mirror_info *mr;
{
	struct mirror_label *label;
	time_t label_time = time.tv_sec - 1; /* time of labeling */
	struct mirror_unit *unit;
	int i;
	struct buf *bp;
	
	for ( i=0; i<mr->nunits; i++ )
		if (mr->unit[i].wip) {
			/*
			 * The write buffers are in use, so we can't use them
			 * for relabeling.  Setting dolabel will call here
			 * again when they are freed. 
			 */
			mr->dolabel = 1;
			return;
		}
	
#ifdef MIRRORTEST
	if (MIRROR_TIMEOUT) {
		/* Widen the window here */
		label_time = time.tv_sec;
		while (time.tv_sec < label_time+30)
			if (mr->queue.b_actf)
				break;
	}
#endif /* MIRRORTEST */
	for ( i=0; i <mr->nunits; i++ )
		mr->unit[i].wip = TRUE;
	mr->dolabel = 0;

	/* for each unit... */
	for ( i = 0; i<mr->nunits; i++ ) {
		unit = mr->unit + i;
		label = unit->label;
		bp = &unit->ubuf;

		/*
		 * update the in memory copy of the labels. DO NOT update the
		 * nunits field.  Although we have changed the memory copy,
		 * that is only for damaged mirroring and should *never* be
		 * written in an on disk label.
		 */
		label->label_time = label_time;
		label->mminor = mr - mirrors;
		label->unit = i;
		label->bmap0 = mr->unit[0].ok? 0 : 0xff;
		label->bmap1 = mr->unit[1].ok? 0 : 0xff;
		if (mr->active == ACTIVE) {
			label->bmap0 |= MRU_INUSE;
			label->bmap1 |= MRU_INUSE;
		}
		/* do the write */
		
		/* setup buf..... */
		bp->b_flags = B_CALL;	  /* All other flags off! */
		bp->b_bcount = ML_BSIZE;
		bp->b_bufsize = ML_BSIZE;
		bp->b_error = 0;
		bp->b_dev = unit->block;
		bp->b_iotype = B_FILIO;	  /* not really, but acts like it! */
		bp->b_un.b_addr = (caddr_t) label;
		bp->b_blkno = ML_BLOCK;
		DEVTOVP(unit->block, bp->b_vp);
		bp->b_iodone = label_update_intr; /* catch/report errors */
		bufinit(bp, mirrorgate);  /* reset semaphores */
		mr->start_slic = l.eng->e_slicaddr; /* save my slic addr. */
		(*bdevsw[major(unit->block)].d_strategy)(bp);
		mr->start_slic = MAX_NUM_SLIC;	  /* forget it. */
	}
	return;
}

/*
 * shutdown_unit()
 * Flushes and invalidates blocks associated with this unit.  Only called from
 * interrupt routines.  This routine sets the dolabel flag so that the
 * interrupt routine will call update_labels (via mirror_start) just before it
 * exits. If there are no more good units, the mirror state is set to SHUTDOWN,
 * and label_update_intr will finish shutting down the mirror. Only called if
 * there was an error that requires this unit to be shutdown.
 */
static void
shutdown_unit(unit)
	struct mirror_unit *unit;
{
	struct mirror_info *mi = mirrors + (unit - mirror_units)/UPM;
	spl_t ipl;
	
	/* turn off unit, if it was on */
	ipl = p_lock(&mi->lock, MIRROR_SPL);
	if (!unit->ok) {
		v_lock(&mi->lock, ipl);
		return;
	}
	unit->ok = FALSE;
	mi->dolabel = TRUE;
	if (!mi->unit[0].ok && !mi->unit[1].ok) {
		mi->active = SHUTDOWN;
		v_lock(&mi->lock, ipl);
		printf( "Mirror %d unit %d failure.\n", mi - mirrors,
		       unit - mi->unit);
		printf("Mirror %d: all units bad, deactivating.\n",
		       mi - mirrors); 
		/*
		 *+ The disk mirroring software has detected an
		 *+ fatal inconsistancy, all mirroring is disabled.
		 */
	} else 	{
		v_lock(&mi->lock, ipl);
		printf( "Mirror %d unit %d failure.\n", mi - mirrors,
		       unit - mi->unit);
		/*
		 *+ The disk mirroring software has detected an
		 *+ fatal inconsistancy, the mirroring on the
		 *+ the specified drive is disabled.
		 */
	}

	/* turn on error light */
	FP_IO_ERROR;
}

/*
 * shutdown_mirror()
 * Shutdown a mirror. Called from label_update_intr  witht he lock held (!!) if
 * the state of the mirror is  SHUTDOWN. Clears out the mirrors queue (fails
 * the I/O). Flushes and invalidates any blocks associated with this mirror.
 * Closes the units associated with this mirror . Sets mirror->active =
 * INACTIVE  
 *
 * We really don't have any useful information to return here, but we're still
 * an int function because we are called from timeout sometimes.
 */
static void
shutdown_mirror(mi)
	struct mirror_info *mi;
{
	dev_t dev;
	struct buf *bp, *nbp;
	int i;

	bp = mi->queue.b_actf;

	/*
	 * Flush I/O requests still in the queue.  If the queue isn't empty,
	 * we're being called because all units failed, so that's EIO.
	 */
	while ( bp ) {
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		bp->b_resid = bp->b_bcount;
		nbp = bp->b_actf;
		biodone(bp);
		bp = nbp;
	}

	mi->queue.b_actf = (struct buf *)0;

	/* Next, close out individual units */
	for (i = 0; i < mi->nunits; i++) {
		mi->unit[i].ok = FALSE;
		dev = mi->unit[i].block;
		/*
		 * We don't use block i/o except to read the label, so all we
		 * need to do is close
		 */
		(void)(*bdevsw[major(dev)].d_close)(dev, FREAD|FWRITE);
	}

	/* finally, deactive mirror */
	mi->active = INACTIVE;
}

/*
 * mirror_restart()
 * Called by timeout when one of the interrupt routines needs to defer a call
 * to either mirror_start or shutdown_mirror.  Obeys the proper protocols for
 * these routines, and HOLDS the LOCK!
 */
static
mirror_restart(mi)
	struct mirror_info *mi;
{
	spl_t ipl;
	
	ipl = p_lock(&mi->lock, MIRROR_SPL);
	if ( mi->active == SHUTDOWN )
		shutdown_mirror( mi );
	if ( (mi->queue.b_actf  || mi->dolabel)
	    && mi->active==ACTIVE )
		mirror_start(mi);
	v_lock(&mi->lock, ipl);
}

/*
 * mirror_read_intr()
 * Called by biodone when a mirror unit has finished the read. Since this
 * routine calls the units' driver's strategy routine, we need to record the
 * slic number of the processor in case that strategy routine will call biodone
 * directly, thereby calling us while we still have the lock.  We fix this
 * situation by calling ourselves via timeout instead of deadlocking.
 *
 * Another situation that can cause us grief is if we are called from the
 * units' driver in a way that generates another call to that driver.  If the
 * driver holds the lock and wants the lock, we get deadlock.
 *
 * If we get an error, we allow retry via mirror_start.  In order to not
 * deadlock if both units are on the same drive and it goes, we need to call
 * the strategy routine via timeout anyway.  If we leave it up to strategy, it
 * is cleaner all the way around.
 */
static
mirror_read_intr(bp)
	struct buf *bp;
{
	struct mirror_unit *unit;
	struct mirror_info *mi;
	int error = 0;
	spl_t ipl;

	unit = (struct mirror_unit *)
		((caddr_t)bp - (caddr_t)&((struct mirror_unit *)0)->ubuf);
	mi = mirrors + (unit - mirror_units)/UPM;

	if (mi->start_slic == l.eng->e_slicaddr) {
		/*
		 * Start_slic is always MAX_NUM_SLIC when the lock isn't held,
		 * and it is set to the holding processor's slic id, so this is
		 * a safe check.  It is only set inside mirror_start, so we are
		 * in a direct function call chain from mirror_start, which
		 * means the device strategy routine called there called
		 * biodone directly. We now arrange for us to get an
		 * "interrupt" via timeout.
		 */
		timeout(mirror_read_intr, (caddr_t)bp, 0);
		return;
	}

	/*
	 * The host disk strategy routine will return EINVAL only if a raw
	 * transaction doesn't meet the alignment constraints.  Thus, we'll
	 * propagate EINVAL without dismantling the mirror.
	 */
	if ( unit->ubuf.b_flags & B_ERROR ) {
		if (unit->ubuf.b_error == EINVAL) {
			ipl = p_lock(&mi->lock, MIRROR_SPL);
			mi->queue.b_actf->b_flags |= B_ERROR;
			mi->queue.b_actf->b_error = EINVAL;
		} else {
			/* Recover from error. */
			error = 1;
			printf( "Mirror %d unit %d read failed, ",
			       mi - mirrors, unit - mi->unit);
			printf("block maj %d min %d\n",  major(unit->block),
			       minor(unit->block) );
			/*
			 *+ A read error occured on the specified disk
			 *+ mirror. The mirroring on the
			 *+ the specified drive will be disabled.
			 */
			shutdown_unit(unit);
			ipl = p_lock( &mi->lock, MIRROR_SPL );
			if (mi->active != ACTIVE) {
				/* No more active units, propagate errors */
				mi->queue.b_actf->b_flags |= B_ERROR;
				mi->queue.b_actf->b_error = EIO;
			}
		}
	} else	ipl = p_lock(&mi->lock, MIRROR_SPL);
	unit->wip = FALSE;

	if ( !error ) {
		/* If !error provides retry. */
		bp = mi->queue.b_actf;
		mi->queue.b_actf = bp->b_actf;
		bp->b_resid = mi->unit[mi->last_read].ubuf.b_resid;
		biodone(bp);
	}

	if ( mi->dolabel
	||   mi->queue.b_actf && mi->active==ACTIVE ) {
		if (error)
			timeout(mirror_restart, (caddr_t)mi, 0);
		else	mirror_start(mi);
	}
	v_lock(&mi->lock, ipl);
	return;
}

/*
 * mirror_write_intr()
 * Called by biodone when a mirror unit has finished the write.
 *
 * See mirror_read_intr for discussion of possible deadlocks and the avoidance
 * code 
 */
static
mirror_write_intr(bp)
	struct buf *bp;
{
	int i;
	int error = 0;
	struct mirror_unit *unit;
	struct mirror_info *mi;
	spl_t ipl;

	unit = (struct mirror_unit *)
		((caddr_t)bp - (caddr_t)&((struct mirror_unit *)0)->ubuf);
	mi = mirrors + (unit - mirror_units)/UPM;

	if (mi->start_slic == l.eng->e_slicaddr) {
		/*
		 * Start_slic is always MAX_NUM_SLIC when the lock isn't held,
		 * and it is set to the holding processor's slic id, so this is
		 * a safe check.  It is only set inside mirror_start, so we are
		 * in a direct function call chain from mirror_start, which
		 * means the device strategy routine called there called
		 * biodone directly. We now arrange for us to get an
		 * "interrupt" via timeout.
		 */
		timeout(mirror_write_intr, (caddr_t)bp, 0);
		return;
	}

	/*
	 * The host disk strategy routine will return EINVAL only if a raw
	 * transaction doesn't meet the alignment constraints.  Thus, we'll
	 * propagate EINVAL without dismantling the mirror.
	 */
	if ( unit->ubuf.b_flags & B_ERROR ) {
		if (unit->ubuf.b_error == EINVAL) {
			ipl = p_lock(&mi->lock, MIRROR_SPL);
			mi->queue.b_actf->b_flags |= B_ERROR;
			mi->queue.b_actf->b_error = EINVAL;
		} else {
			error = 1;
			printf("Mirror %d unit %d write failed, ",
			       mi - mirrors, unit - mi->unit);
			printf("block maj %d min %d\n", major(unit->block),
			       minor(unit->block) );
			/*
			 *+ A write error occured on the specified disk
			 *+ mirror. The mirroring on the
			 *+ the specified drive will be disabled.
			 */
			shutdown_unit(unit);
			ipl = p_lock(&mi->lock, MIRROR_SPL);
			if (mi->active != ACTIVE) {
				/* No alive units, propagate errors */
				mi->queue.b_actf->b_flags |= B_ERROR;
				mi->queue.b_actf->b_error = EIO;
			}
		}
	} else	ipl = p_lock(&mi->lock, MIRROR_SPL);
		
	unit->wip = FALSE;
	
	if ( mi->unit[0].wip || mi->unit[1].wip ) {
		v_lock(&mi->lock, ipl);
		return;
	}

	bp = mi->queue.b_actf;

	mi->queue.b_actf = bp->b_actf;

	bp->b_resid = bp->b_bcount;
	for (i=0, bp->b_resid=mi->unit[0].ubuf.b_resid; i<mi->nunits; i++)
		if (mi->unit[i].ok && mi->unit[i].ubuf.b_resid < bp->b_resid)
			bp->b_resid = mi->unit[i].ubuf.b_resid;
	biodone(bp);

	if ( mi->dolabel
	||  mi->queue.b_actf && mi->active==ACTIVE ) {
		if (error)
			timeout(mirror_restart, (caddr_t)mi, 0);
		else	mirror_start(mi);
	}
	v_lock(&mi->lock, ipl);
	return;
}

/*
 * label_update_intr()
 * Called by biodone when a mirror unit has finished writing the label.  Checks
 * for errors and corrects both memory copies of the mirror labels as well as
 * the unit structures should it detect a previously good unit gone bad.
 * Mirror label updates are still attempted to bad units, but failure comes as
 * no surprise.
 */
static
label_update_intr(bp)
	struct buf *bp;
{
	spl_t ipl;
	struct mirror_label *label = (struct mirror_label *)bp->b_un.b_addr;
	struct mirror_info *mi = mirrors + label->mminor;
	struct mirror_unit *unit = mi->unit + label->unit;
	int error = 0;

	if (mi->start_slic == l.eng->e_slicaddr) {
		/*
		 * Start_slic is always MAX_NUM_SLIC when the lock isn't held,
		 * and it is set to the holding processor's slic id, so this is
		 * a safe check.  It is only set inside mirror_start, so we are
		 * in a direct function call chain from mirror_start, which
		 * means the device strategy routine called there called
		 * biodone directly. We now arrange for us to get an
		 * "interrupt" via timeout.
		 */
		timeout(label_update_intr, (caddr_t)bp, 0);
		return;
	}

	if (bp->b_flags & B_ERROR) {
		error = 1;
		if (unit->ok) {
			/* Unit was good, went bad.  Tell 'em, Jim */
			printf( "Mirror %d: error labeling unit %d",
			       mi - mirrors, unit - mi->unit);
			printf( " block dev maj = %d, min = %d\n",
			       major(unit->block), minor(unit->block));
			/*
			 *+ A labeling error occured on the specified disk
			 *+ mirror. The mirroring on the
			 *+ the specified drive will be disabled.
			 */
			shutdown_unit(unit);
		}
	}

	ipl = p_lock(&mi->lock, MIRROR_SPL);
	unit->wip = FALSE;
	if (mi->unit[0].wip || mi->unit[1].wip) {
		v_lock(&mi->lock, ipl);
		return;
	}
	
	/*
	 * If we need to label again, start it, and we'll check for shutdown
	 * next time around.  Always shutdown before trying to service the
	 * queue, otherwise there is a race between the interrupt routine and
	 * shutdown_mirror for calling biodone on the first buffer in the
	 * queue. If we call mirror_start when we've gotten an error, we need
	 * to call it via timeout to avoid deadlock.  See mirror_read_intr for
	 * more info
	 */
	if ( mi->dolabel )
		timeout(mirror_restart, (caddr_t)mi, MIRROR_TIMEOUT);
	else	if ( mi->active == SHUTDOWN ) {
		if (error)
			timeout(mirror_restart, (caddr_t)mi, MIRROR_TIMEOUT);
		else	shutdown_mirror( mi );
	} else	if ( mi->queue.b_actf && mi->active==ACTIVE ) {
		if (error)
			timeout(mirror_restart, (caddr_t)mi, MIRROR_TIMEOUT);
		mirror_start(mi);
	}
	v_lock(&mi->lock, ipl);
}
