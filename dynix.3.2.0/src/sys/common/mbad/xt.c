/* Copyright (c) 1984, 1985, 1986, 1987 Sequent Computer Systems, Inc.
 * All rights reserved
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

#ifndef	lint
static	char	rcsid[] = "$Header: xt.c 2.22 1991/12/19 00:35:43 $";
#endif	lint

/*
 * xt.c
 * 
 * Xylogics 472 Multibus Tape Driver
 */

/* $Log: xt.c,v $
 *
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/uio.h"
#include "../h/systm.h"
#include "../h/vm.h"
#include "../h/mtio.h"
#include "../h/ioctl.h"
#include "../h/file.h"
#include "../h/kernel.h"
#include "../h/cmn_err.h"

#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/ioconf.h"

#include "../mbad/mbad.h"
#include "../mbad/xt.h"

#ifdef	XTDEBUG

int	xtdebug	= 0;

#endif	XTDEBUG

/*
 * Driver multibus interface routines and variables.
 */
int	xtprobe(), xtboot(), xtintr();
struct	mbad_driver zt_driver = {
	"zt",				/* name */
	MBD_TYPICAL,			/* configuration flags */
	xtprobe,			/* probe routine */
	xtboot,				/* boot routine */
	xtintr,				/* interrupt routine */
};

/*
 * configuration information declared in conf_xt.c
 */

extern	int	xtmaxunit;		/* number of units */
extern	int	xtmaxctlr;		/* number of controllers */
extern	struct	buf	cxtbuf[];	/* buffer/unit for commands */
extern	struct	buf	xttab[];	/* queue head per controller */
extern	struct	buf	xtutab[];	/* queue head per unit */
extern	struct	xt_unit	xtunits[];	/* unit descriptors */
extern	struct	xt_softc xt_softc[];	/* software state, per unit */
extern	struct	xt_ctlr	xtctlr[];	/* software state, per controller */
#ifdef	FULLXT
extern	int	xtdensel;		/* density selection allowed */
#endif	FULLXT

extern	gate_t	xtgate;			/* gate for interlocks */
extern	lock_t	xtlock[];		/* lock/controller for queues */
#ifdef XTNEEDLOWBUF
extern	caddr_t topmem;			/* highest addressable physmem addr */
#endif XTNEEDLOWBUF

/*
 * local configuration information
 */

struct	mbad_dev	*xt_dev;	/* first of configured devices */
int	xtintr_base;			/* interrupt base level */

#define	INF	(daddr_t)1000000

/*
 * States for xtbuf[ctlr].b_active, the per controller state flag.
 * This is used to sequence control in the driver.
 */

#define	SSEEK	1		/* seeking */
#define	SIO	2		/* doing seq i/o */
#define	SCOM	3		/* sending control command */
#define	SREW	4		/* sending a drive rewind */

#define	b_command	b_resid

/*
 * Determine if there is a controller at this address.
 */

xtprobe(mp)
	register struct mbad_probe *mp;
{
	struct xtdevice *xtaddr;
	short i;

	xtaddr = (struct xtdevice *) &mp->mp_desc->mb_ios->mb_io[mp->mp_csr];
	i = xtaddr->xtreset;
#ifdef lint
	printf("reset status 0x%x\n", i);
	/*
	 *+ Lint Only.
	 */
#endif lint
	DELAY(200);
	if(xtaddr->xtcsr & XT_ADDR24)
		return(1);
	printf("%s controller strapped for 20-bit addressing: not allowed\n",
							zt_driver.mbd_name);
	/*
	 *+ The multibus adaptor cannot address 20 bit devices.
	 *+ The controller needs to be strapped for 24 bit addressing.
	 */
	return(0);
}

xtboot(nxt, md)
	register struct mbad_dev *md;
{
	register struct xt_softc *sc;
	register struct xt_ctlr *cc;
	register ctlr;
	register unit;
	struct xt_unit *up;

	xtintr_base = md->md_vector;
	xt_dev = md;
	/*
	 * Initialize each controller.
	 */
	for(ctlr = 0; ctlr < nxt; ctlr++) {
		md = &xt_dev[ctlr];
		if(ctlr >= xtmaxctlr) {
			if(md->md_alive)
				CPRINTF("%s%d: Not binary configured. Ignored\n",
						zt_driver.mbd_name, ctlr);
			continue;
		}
		cc = &xtctlr[ctlr];
		if(md->md_alive == 0) {
			cc->cc_alive = 0;
			continue;
		}
		cc->cc_alive = 1;
		cc->cc_alloc = 0;
		cc->cc_desc = md->md_desc;
		cc->cc_level = md->md_level;
		cc->cc_ipmap = md->md_basemap;
		cc->cc_dmap  = md->md_basemap + 1;
		cc->cc_nmaps = md->md_nmaps - 1;
		cc->cc_xtaddr = (struct xtdevice *)
			&cc->cc_desc->mb_ios->mb_io[md->md_csr];
		init_lock(&xtlock[ctlr], xtgate);
		init_sema(&cc->cc_tsema, 0, 0, xtgate);
		/*
		 * Guarantee that the iopb can be mapped by
		 * one map register.
		 */
#ifndef lint
		ASSERT(32>=sizeof(struct xt_iopb), "xtboot: one mapreg");
                /*
                 *+ The xt driver has only one mbad map to map the 472
                 *+ iopb.  The iopb structure is expected to be 32 bytes
                 *+ long.
                 */
#endif lint
		callocrnd(32);
		cc->cc_iopb = (struct xt_iopb *)calloc(sizeof(struct xt_iopb));
		cc->cc_iopbaddr = mbad_physmap(cc->cc_desc, cc->cc_ipmap,
				(caddr_t) cc->cc_iopb, sizeof(struct xt_iopb), 1);
	}
	/*
	 * Initialize each drive, handling wild cards in the
	 * configuration description.  If a fully-specified
	 * drive conflicts with an already allocated drive,
	 * or if there aren't any more drives to allocate,
	 * record that the drive isn't alive.
	 *
	 * There's no way to tell if a drive exists unless it has
	 * a tape loaded and on line, so we just assume that
	 * all possible drives exist.
	 */
	for(unit = 0; unit < xtmaxunit; unit++) {
#ifdef	XTDEBUG
		printf("looking for xt%d\n", unit);
#endif
		up = &xtunits[unit];
		sc = &xt_softc[unit];
		if(up->u_ctlr == ANY)
			for(ctlr = 0; ctlr < xtmaxctlr; ctlr++) {
				cc = &xtctlr[ctlr];
				if(cc->cc_alive == 0)
					continue;
				if((up->u_drive==ANY && cc->cc_alloc!=0xFF)||
				   ((cc->cc_alloc & (1 << up->u_drive)) == 0)) {
					up->u_ctlr = ctlr;
					break;
				}
			}
		if(up->u_ctlr == ANY) {
#ifdef	XTDEBUG
			printf("xt%d: no controller available\n", unit);
#endif
			init_sema(&sc->sc_sema, 0, 0, xtgate);
			continue;
		}
		cc = &xtctlr[up->u_ctlr];
		if(up->u_drive == ANY) {
			register drive;

			for(drive = 0; drive < 8; drive++)
				if((cc->cc_alloc & (1 << drive)) == 0) {
					up->u_drive = drive;
					break;
				}
		}
		if(up->u_drive == ANY || (cc->cc_alloc & (1 << up->u_drive))) {
#ifdef	XTDEBUG
			printf("xt%d: no drive available\n", unit);
#endif
			if(up->u_drive != ANY)
				printf("xt%d: drive %d ctlr %d conflict\n",
						up->u_drive, up->u_ctlr);
				/*
				 *+ The specified drive conflicts with
				 *+ another device already configured.
				 */
			init_sema(&sc->sc_sema, 0, 0, xtgate);
			continue;
		}

#ifdef XTNEEDLOWBUF
		/*
		 * allocate buffer space for transfers on machines with
		 * > 64M memory.  This is because Mbad can only access
		 * lower 64M.
		 */
		if (topmem > (caddr_t)MAX_MBAD_ADDR_MEM) {
			int needed = cc->cc_nmaps * MB_MRSIZE;
			caddr_t where;

			callocrnd(CLBYTES);

			/*
			 * If not enough core below 64M to fit the IO
			 * transfer buffer, disable drive.  calloc(0)
			 * gives current allocation pointer, but calloc()
			 * always returns contiguous memory, thus it
			 * needs to be checked again after the allocation
			 */
			if ((calloc(0) + needed > (caddr_t)MAX_MBAD_ADDR_MEM)
				|| (where = calloc(needed))
					> (caddr_t)MAX_MBAD_ADDR_MEM) {

				CPRINTF("xt%d: not enough memory addressable below 64M\n",
								up->u_drive);
				init_sema(&sc->sc_sema, 0, 0, xtgate);
				continue;
			}

			sc->sc_lobuf = where;
			init_sema(&sc->sc_lobufsema, 1, 0, xtgate);
#ifdef XTDEBUG
			printf("lobuf = %x, size = %d\n", sc->sc_lobuf,
					cc->cc_nmaps * MB_MRSIZE);
#endif

		} else
			sc->sc_lobuf = 0;
#endif XTNEEDLOWBUF
		cc->cc_alloc |= 1 << up->u_drive;
		sc->sc_cc = cc;
		CPRINTF("xt%d at %s%d drive %d\n", unit, zt_driver.mbd_name,
						up->u_ctlr, up->u_drive);
		init_sema(&sc->sc_sema, 1, 0, xtgate);
		bufinit(&cxtbuf[unit], xtgate);
	}
}

int	xttimer();

/*
 * Open the device.  Tapes are unique open
 * devices, so we refuse if it is already open.
 * Also check that a tape is available, and
 * that it's not write protected if opening
 * for write.
 *
 * Get the semaphore for this unit, and hold it
 * until the unit is closed.
 */

xtopen(dev, flag)
	dev_t dev;
	int flag;
{
	register int unit;
	register struct xt_softc *sc;
	int dens;

	unit = XTUNIT(dev);
	sc = &xt_softc[unit];

	/*
	 * make sure unit is within the range described
	 * in conf_xt.c.  Return EBUSY for backwards
	 * compatibility.
	 */
	if (unit >= xtmaxunit)
		return(EBUSY);

	/*
	 * autoconfig may allocate space in the xt_softc table for 
	 * a tape drive that doesn't exist but the one before it 
	 * and/or the one after it does.  Check to make sure the structures
	 * have been allocated for the drive before going further.
	 */
	if ((sc->sc_cc->cc_iopb == NULL) || (sc->sc_cc->cc_xtaddr == NULL))
		return (ENXIO);

#ifndef	FULLXT
	if (((minor(dev)&T_INVALID) != T_HIDEN) || !cp_sema(&sc->sc_sema))
		return(EBUSY);
#else
	if (!cp_sema(&sc->sc_sema))
		return(EBUSY);
#endif	FULLXT

	if (sc->sc_tact == 0) {
		sc->sc_tact = 1;
		timeout(xttimer, (caddr_t)xtunits[unit].u_ctlr, 2*hz);
	}
	sc->sc_harderr = 0;
	sc->sc_eot = 0;
	do {
#ifdef	FULLXT
		xtcommand(dev, XT_SPARAM | (minor(dev) & T_HISPEED
				? XT_HIGH : XT_LOW), 1);
#else
		xtcommand(dev, XT_SPARAM | XT_LOW, 1);
#endif	FULLXT
		if(sc->sc_dstat & XTS_REW)
			p_sema(&sc->sc_cc->cc_tsema, PZERO+1);
	} while(sc->sc_dstat & XTS_REW);
	/*
	 * The timer is not needed after the do loop checking for
	 * rewinds. We will now turn off the timer, restore old u.u_qsave,
	 * and clear the flag!
	 */
	untimeout(xttimer, (caddr_t) xtunits[unit].u_ctlr);
	sc->sc_tact = 0;
	if((sc->sc_dstat&(XTS_ONL|XTS_RDY)) != (XTS_ONL|XTS_RDY)) {
		uprintf("xt%d: not online: drive status = 0%o\n",
			unit, sc->sc_dstat);
                /*
                 *+ The requested tape drive is not online and ready.
                 *+ Corrective action:  put the tape drive online.
                 */
		v_sema(&sc->sc_sema);
		return(EIO);
	}
	if((flag&FWRITE) && (sc->sc_dstat&XTS_FPT)) {
		uprintf("xt%d: no write ring\n", unit);
                /*
                 *+ The requested tape drive is loaded with a tape that has
                 *+ no write ring.  Corrective action:  unload the tape and
                 *+ attach a write ring to the reel.
                 */
		v_sema(&sc->sc_sema);
		return(EIO);
	}
#ifdef	FULLXT
	dens = minor(dev) & (T_HIDEN | T_SWAB);
#else
	dens = 0;
#endif	FULLXT
	if((sc->sc_dstat&XTS_BOT) == 0 && (flag&FWRITE) &&
	    dens != sc->sc_dens) {
		uprintf("xt%d: can't change density in mid-tape\n", unit);
                /*
                 *+ After the tape density has been selected and the tape has
                 *+ been written or read, the density cannot be changed.
                 *+ Corrective action:  don't request a density change.
                 */
		v_sema(&sc->sc_sema);
		return(EIO);
	}
#ifdef	FULLXT
	if(xtdensel && sc->sc_dstat & XTS_BOT)
		xtcommand(dev, XT_SPARAM | (dens&T_HIDEN ? XT_GCR : XT_PE), 1);
#endif	FULLXT
	sc->sc_blkno = (daddr_t)0;
	sc->sc_nxrec = INF;
	sc->sc_lastiow = 0;
	sc->sc_dens = dens;
#ifdef	XTDEBUG
	if(xtdebug > 2)
		printf("open OK: dens 0x%x\n", dens);
#endif
	return(0);
}

/*
 * Close tape device.
 *
 * If tape was open for just writing or last operation was
 * a write, then write two EOF's and backspace over the last one.
 * Unless this is a non-rewinding special file, rewind the tape.
 * Make the tape available to others.
 */

xtclose(dev, flag)
	register dev_t dev;
	register flag;
{
	register struct xt_softc *sc = &xt_softc[XTUNIT(dev)];

#ifdef	XTDEBUG
	if(xtdebug > 1)
		printf("xtxlose dev 0x%x\n", dev);
#endif
	sc->sc_harderr = 0;
	sc->sc_eot = 0;
	if (((flag & FWRITE) || (flag & FAPPEND)) && sc->sc_lastiow) {
		xtcommand(dev, XT_FMARK | XT_RETY, 1);
		xtcommand(dev, XT_FMARK | XT_RETY, 1);
		xtcommand(dev, XT_SEEK | XT_REC | XT_REV, 1);
	}
	if(((minor(dev)&T_NOREWIND) == 0) && sc->sc_offline == 0) {
		xtcommand(dev, XT_SEEK | XT_REW, 1);
		sc->sc_fileno = 0;
	} else {
		if (sc->sc_blkno != 0)
			sc->sc_fileno += 1;
	}
	sc->sc_offline = 0;
	v_sema(&sc->sc_sema);
	return(0);
}

/*
 * Execute a command on the tape drive
 * a specified number of times.
 */

xtcommand(dev, com, count)
	dev_t dev;
	int com, count;
{
	register struct buf *bp;

	bp = &cxtbuf[XTUNIT(dev)];
#ifdef	XTDEBUG
	if(xtdebug > 1)
		printf("xtcommand(0x%x, 0x%x, %d) bp 0x%x, unit %d ...", dev,
			com, count, bp, XTUNIT(dev));
	else if(xtdebug)
		printf("c");
#endif
	bufalloc(bp);
#ifdef	XTDEBUG
	if(xtdebug > 1)
		printf("got sema\n");
#endif
	bp->b_flags &= ~B_ERROR;
	bp->b_dev = dev;
	bp->b_bcount = count;
	bp->b_command = com;
	bp->b_blkno = 0;
	BIODONE(bp) = 0;
	xtstrat(bp);
	biowait(bp);
	bp->b_flags &= B_ERROR;
#ifdef	XTDEBUG
	if(xtdebug > 1)
		printf("xtcommand done\n");
#endif
	buffree(bp);
}

/*
 * Queue a tape operation.
 */

xtstrat(bp)
	register struct buf *bp;
{
	int unit = XTUNIT(bp->b_dev);
#ifdef XTNEEDLOWBUF
	register struct xt_softc *sc = &xt_softc[unit];
#endif
	register struct buf *dp;
	register struct buf *cbp;
	register struct xt_unit *up;
	int ctlr;
	int s;

	/*
	 * Put transfer at end of unit queue.
	 * Get the lock for this controller to manipulate
	 * the queues.
	 */
	up = &xtunits[unit];
	ctlr = up->u_ctlr;
	dp = &xtutab[unit];
	cbp = &xttab[ctlr];
	s = p_lock(&xtlock[ctlr], SPL5);

#ifdef XTNEEDLOWBUF
	/*
	 * if this machine has more than 64M, need to copy the buffer to
	 * an area readable by Multibus.
	 */
	if ((sc->sc_lobuf) && (bp != &cxtbuf[unit])) {
		p_sema_v_lock(&sc->sc_lobufsema, PRIBIO, &xtlock[ctlr], s);
		s = p_lock(&xtlock[ctlr], SPL5);
		sc->sc_buflocked = bp;

#ifdef XTDEBUG
		if (xtdebug > 1)
			printf("xtstrat: doing bp swizzle\n");
#endif
		if ((bp->b_flags & B_READ) == B_WRITE && bp->b_bcount) {
			if ((bp->b_flags & B_PHYS) == B_PHYS) {
#ifdef XTDEBUG
				if (xtdebug > 1)
					printf("xtstrat: doing copyin\n");
#endif
				if (copyin(bp->b_un.b_addr, sc->sc_lobuf,
							(u_int)bp->b_bcount)) {
					bp->b_flags |= B_ERROR;
					bp->b_error = EFAULT;
					v_sema(&sc->sc_lobufsema);
					biodone(bp);
					v_lock(&xtlock[ctlr], s);
					return;
				}
			} else {
				bcopy((caddr_t)bp->b_un.b_addr,
				      (caddr_t)sc->sc_lobuf,
				      (unsigned)bp->b_bcount);
			}
		}
		sc->sc_savbuf = bp->b_un.b_addr;
		bp->b_un.b_addr = sc->sc_lobuf;
#ifdef XTDEBUG
		if (xtdebug > 1)
			printf("sc_savbuf=0x%x\n", sc->sc_savbuf);
#endif
		/*
		 * need to tell mbad_setup that this is a kernel-virtual
		 * address.
		 */
		bp->b_iotype = B_FILIO;
	}
#endif XTNEEDLOWBUF
				
	bp->av_forw = NULL;
#ifdef	XTDEBUG
	if(xtdebug > 1)
		printf("xtstrategy(0x%x) %s: ctlr %d unit %d\n", bp,
				bp->b_flags & B_READ ? "read" : "write",
				ctlr, unit);
	else if(xtdebug)
		printf(bp->b_flags & B_READ ? "r" : "w");
#endif
	if(dp->b_actf == NULL) {
		dp->b_actf = bp;
		/*
		 * Transport not already active...
		 * put at end of controller queue.
		 */
#ifdef	XTDEBUG
		if(xtdebug > 2)
			printf("transport free\n");
#endif
		dp->b_forw = NULL;
		if(cbp->b_actf == NULL) {
#ifdef	XTDEBUG
			if(xtdebug > 2)
				printf("controller free\n");
#endif
			cbp->b_actf = dp;
		} else
			cbp->b_actl->b_forw = dp;
		cbp->b_actl = dp;
	} else
		dp->b_actl->av_forw = bp;
	dp->b_actl = bp;
	/*
	 * If the controller is not busy, get
	 * it going.
	 */
	if(cbp->b_active == 0)
		xtstart(ctlr);
#ifdef	XTDEBUG
	else if(xtdebug > 2)
		printf("controller busy - no restart\n");
#endif

#ifdef XTNEEDLOWBUF
	/*
	 * machines with > 64M need to do i/o in locore.  Do the copyout
	 * back to user space if this was a read on the raw device.
	 * Block device reads need this copying to be done in xtintr().
	 */
	if ((bp->b_flags & B_PHYS) && (sc->sc_lobuf) && (bp != &cxtbuf[unit])) {

		/*
		 * wait for the IO to complete.  xtintr() should V
		 * this semaphore when it is done.
		 */
		p_sema_v_lock(&bp->b_iowait, PRIBIO, &xtlock[ctlr], s);
		s = p_lock(&xtlock[ctlr], SPL5);

		bp->b_un.b_addr = sc->sc_savbuf;
		if ((bp->b_flags & B_READ) == B_READ && sc->sc_acnt) {
#ifdef XTDEBUG
			printf("xtstrat:copyout(0x%x,0x%x,0x%x)\n",
			sc->sc_lobuf, bp->b_un.b_addr,(u_int)sc->sc_acnt);
#endif
			if (copyout((caddr_t)sc->sc_lobuf,
				(caddr_t)bp->b_un.b_addr,
					(u_int)sc->sc_acnt) == EFAULT) {
				bp->b_flags |= B_ERROR;
				bp->b_error = EFAULT;
			}
		}
		v_sema(&sc->sc_lobufsema);
		biodone(bp);
	}
#endif XTNEEDLOWBUF
	v_lock(&xtlock[ctlr], s);
}

/*
 * Start activity on a controller.
 * The lock is held by caller.
 */

xtstart(ctlr)
{
	register struct xt_ctlr *cc;
	register struct buf *bp;
	register struct buf *dp;
	register struct xt_softc *sc;
	register struct buf *cbp;
	struct xt_unit *up;
	int unit;
	int cmd;
	int cnt;
	daddr_t blkno;

	cc = &xtctlr[ctlr];
	cbp = &xttab[ctlr];
	/*
	 * Look for an idle transport on the controller.
	 */
loop:
#ifdef	XTDEBUG
	if(xtdebug > 1) 
		printf("xtstart at loop: ctlr %d, cc 0x%x\n", ctlr, cc);
	else if(xtdebug)
		printf("s");
#endif
	if((dp = cbp->b_actf) == NULL)
		return;
	if((bp = dp->b_actf) == NULL) {
		cbp->b_actf = dp->b_forw;
		goto loop;
	}
	unit = XTUNIT(bp->b_dev);
	sc = &xt_softc[unit];
	up = &xtunits[unit];
	ASSERT(up->u_ctlr==ctlr, "xtstart: ctlr");
        /*
         *+ The controller state structure passed to xtstart() does not
         *+ match the controller state structure indexed by this drive.
         */
	/*
	 * Default is that last command was NOT a write command;
	 * if we do a write command we will notice this in xtintr().
	 */
	if(sc->sc_harderr) {
		/*
		 * Have had a hard error on a non-raw tape.
		 */
#ifdef	XTDEBUG
		if(xtdebug > 1)
			printf("hard error pre-abort\n");
#endif
		bp->b_flags |= B_ERROR;
		if (sc->sc_eot)
			bp->b_error = ENOSPC;
		else
			bp->b_error = EIO;
		goto next;
	}
	sc->sc_lastiow = 0;
	if(bp == &cxtbuf[unit]) {
		/*
		 * Execute control operation with the specified count.
		 * Set next state; give 5 minutes to complete
		 * rewind, or 10 seconds per iteration (minimum 60
		 * seconds and max 5 minutes) to complete other ops.
		 */
		if(bp->b_command == (XT_SEEK | XT_REW))
			cbp->b_active = SREW;
		else
			cbp->b_active = SCOM;
#ifdef	XTDEBUG
		if(xtdebug > 2)
			printf("cxtbuf: cmd 0x%x count %d\n", bp->b_command,
								bp->b_bcount);
#endif
		cnt = bp->b_bcount;
		goto dobpcmd;
	}
	/*
	 * The following checks handle boundary cases for operation
	 * on non-raw tapes.  On raw tapes the initialization of
	 * sc->sc_nxrec by xtphys causes them to be skipped normally
	 * (except in the case of retries).
	 */
	if(bdbtofsb(bp->b_blkno) > sc->sc_nxrec) {
		/*
		 * Can't read past known end-of-file.
		 */
#ifdef	XTDEBUG
		if(xtdebug > 1)
			printf("EOF pre-abort\n");
#endif
		bp->b_flags |= B_ERROR;
		bp->b_error = ENXIO;
		goto next;
	}
	if(bdbtofsb(bp->b_blkno) == sc->sc_nxrec &&
	    (bp->b_flags & B_READ) == B_READ) {
		/*
		 * Reading at end of file returns 0 bytes.
		 */
#ifdef	XTDEBUG
		if(xtdebug > 1)
			printf("EOF 0 bytes\n");
#endif
		clrbuf(bp);
		/* bp->b_resid = bp->b_bcount; */
		goto next;
	}
	if((bp->b_flags & B_READ) == B_WRITE)
		/*
		 * Writing sets EOF
		 */
		sc->sc_nxrec = bdbtofsb(bp->b_blkno) + 1;
	/*
	 * If the data transfer command is in the correct place,
	 * set up all the parameters for xtgo.
	 */
	if((blkno = sc->sc_blkno) == bdbtofsb(bp->b_blkno)) {
		cbp->b_active = SIO;
		if((bp->b_flags&B_READ) == B_WRITE)
			cmd = XT_WRITE | XT_RETY;
		else
			cmd = XT_READ | XT_RETY;
#ifdef	FULLXT
		if(sc->sc_dens & T_SWAB)
			cmd |= XT_SWAP;
#endif	FULLXT
#ifdef	XTDEBUG
		if(xtdebug > 1)
			printf("data: cmd 0x%x\n", cmd);
#endif
		xtgo(cc, bp, cmd, up->u_drive, 0);
		return;
	}
	/*
	 * Tape positioned incorrectly;
	 * set to position forwards or backwards to the correct spot.
	 * This happens for raw tapes only on error retries.
	 */
	cbp->b_active = SSEEK;
	if(blkno < bdbtofsb(bp->b_blkno)) {
		bp->b_command = XT_SEEK | XT_REC;
		cnt = bdbtofsb(bp->b_blkno) - blkno;
	} else {
		bp->b_command = XT_SEEK | XT_REC | XT_REV;
		cnt = blkno - bdbtofsb(bp->b_blkno);
	}
dobpcmd:
	/*
	 * Do the command in bp.
	 */
#ifdef	XTDEBUG
	if(xtdebug > 2)
		printf("bp cmd: 0x%x cnt %d\n", bp->b_command, bp->b_bcount);
#endif
	xtgo(cc, bp, (int)bp->b_command, up->u_drive, cnt);
	return;

next:
	/*
	 * Done with this operation due to error or
	 * the fact that it doesn't do anything.
	 * Dequeue the transfer and continue processing this slave.
	 */
	dp->b_actf = bp->av_forw;
#ifdef XTNEEDLOWBUF
 	/*
 	 * Need to release the semaphore around the lowcore buffer, since
 	 * no operation is being done with it.  Do this only for block i/o.  
 	 */
 	if ((bp == sc->sc_buflocked) && ((bp->b_flags & B_PHYS) == 0)) {
 		bp->b_un.b_addr = sc->sc_savbuf;
 		v_sema(&sc->sc_lobufsema);
 	}
#endif XTNEEDLOWBUF
	biodone(bp);
	goto loop;
}

/*
 * Stuff the iopb and start the device.
 */

xtgo(cc, bp, cmd, unit, cnt)
	register struct xt_ctlr *cc;
	register struct buf *bp;
{
	register struct xtdevice *xtaddr;
	register struct xt_iopb *ip;
	unsigned mbaddr;

	xtaddr = cc->cc_xtaddr;
	ip = cc->cc_iopb;
	SETIOPBADDR(xtaddr, cc->cc_iopbaddr);
	ip->io_scomm = cmd | XT_IEN | XT_RELO | XT_AUD;
	ip->io_status = 0;
	ip->io_compcode = 0;
	ip->io_imode = XTS_IEI;
	ip->io_throt = XTT_T64y;
	ip->io_drive = unit;
	if( (cmd & XT_COMMAND) == XT_SEEK )
		ip->io_cnt = cnt;
	else
		ip->io_cnt = bp->b_bcount;
	if( (bp==&cxtbuf[XTUNIT(bp->b_dev)]) || ((cmd & XT_COMMAND)==XT_SEEK) )
		mbaddr = 0;
	else
		mbaddr = mbad_setup(bp, cc->cc_desc, cc->cc_dmap, cc->cc_nmaps);
	SETBADDR(ip, mbaddr);
	ip->io_acnt = 0;
#ifdef	XTDEBUG
	if(xtdebug > 1) {
		printf("xtgo\n");
		xtdumpiopb(ip);
		printf("iopbaddr 0x%x, csr before 0x%x, ", GETIOPBADDR(xtaddr),
								xtaddr->xtcsr);
	} else if(xtdebug)
		printf("g");
#endif
	xtaddr->xtcsr = XT_GO;
#ifdef	XTDEBUG
	if(xtdebug > 1)
		printf("after 0x%x\n", xtaddr->xtcsr);
#endif
	mbad_reenable(cc->cc_desc, cc->cc_level);
}

/*
 * xt interrupt routine.
 */

xtintr(level)
	int level;
{
	struct buf *dp;
	struct buf *bp;
	int ctlr;
	register struct xtdevice *xtaddr;
	register struct xt_softc *sc;
	register struct xt_iopb *ip;
	register struct xt_ctlr *cc;
	struct buf *cbp;
	int unit;
	int state;
	spl_t s;

	ctlr = level - xtintr_base;
	if(ctlr < 0 || ctlr >= xtmaxctlr) {
		printf("xtintr: unknown interrupt at level %d\n", level);
		/*
		 *+ An unexpected response was received from the Xylogics 
		 *+ 472 board.
		 */
		return;
	}
#ifdef	XTDEBUG
	if(xtdebug > 1)
		printf("xtintr(%d) controller %d\n", level, ctlr);
	else if(xtdebug)
		printf("I");
#endif
	cc = &xtctlr[ctlr];
	xtaddr = cc->cc_xtaddr;
	cbp = &xttab[ctlr];
	s = p_lock(&xtlock[ctlr], SPL5);
	if((dp = cbp->b_actf) == NULL) {
		printf("%s%d: extraneous interrupt\n", zt_driver.mbd_name, ctlr);
		/*
		 *+ An unexpected response was received from the Xylogics 
		 *+ 472 board.
		 */
		xtaddr->xtcsr = XT_IPND;
		v_lock(&xtlock[ctlr], s);
		return;
	}
	bp = dp->b_actf;
	unit = XTUNIT(bp->b_dev);
	sc = &xt_softc[unit];
	ip = cc->cc_iopb;
#ifdef	XTDEBUG
	if(xtdebug > 2)
		xtdumpiopb(ip);
#endif
	sc->sc_status = ip->io_status;
	sc->sc_dstat  = ip->io_dstat;
	if((ip->io_scomm & XT_COMMAND) == XT_READ ||
	    (ip->io_scomm & XT_COMMAND) == XT_SEEK)
		sc->sc_acnt = ip->io_acnt;
	else
		sc->sc_acnt = bp->b_bcount;
#ifdef	XTDEBUG
	if(xtdebug >1)
		printf("cmd 0x%x io_acnt %d, sc_acnt %d\n", ip->io_scomm,
						sc->sc_acnt, ip->io_acnt);
#endif
	sc->sc_resid = bp->b_bcount - sc->sc_acnt;
	if((ip->io_scomm & XT_COMMAND) == XT_WRITE ||
	   (ip->io_scomm & XT_COMMAND) == XT_FMARK)
		sc->sc_lastiow = 1;
	state = cbp->b_active;
	cbp->b_active = 0;
	/*
	 * Check for errors.
	 */
	if(xtaddr->xtcsr&(XT_ERR|XT_DERR)) {
		xtaddr->xtcsr = XT_ERR;
		/*
		 * If we hit the end of the tape file, update our position.
		 */
		if(ip->io_compcode == XTC_TMREAD) {
			xtseteof(bp);
			state = SCOM;
			sc->sc_resid = bp->b_bcount;
			goto opdone;
		}
		/*
		 * If the record was short or if it's a
		 * recovered data error, then no error
		 * has occurred.
		 */
		if(ip->io_compcode == XTC_SHORT)
			goto noerror;
		if(ip->io_compcode == XTC_CORR) {
			printf("xt%d: recovered error bn%d\n", unit,
								bp->b_blkno);
			/*
			 *+ The Xylogic 472 detected but corrected a data
			 *+ error.
			 */
			goto noerror;
		}
		/*
		 * If we were reading raw tape and the only error was that the
		 * record was too long, then we don't consider this an error.
		 */
		if( bp->b_iotype == B_PHYS && (bp->b_flags&B_READ) == B_READ &&
		    ip->io_compcode == XTC_LONG)
			goto noerror;

		/*
		 * If we are reading and an EOT sticker is detected, ignore it.
		 */
		if((bp->b_flags&B_READ) == B_READ && ip->io_compcode == XTC_EOT)
			goto noerror;

		/*
		 * Hard or non-i/o errors on non-raw tape
		 * cause it to close.
		 */
		if(sc->sc_harderr == 0 && bp->b_iotype != B_PHYS &&
		    bp != &cxtbuf[unit])
			sc->sc_harderr = 1;
		/*
		 * If we hit EOT on write, do not report failure.
		 * Since sc->sc_harderr was set to 1, the EOT
		 * situation will be reported on the *next* write
		 * as handled in xtstart().
		 */
		if (ip->io_compcode == XTC_EOT) {
			sc->sc_eot = 1;
			goto opdone;
		}
		/*
		 * Couldn't recover error
		 */
		printf("xt%d: hard error bn%d er=0x%x\n", unit, bp->b_blkno,
							ip->io_compcode);
		/*
		 *+ The Xylogics 472 board detected a hard error.
		 */
		bp->b_flags |= B_ERROR;
		goto opdone;
	}
	/*
	 * Advance tape control FSM.
	 */
noerror:
	switch (state) {

	case SIO:
		/*
		 * Read/write increments tape block number
		 */
		sc->sc_blkno++;
		goto opdone;

	/*
	 * For positioning commands, update current position.
	 */
	case SREW:
		sc->sc_blkno = 0;
		goto opdone;

	case SCOM:
		if(bp == &cxtbuf[unit])
			switch (bp->b_command) {

			case XT_SEEK|XT_REC:
				sc->sc_blkno += bp->b_bcount;
				break;

			case XT_SEEK|XT_REC|XT_REV:
				sc->sc_blkno -= bp->b_bcount;
				break;
			}
		goto opdone;

	case SSEEK:
		sc->sc_blkno = bdbtofsb(bp->b_blkno);
		goto opcont;

	default:
		panic("xtintr");
                /*
                 *+ The tape control finite state machine is in
                 *+ an illegal state.
                 */
	}
opdone:
	/*
	 * Reset error count and remove
	 * from device queue.
	 */
	dp->b_actf = bp->av_forw;
	bp->b_resid = sc->sc_resid;
#ifdef XTNEEDLOWBUF
	/*
	 * If this was block I/O on a machine with > 64M memory,
	 * we need to un-swap the I/O buffers, and bcopy out the
	 * contents of the buffer (if the operation was a read).
	 */
	if (sc->sc_lobuf && bp != &cxtbuf[unit] &&
		bp == sc->sc_buflocked && (bp->b_flags & B_PHYS) == 0) {
		bp->b_un.b_addr = sc->sc_savbuf;

		if ((bp->b_flags & B_READ) == B_READ && sc->sc_acnt)
			bcopy((caddr_t)sc->sc_lobuf,
			      (caddr_t)bp->b_un.b_addr,
			      (unsigned)sc->sc_acnt);

		v_sema(&sc->sc_lobufsema);
	}
#endif XTNEEDLOWBUF

	biodone(bp);
	/*
	 * Circulate slave to end of controller
	 * queue to give other slaves a chance.
	 */
	cbp->b_actf = dp->b_forw;
	if(dp->b_actf) {
		dp->b_forw = NULL;
		if(cbp->b_actf == NULL)
			cbp->b_actf = dp;
		else
			cbp->b_actl->b_forw = dp;
		cbp->b_actl = dp;
	}
	if(cbp->b_actf == NULL) {
		v_lock(&xtlock[ctlr], s);
		xtaddr->xtcsr = XT_IPND;
		return;
	}
opcont:
	xtaddr->xtcsr = XT_IPND;
	xtstart(ctlr);
	v_lock(&xtlock[ctlr], s);
}

xttimer(ctlr) {
	register struct xt_ctlr *cc;

	cc = &xtctlr[ctlr];
#ifdef	XTDEBUG
	if(xtdebug > 4) {
		register struct xtdevice *xtaddr;

		printf("xttimer(%d) cc 0x%x\n", ctlr, cc);
		xtaddr = cc->cc_xtaddr;
		printf("xtaddr 0x%x\n", xtaddr);
		printf("csr 0x%x\n", xtaddr->xtcsr);
		printf("iopbaddr 0x%x\n", GETIOPBADDR(xtaddr));
		xtdumpiopb(cc->cc_iopb);
	}
#endif
	(void) cv_sema(&cc->cc_tsema);
	timeout(xttimer, (caddr_t)ctlr, 2*hz);
}

xtseteof(bp)
	register struct buf *bp;
{
	register int unit = XTUNIT(bp->b_dev);
	register struct xt_softc *sc = &xt_softc[unit];

	if(bp == &cxtbuf[unit]) {
		if(sc->sc_blkno > bdbtofsb(bp->b_blkno)) {
			/* reversing */
			sc->sc_nxrec = bdbtofsb(bp->b_blkno) - sc->sc_acnt;
			sc->sc_blkno = sc->sc_nxrec;
		} else {
			/* spacing forward */
			sc->sc_blkno = bdbtofsb(bp->b_blkno) + sc->sc_acnt;
			sc->sc_nxrec = sc->sc_blkno - 1;
		}
		return;
	} 
	/* eof on read */
	sc->sc_nxrec = bdbtofsb(bp->b_blkno);
}

/*
 * The io_acnt is an unsifned short - so truncated request to that.
 */

xtminphys(bp)
	struct buf *bp;
{
	if (bp->b_bcount > 0xffff)
		bp->b_bcount = 0xffff;
	return(bp->b_bcount);
}

xtread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register err;

	err = xtphys(dev, uio);
	if(err)
		return(err);
	return(physio(xtstrat, (struct buf *)0, dev, B_READ, xtminphys, uio));
}

xtwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register err;

	err = xtphys(dev, uio);
	if(err)
		return(err);
	return(physio(xtstrat, (struct buf *)0, dev, B_WRITE, xtminphys, uio));
}

/*
 * Verify that the requested transfer can be accomodated
 * by the number of map registers allocated.
 * If it's ok, set up sc_blkno and sc_nxrec
 * so that the tape will appear positioned correctly.
 */

static
xtphys(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register daddr_t a;
	register struct xt_softc *sc;
	register unsigned off;

	sc = &xt_softc[XTUNIT(dev)];
	/*
	 * Insure request isn't too large for mapping registers.
	 * This mimics case B_RAWIO in mbad_setup() and is independent of
	 * page size.
	 */
	off = (long) uio->uio_iov->iov_base & (MB_MRSIZE-1);
	if (((off+uio->uio_resid+MB_MRSIZE-1)/MB_MRSIZE) > sc->sc_cc->cc_nmaps) {
		printf("xt%d: not enough maps for transfer\n",XTUNIT(dev));
		/*
		 *+ The driver requires a given number of MULTIBUS mapping 
		 *+ registers in order to communicate with the Xylogics 472 
		 *+ board.  Too few mapping registers were allocated.  
		 *+ Corrective action: edit the system.std file to request 
		 *+ the proper number of maps and rebuild the kernel.  
		 */
		return(ENXIO);
	}
	a = bdbtofsb(uio->uio_offset >> 9);
	sc->sc_blkno = a;
	sc->sc_nxrec = a + 1;
	return(0);
}

/*
 * N.B.:  This depends on the values of the MT codes here
 */

static	short	xtops[] = {
	XT_FMARK|XT_RETY,		/* MTWEOF - write file mark */
	XT_SEEK|XT_FILE,		/* MTFSF  - forward space file */
	XT_SEEK|XT_FILE|XT_REV,		/* MTBSF  - backspace file */
	XT_SEEK|XT_REC,			/* MTFSR  - forward space record */
	XT_SEEK|XT_REC|XT_REV,		/* MTBSR  - backspace record */
	XT_SEEK|XT_REW,			/* MTREW  - rewind */
	XT_SEEK|XT_UNLOAD,		/* MTOFFL - rewind and offline */
	XT_DSTAT			/* MTNOP  - no operation - set status */
};

#define	NXTOPS	(sizeof (xtops) / sizeof (xtops[0]))

/*ARGSUSED*/
xtioctl(dev, cmd, data, flag)
	caddr_t data;
	dev_t dev;
{
	int unit = XTUNIT(dev);
	register struct xt_softc *sc = &xt_softc[unit];
	register struct buf *bp = &cxtbuf[unit];
	register callcount;
	int fcount;
	int tmp_fcount;
	struct mtop *mtop;
	struct mtget *mtget;

#ifdef	XTDEBUG
	if(xtdebug > 1)
		printf("xtioctl(0x%x, 0x%x, 0x%x, 0x%x)\n", dev, cmd, data,
									flag);
#endif
	switch (cmd) {

	case MTIOCTOP:	/* tape operation */
		mtop = (struct mtop *) data;
#ifdef	XTDEBUG
		if(xtdebug > 1)
			printf("mtop %d\n", mtop->mt_op);
#endif
		if(mtop->mt_op >= NXTOPS)
			return(EINVAL);
		switch(mtop->mt_op) {

		case MTWEOF:
			callcount = mtop->mt_count;
			fcount = 1;
			tmp_fcount = sc->sc_fileno + 1;
			break;

		case MTFSF:
		case MTBSF:
		case MTFSR:
		case MTBSR:
			callcount = 1;
			fcount = mtop->mt_count;
			tmp_fcount = sc->sc_fileno + fcount;
			break;

		case MTOFFL:
			sc->sc_offline = 1;
		case MTREW:
			tmp_fcount = 0;
			callcount = 1;
			fcount = 1;
			break;
		case MTNOP:
			tmp_fcount = sc->sc_fileno;
			callcount = 1;
			fcount = 1;
			break;

		default:
			return(ENXIO);
		}
		if(callcount <= 0 || fcount <= 0)
			return(ENXIO);
		while (--callcount >= 0) {
			xtcommand(dev, xtops[mtop->mt_op], fcount);
			if((mtop->mt_op == MTFSR || mtop->mt_op == MTBSR) &&
			    bp->b_resid) {
				bp->b_flags |= B_ERROR;
				bp->b_error = EIO;
				break;
			}
			if((bp->b_flags&B_ERROR) || sc->sc_dstat & XTS_BOT)
				break;
		}
		if (! (bp->b_flags&B_ERROR)) {
			sc->sc_fileno = tmp_fcount;
		}
		return(geterror(bp));

	case MTIOCGET:
		mtget = (struct mtget *) data;
		mtget->mt_dsreg = sc->sc_dstat;
		mtget->mt_erreg = sc->sc_status;
		mtget->mt_resid = sc->sc_resid;
		mtget->mt_fileno = sc->sc_fileno;
		mtget->mt_blkno = sc->sc_blkno;
		mtget->mt_type = MT_ISXT;
		break;

	default:
		return(ENXIO);
	}
	return(0);
}

#ifdef	XTDEBUG
xtdumpiopb(ip)
	register struct xt_iopb *ip;
{
	printf("io_scomm	0x%x\n", ip->io_scomm);
	printf("io_status	0x%x\n", ip->io_status);
	printf("io_compcode	0x%x\n", ip->io_compcode);
	printf("io_dstat	0x%x\n", ip->io_dstat);
	printf("io_imode	0x%x\n", ip->io_imode);
	printf("io_throt	0x%x\n", ip->io_throt);
	printf("io_drive	0x%x\n", ip->io_drive);
	printf("io_cnt		0x%x\n", ip->io_cnt);
	printf("io_baddr	0x%x\n", ip->io_baddr);
	printf("io_xbaddr	0x%x\n", ip->io_xbaddr);
	printf("io_niop		0x%x\n", ip->io_niop);
	printf("io_acnt		0x%x\n", ip->io_acnt);
};
#endif
