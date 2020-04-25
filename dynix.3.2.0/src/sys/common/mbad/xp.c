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
static	char	rcsid[] = "$Header: xp.c 2.18 1991/04/24 18:54:11 $";
#endif

/*
 * xp.c
 *
 * Xylogics 450 SMD Disk Driver
 */

/* $Log: xp.c,v $
 *
 *
 */

#ifdef XPDEBUG
int	xpdebug = 3;

#define	NTRACE	1000
char	xptrace[NTRACE+1];
char	*xptracep	= xptrace;
#define	TRACE(c) {			\
	*xptracep++ = (c);		\
	if(xptracep >= &xptrace[NTRACE])\
		xptracep = xptrace;	\
	*xptracep = '\0';		\
}

char tracech[] = "0123456789abcdef";

TRACEN(n, base) {
	int m = n/base;
	if (m) TRACEN(m, base);
	TRACE(tracech[n%base]);
}
#else
#define	TRACE(x)
#define TRACEN(x, base)
#endif XPDEBUG

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/uio.h"
#include "../h/systm.h"
#include "../h/kernel.h"
#include "../h/vm.h"
#include "../h/dk.h"
#include "../h/vtoc.h"
#include "../h/file.h"
#include "../h/cmn_err.h"

#include "../balance/slicreg.h"
#include "../balance/clkarb.h"		/* For front-panel leds */

#include "../machine/ioconf.h"		/* IO Configuration Definitions */
#include "../machine/pte.h"
#include "../machine/intctl.h"

#include "../mbad/mbad.h"		/* Multibus interface definitions */
#include "../mbad/xp.h"
#include "../mbad/dkbad.h"

#ifdef XPDEBUG
/*
 * Get enough stuff defined to ref l.me to keep interrupt count stat.
 */
#include "../machine/plocal.h"
int	xppint[MAXNUMCPU];		/* count # intr's per processor */
#endif	XPDEBUG

#define b_diskaddr      b_resid
#define b_psect         b_error

/*
 * locals 
 */

int	xpc_dse;			/* counter for dse errors */
int	xpc_slack;			/* counter for slack errors */
int	xpintr_base;			/* base interrupt vector */
int	xpmaxctlr;			/* number of controllers */
struct xp_softc *xp_softc;		/* pointer to soft state */

struct	xp_iopb	*xpmakeiopb();
int	xptimer();

/*
 * Structure to define this driver to configuration code.
 */

int	xpprobe(), xpboot(), xpintr(), xpstrat(), xpcstrat();

struct	mbad_driver xy_driver = {
	"xy",				/* name */
	MBD_TYPICAL,			/* configuration flags */
	xpprobe,			/* probe procedure */
	xpboot,				/* boot procedure */
	xpintr,				/* intr procedure */
};

/*
 * completion codes
 */

char	*xp_compcodes[] = {
	"successful completion",
	"interrupt pending",
	"0x02 reserved",
	"busy conflict",
	"operation timeout",
	"header not found",
	"hard ECC",
	"illegal cylinder address",
	"0x08 reserved",
	"sector slip command error",
	"illegal sector address",
	"0x0B reserved",
	"0x0C reserved",
	"last sector too small",
	"slave ACK error (NXM)",
	"0x0F reserved",
	"0x10 reserved",
	"0x11 reserved",
	"cylinder/head header error",
	"auto seek retry successful",
	"write protect error",
	"unimplemented command",
	"drive not ready",
	"sector count zero",
	"drive faulted",
	"illegal sector size",
	"self test A",
	"self test B",
	"self test C",
	"0x1D reserved",
	"soft ECC error",
	"recovered ECC error",
	"illegal head",
	"disk sequencer error",
	"0x22 reserved",
	"0x23 reserved",
	"0x24 reserved",
	"seek error",
};

#define	NCOMPCODES	(sizeof(xp_compcodes) / sizeof(xp_compcodes[0]))

/*
 * xpprobe()
 *	See if a given xy board exists.
 *
 * Called for each board until it is found on some multibus.
 *
 * If we try to talk to it and the reset register isn't on the MBAd,
 * it generates an NMI which is handled by the MBAd probing software.
 *
 * Returns `true' if board exists and is ok, `false' if not.
 */

xpprobe(mp)
register struct mbad_probe *mp;
{
	register struct xpdevice *xpaddr;
	u_char	status;

	xpaddr = (struct xpdevice *)&mp->mp_desc->mb_ios->mb_io[mp->mp_csr];
	status = xpaddr->xpreset;
#ifdef lint
	printf("reset status 0x%x\n", status);
	/*
	 *+ Lint only.
	 */
#endif lint
	DELAY(200);
	if(xpaddr->xpcsr & XP_ADDR24)
		return(1);
	printf("xy controller strapped for 20-bit addressing: not allowed\n");
	/*
         *+ The multibus adaptor cannot address 20 bit devices.
         *+ The controller needs to be strapped for 24 bit addressing.
         */
	return(0);
}

/* 
 * xpboot()
 *	Called after all xy devices have been probed.
 *
 * Assumes mono-processor environment and no interrupt
 * capability.
 *
 * This procedure is called *once* at system boot time.
 */

xpboot(nxp, xpdevs)
int nxp;
register struct	mbad_dev *xpdevs;
{
	register struct	xp_unit *up;
	register int ctlr;
	register int unit;
	struct xp_iopb tempiopb;
	register struct xp_iopb *ip;
	register drive;
	register struct	mbad_dev *md;
	struct xp_softc *sc;
	struct v_open *vo;
	caddr_t calloc();

	xpintr_base = xpdevs[0].md_vector;
	xpmaxctlr = nxp;
	xp_softc = (struct xp_softc *)calloc(nxp*sizeof(struct xp_softc));
	/*
	 * Initialize each controller
	 */
	for(ctlr = 0; ctlr < nxp; ctlr++) {
		md = &xpdevs[ctlr];
		sc = &xp_softc[ctlr];
		if (!md->md_alive) {
			sc->sc_alive = 0;
			continue;
		}
		/*
		 * Initialize semaphores, locks and whatever else
		 * per controller.
		 */
		sc->sc_alive = 1;
		sc->sc_drives = 0;
		sc->sc_xpaddr = (struct xpdevice *)
			&md->md_desc->mb_ios->mb_io[md->md_csr];
		init_lock(&sc->sc_lock, xpgate);
		sc->sc_bhead.av_forw = sc->sc_bhead.av_back = NULL;
		sc->sc_bhead.b_back = sc->sc_bhead.b_back = NULL;
		/*
		 * Remember multibus information
		 */
		sc->sc_desc = md->md_desc;
		sc->sc_level = md->md_level;
		sc->sc_ipmap = md->md_basemap;
		sc->sc_dmap = md->md_basemap + 1;
		sc->sc_nmaps = md->md_nmaps - 1;
		/*
		 * Allocate iopb's and chain them together.  Ensure
		 * that they'll all fit within one MBAd map register so
		 * that (1) we'll need to allocate only one, and (2)
		 * that they'll be within one 2^16-byte segment of MB
		 * address.  Initialize MB map for iopb's and semaphore
		 * for allocation.
		 */
		callocrnd(MB_MRSIZE);
		sc->sc_base = calloc((NIOPB + 1) * sizeof (struct xp_iopb));
		sc->sc_free = (struct xp_iopb *) sc->sc_base;
		for(ip = sc->sc_free; ip<&sc->sc_free[NIOPB-1]; ip++) {
			ip->io_forw = ip + 1;
			ip->io_actf = NULL;
			ip->io_actb = NULL;
			ip->io_status = XPS_DONE;
			ip->io_comm = XP_NOP;
		}
		ip->io_forw = NULL;
		ip->io_actf = NULL;
		ip->io_actb = NULL;
		ip->io_status = XPS_DONE;
		ip->io_comm = XP_NOP;
		/*
		 * this last iopb is reserved for reset drive commands
		 */
		ip++;
		ip->io_forw = NULL;
		ip->io_actf = NULL;
		ip->io_actb = NULL;
		ip->io_status = XPS_DONE;
		ip->io_comm = XP_NOP;
		sc->sc_resiopb = ip;
		sc->sc_mbaddr = mbad_physmap(sc->sc_desc, sc->sc_ipmap,
			sc->sc_base, (NIOPB + 1) * sizeof (struct xp_iopb), 1);
		/*
		 * Initialize map allocation arena
		 */
		sc->sc_malloc = calloc((int)sc->sc_nmaps);
		/*
		 * Get buffer for header manipulations
		 */
		callocrnd(MB_MRSIZE);
		sc->sc_header =
			(struct xpheader *) calloc(sizeof (struct xpheader));
	}
	for(unit = 0; unit < xpmaxunit; unit++) {
		/*
		 * Test for existance of drive, and if found read the bad
		 * sector table.  If the drive or controller is
		 * wildcarded, find the first unallocated one that
		 * matches.
		 */
		up = &xpunits[unit];
		for(ctlr = 0; ctlr < xpmaxctlr && up->u_alive == 0; ctlr++) {
			if(ctlr != up->u_ctlr && up->u_ctlr != ANY)
				continue;
			sc = &xp_softc[ctlr];
			if(sc->sc_alive == 0)
				continue;
			for(drive = 0; drive < 4; drive++) {
				if(drive != up->u_drive &&
				   up->u_drive != ANY)
					continue;
				if(sc->sc_drives & (1 << drive))
					continue;
				if(xpslave(sc, drive, up->u_st, &tempiopb)) {
					sc->sc_drives |= (1 << drive);
					up->u_alive = 1;
					up->u_ctlr = ctlr;
					up->u_drive = drive;
					up->u_sc = sc;
					init_sema(&up->u_xpsema, 1, 0, xpgate);
					vo = (struct v_open *)
						calloc(sizeof(struct v_open));
					up->u_xppart = &vo->v_v;
					up->u_opens = &vo->v_vo.v_opens[0];
					up->u_modes = &vo->v_vo.v_modes[0];
					bufinit(&up->u_ctlbuf, xpgate);
					if (dk_nxdrive < dk_ndrives) {
						up->u_dk = &dk[dk_nxdrive++];
						bcopy("xpX", up->u_dk->dk_name, 4);
						up->u_dk->dk_name[2] = unit + '0';
						up->u_dk->dk_bps = 66*46;
						up->u_lastcyl = 0;
						up->u_active = 0;
					} else {
						up->u_dk = (struct dk *)0;
					}
					if(xpgetbad(up, &tempiopb) == 0) {
						up->u_alive = 0;
						break;
					}
					CPRINTF("xp%d at %s%d drive %d: %s\n",
						unit, xy_driver.mbd_name,
						up->u_ctlr, up->u_drive,
						up->u_st->st_name);
					break;
				}
			}
		}
	}
}

/*
 * Set drive parameters and verify that drive exists.
 */

xpslave(sc, drive, st, ip)
register struct xp_softc *sc;
register struct xpst *st;
register struct xp_iopb *ip;
{
	register struct xpdevice *xpaddr;
	register i;
	long mbaddr;
	int csr;
	int timer;

	xpaddr = sc->sc_xpaddr;
	ip->io_comm = XP_DSIZE | XP_RELO | XP_AUD;
	ip->io_imode = XPM_ECC2;
	ip->io_throt = xpthrottle;
	ip->io_drive = (st->st_type << 6) | drive;
	ip->io_diskaddr.io_head = st->st_ntrack - 1;
	ip->io_diskaddr.io_sect = st->st_nsect  - 1;
	ip->io_diskaddr.io_cyl = st->st_ncyl   - 1;
	ip->io_hdoff = 0;
	i = xpaddr->xpreset;
#ifdef lint
	printf("reset status 0x%x\n", i);
	/*
	 *+ Lint Only.
	 */
#endif lint
	DELAY(400);
	mbaddr = mbad_physmap(sc->sc_desc, sc->sc_dmap, (caddr_t) ip,
						sizeof (struct xp_iopb), 2);
	SETIOPBADDR(xpaddr, mbaddr);
	TRACE('S');
	timer = XP_TIMER;
	xpaddr->xpcsr = XP_GO;
	while(xpaddr->xpcsr & XP_BUSY){
		if(--timer == 0) {
			printf("xp%d timeout: cmd 0x%x\n", drive,
						ip->io_comm & XP_COMMASK);
			/*
			 *+ The Xylogic 450 board did not respond after
                         *+ a driver wait.
			 */
			i = xpaddr->xpreset;
			return(0);
		}
	}
	csr = xpaddr->xpcsr;
	if((csr & (XP_ERR|XP_DERR)) == 0 && ip->io_compcode == 0 &&
	   (csr & XP_DRDY))
		return(1);
	xpaddr->xpcsr = XP_ERR;
	return(0);
}

/*
 * Get defects list as define by DEC Standard 144 ala Sequent
 */

xpgetbad(up, ip)
struct xp_unit *up;
register struct xp_iopb *ip;
{
	struct xp_softc *sc;
	struct xpdevice *xpaddr;
	struct dkbad *dkbad;
	int blks;
	int timer;
	long mbaddr, daddr;
	register int i, block, copy;
	register union bt_bad *bt;

	sc = up->u_sc;
	xpaddr = sc->sc_xpaddr;
	up->u_bad = (struct dkbad *)calloc(sizeof(struct dkbad) * DK_NBADMAX);
	dkbad = up->u_bad;
	bt = dkbad->bt_bad;
	for(i=0; i<DK_MAXBAD; i++) {
		bt[i].bt_cyl = DK_END;
		bt[i].bt_trksec = DK_END;
	}
	block = 0;
	mbaddr = mbad_physmap(sc->sc_desc, sc->sc_dmap, (caddr_t) ip,
						sizeof (struct xp_iopb), 2);
	daddr = mbad_physmap(sc->sc_desc, sc->sc_dmap+2,
		(caddr_t)dkbad, sizeof(struct dkbad), 2);
	/*
	 * read block 0
	 */
	for(copy=0; copy<DK_NBADCOPY; copy++) {
		SETIOPBADDR(xpaddr, mbaddr);
		TRACE('S');
		ip->io_lbaddr = daddr;
		ip->io_comm = XP_READ | XP_RELO | XP_AUD;
		ip->io_imode = XPM_ECC2;
		ip->io_throt = xpthrottle;
		ip->io_drive = (up->u_st->st_type << 6) | up->u_drive;
		ip->io_diskaddr.io_head = up->u_st->st_ntrack - 1;
		ip->io_diskaddr.io_cyl = up->u_st->st_ncyl   - 1;
		ip->io_diskaddr.io_sect = DK_LOC(block, copy);
		ip->io_scnt = 1;
		ip->io_hdoff = 0;
		timer = XP_TIMER;
		xpaddr->xpcsr = XP_GO;
		DELAY(200);
		while(xpaddr->xpcsr & XP_BUSY){
			if(--timer == 0) {
				printf("xp%d timeout: cmd 0x%x\n",
					up->u_drive, ip->io_comm & XP_COMMASK);
				/*
				 *+ The Xylogic 450 board did not respond after
				 *+ a driver wait.
				 */
				i = xpaddr->xpreset;
				return(0);
			}
		}
		if((xpaddr->xpcsr & (XP_ERR|XP_DERR)) == 0 &&
					ip->io_compcode == 0 &&
					dkbad->bt_lastb < DK_NBADMAX)
			break;
	}
	xpaddr->xpcsr = XP_ERR;
	if(copy == DK_NBADCOPY) {
		printf("WARNING: can not read xp%d's bad block list\n",
								up->u_drive);
		/*
		 *+ The bad block list entry indicated could not be read.
		 *+ The disk may need to be reformatted.
		 */
		dkbad->bt_lastb = 0;
		bt = dkbad->bt_bad;
		for(i=0; i<DK_MAXBAD; i++) {
			bt[i].bt_cyl = DK_END;
			bt[i].bt_trksec = DK_END;
		}
	}
	if((blks = dkbad->bt_lastb) == 0)
		return(1);
	/*
	 * read block n
	 */
	do {
		block++;
		daddr = mbad_physmap(sc->sc_desc, sc->sc_dmap+2,
			(caddr_t)&dkbad[block], sizeof(struct dkbad), 2);
		for(copy=0; copy<DK_NBADCOPY; copy++) {
			SETIOPBADDR(xpaddr, mbaddr);
			TRACE('S');
			ip->io_lbaddr = daddr;
			ip->io_comm = XP_READ | XP_RELO | XP_AUD;
			ip->io_imode = XPM_ECC2;
			ip->io_throt = xpthrottle;
			ip->io_drive = (up->u_st->st_type << 6) | up->u_drive;
			ip->io_diskaddr.io_head = up->u_st->st_ntrack - 1;
			ip->io_diskaddr.io_cyl = up->u_st->st_ncyl   - 1;
			ip->io_diskaddr.io_sect = DK_LOC(block, copy);
			ip->io_scnt = 1;
			ip->io_hdoff = 0;
			timer = XP_TIMER;
			xpaddr->xpcsr = XP_GO;
			DELAY(200);
			while(xpaddr->xpcsr & XP_BUSY){
				if(--timer == 0) {
					printf("xp%d timeout: cmd 0x%x\n",
					 up->u_drive, ip->io_comm & XP_COMMASK);
					/*
					 *+ The Xylogic 450 board did not 
					 *+ respond after a driver wait.
					 */
					i = xpaddr->xpreset;
					return(0);
				}
			}
			if((xpaddr->xpcsr & (XP_ERR|XP_DERR)) == 0 &&
						ip->io_compcode == 0)
				break;
		}
		xpaddr->xpcsr = XP_ERR;
		if(copy == DK_NBADCOPY) {
			printf("WARNING: can not read block %d of \
xp%d's bad block list\n", block, up->u_drive);
			/*
			 *+ The bad block list entry indicated could not
			 *+ be read. The disk may need to be reformatted.
			 */
			bt = (union bt_bad *)&dkbad[block];
			for(i=0; i<DK_NBAD_N; i++) {
				bt[i].bt_cyl = DK_INVAL;
				bt[i].bt_trksec = DK_INVAL;
			}
		}
	} while(--blks);
	return(1);
}

/*
 * Check for standard stuff and errors on open.
 *
 * Read VTOC info from disk.
 */

xpopen(dev, flags)
dev_t	dev;
int	flags;
{
	register int unit, part;
	register struct xp_unit *up;
	register s;
	register struct xp_softc *sc;
	int	 err;

	TRACE('O');
	unit = VUNIT(dev);
	part = VPART(dev);
	up = &xpunits[unit];
	sc = up->u_sc;
	err = 0;

	if(unit >= xpmaxunit || up->u_alive == 0)
		return(ENXIO);

	/* Serialise xpopen() calls */
	p_sema(&up->u_xpsema, PRIBIO);

	if(sc->sc_opened == 0) {
		timeout(xptimer, (caddr_t)up->u_ctlr, 10*hz);
		sc->sc_opened = 1;
	}
	/*
	 * Ensure that a writer to the V_ALL device holds out all other
	 * accessors of the V_ALL device
	 */
	if (V_ALL(dev) && (up->u_allbusy))
		err = EBUSY;

	if (!up->u_xpvtoc_read && !err) {
		/*
		 * If we have not read the VTOC off this disk, do it now.
		 * readdisklabel() will give us compatibility info if the disk
		 * does not have a VTOC.
		 */
		
		err = vtoc_opencheck(dev, flags, xpcstrat,
				up->u_xppart, up->u_st->st_size, (daddr_t)0,
				-1, up->u_modes[part], "xp");
	}

	if (!err) {
		/*
		 * Succesful open
		 */
		up->u_nopen++;
		if (V_ALL(dev)) {
			/*
			 * Successful open on whole-disk;
			 * lock others out.
			 */
			if (flags & FWRITE){
				up->u_allbusy = 1;
			}
		} else {
			up->u_opens[part] += 1;
			up->u_modes[part] |= flags;
			up->u_xpvtoc_read = 1;
		}
	}
#ifdef XPDEBUG
	if(xpdebug > 2)
		printf("xpopen: dev 0x%x unit %d len %d type %d maxmin 0x%x\n",
				dev, unit,
				up->u_xppart->v_part[VPART(dev)].p_size,
				up->u_st->st_type, xpmaxunit);
#endif XPDEBUG
	v_sema(&up->u_xpsema);
	return(err);
}

/*ARGSUSED*/
xpclose(dev, flag)
	dev_t dev;
	int flag;
{
	spl_t s;
	register struct xp_unit *up = &xpunits[VUNIT(dev)];
	int	part;

	part = VPART(dev);
	s = p_lock(&up->u_sc->sc_lock, SPL5);
	if (!V_ALL(dev)) {
		up->u_opens[part] -= 1;
		up->u_modes[part] &= ~(flag&FUSEM);
	}

	if (--up->u_nopen == 0)
		up->u_xpvtoc_read = 0;
	/*
	 * If we're holding the whole-disk partition, flag it as free now
	 */
	if (V_ALL(dev) && up->u_allbusy)
		up->u_allbusy = 0;
	v_lock(&up->u_sc->sc_lock, s);	
}

xptimer(ctlr)
{
	register s;
	register struct xp_softc *sc;
	
	sc = &xp_softc[ctlr];
	if((s = cp_lock(&sc->sc_lock, SPL5)) != CPLOCKFAIL) {
		if(sc->sc_need) {
#ifdef XPDEBUG
			register struct xp_iopb *ip;
			register struct xpdevice *xpaddr = sc->sc_xpaddr;

#endif XPDEBUG
			printf("%s%d: lost interrupt\n", xy_driver.mbd_name,
									ctlr);
			/*
			 *+ No interrupt has occured since the last command
			 *+ was issued. The disk may not be configered
			 *+ correctly or they may be a cabling problem.
			 */
			TRACE('L');
#ifdef XPDEBUG
			printf("csr 0x%x, mbaddr 0x%x\n", xpaddr->xpcsr,
				GETIOPBADDR(xpaddr));
			printf("Active queue:\n");
			for(ip = sc->sc_active; ip != NULL; ip = ip->io_forw)
			    printf("iopb at 0x%x: status 0x%x, compcode 0x%x\n",
					ip, ip->io_status, ip->io_compcode);
			printf("Done queue:\n");
			for(ip = sc->sc_done; ip != NULL; ip = ip->io_forw)
			    printf("iopb at 0x%x: status 0x%x, compcode 0x%x\n",
					ip, ip->io_status, ip->io_compcode);
			/*
			 * printf("New queue:\n");
			 * for(ip = sc->sc_new; ip != NULL; ip = ip->io_forw)
			 *  printf("iopb at 0x%x: status 0x%x, compcode 0x%x\n",
		         *	ip, ip->io_status, ip->io_compcode);
			 */
			printf("xptrace: %s\n", xptrace);
#endif XPDEBUG
			xpintr_guts(ctlr);
		} else if(sc->sc_active != NULL)
			sc->sc_need = 1;
		v_lock(&sc->sc_lock, s);
	}
	timeout(xptimer, (caddr_t)ctlr, 10*hz);
}

/*
 * Do disk i/o but don't check for boundary conditions.
 */
xpcstrat(bp)
register struct buf *bp;
{
	register x;
	register struct xp_unit *up;
	register struct xp_softc *sc;
	struct xpst *st;
	struct xpdiskaddr diskaddr;
	int sec;

#ifdef XPDEBUG
	if(xpdebug >1) {
		printf("xpstrat(%s) bp=%x, dev=%x, cnt=%d, blk=%x, vaddr=%x\n",
			(bp->b_flags & B_READ) ? "READ" : "WRITE",
			bp, bp->b_dev, bp->b_bcount, bp->b_blkno,
			bp->b_un.b_addr);
	} else if(xpdebug)
		printf("%s", (bp->b_flags & B_READ) ? "r" : "w");
#endif XPDEBUG
	TRACE((bp->b_flags & B_READ) ? 'r' : 'w');
	up = &xpunits[VUNIT(bp->b_dev)];
	sc = up->u_sc;
	st = up->u_st;
	if (bp->b_bcount <= 0 ||
	    ((bp->b_bcount & 0x1FF) != 0) || (bp->b_blkno < 0) ||
	    (bp->b_blkno >= st->st_nspc * st->st_ncyl)) { 
		bp->b_flags |= B_ERROR;
		bp->b_error = EINVAL;
		biodone(bp);
		return;
	}

	diskaddr.io_cyl = bp->b_blkno/st->st_nspc;
	sec = bp->b_blkno % st->st_nspc;
	diskaddr.io_head = sec / st->st_nsect;
	sec %= st->st_nsect;
	diskaddr.io_sect = sec;
	bp->b_diskaddr = *(long *)&diskaddr;

	x = p_lock(&sc->sc_lock, SPL5);

	disksort(&sc->sc_bhead, bp);
	sc->sc_newiopb = 0;
	xpstart(sc, 1);
	if(sc->sc_newiopb) {
		sc->sc_newiopb = 0;
		xpgo(sc);
	}
	v_lock(&sc->sc_lock, x);
}

xpstrat(bp)
register struct	buf *bp;
{
	register x;
	register struct xp_unit *up;
	register struct xp_softc *sc;
	struct xpst *st;
	int size;
	struct xpdiskaddr diskaddr;
	int sec;

#ifdef XPDEBUG
	if(xpdebug >1) {
		printf("xpstrat(%s) bp=%x, dev=%x, cnt=%d, blk=%x, vaddr=%x\n",
			(bp->b_flags & B_READ) ? "READ" : "WRITE",
			bp, bp->b_dev, bp->b_bcount, bp->b_blkno,
			bp->b_un.b_addr);
	} else if(xpdebug)
		printf("%s", (bp->b_flags & B_READ) ? "r" : "w");
#endif XPDEBUG
	TRACE((bp->b_flags & B_READ) ? 'r' : 'w');
	up = &xpunits[VUNIT(bp->b_dev)];
	sc = up->u_sc;
	st = up->u_st;
	size = bp->b_bcount >> DEV_BSHIFT;
	if (bp->b_bcount <= 0 ||
	    ((bp->b_bcount & 0x1FF) != 0) || (bp->b_blkno < 0) ||
	    ((bp->b_blkno + size) >
			up->u_xppart->v_part[VPART(bp->b_dev)].p_size)) {
		bp->b_flags |= B_ERROR;
		bp->b_error = EINVAL;
		u.u_error = EINVAL;
		biodone(bp);
		return;
	}
	xpsrequests[VUNIT(bp->b_dev)]++;		/* pay for it */

	sec = bp->b_blkno + up->u_xppart->v_part[VPART(bp->b_dev)].p_start;
	diskaddr.io_cyl = sec /st->st_nspc;
	sec %= st->st_nspc;
	diskaddr.io_head = sec / st->st_nsect;
	diskaddr.io_sect = sec % st->st_nsect;
	bp->b_diskaddr = *(long *)&diskaddr;

	x = p_lock(&sc->sc_lock, SPL5);
	disksort(&sc->sc_bhead, bp);
	sc->sc_newiopb = 0;
	xpstart(sc, 1);
	if(sc->sc_newiopb) {
		sc->sc_newiopb = 0;
		xpgo(sc);
	}
	v_lock(&sc->sc_lock, x);
}

/*
 * xpminphys - correct for too large a request.
 *
 * We correct to nmaps-1 in case non-page-aligned transfer (to be sure
 * nmaps map-registers will map the request).
 */

xpminphys(bp)
register struct buf *bp;
{
	int	nmaps;

	nmaps = xpunits[VUNIT(bp->b_dev)].u_sc->sc_nmaps;
	if ((bp->b_bcount/MB_MRSIZE) > nmaps-1)
		bp->b_bcount = (nmaps-1) * MB_MRSIZE;
}

xpwrite(dev, uio)
dev_t	dev;
struct uio *uio;
{
	int err, diff;
	off_t lim;

	if (V_ALL(dev))
		return(physio(xpcstrat, (struct buf *)0, dev, B_WRITE, 
		               xpminphys, uio));

	lim = xpunits[VUNIT(dev)].u_xppart->v_part[VPART(dev)].p_size;
	lim <<= DEV_BSHIFT;
	err = physck(lim, uio, B_WRITE, &diff);
	if (err != 0) {
		if (err == -1)	/* not an error, but request of 0 bytes */
			err = 0;
		return(err);
	}
	err = physio(xpstrat, (struct buf *)0, dev, B_WRITE, xpminphys, uio);
	uio->uio_resid += diff;
	return(err);
}

xpread(dev, uio)
dev_t	dev;
struct uio *uio;
{
	int err, diff;
	off_t lim;

	if (V_ALL(dev))
		return(physio(xpcstrat, (struct buf *)0, dev, B_READ, 
		               xpminphys, uio));

	lim = xpunits[VUNIT(dev)].u_xppart->v_part[VPART(dev)].p_size;
	lim <<= DEV_BSHIFT;
	err = physck(lim, uio, B_READ, &diff);
	if (err != 0) {
		if (err == -1)	/* not an error, but request of 0 bytes */
			err = 0;
		return(err);
	}
	err = physio(xpstrat, (struct buf *)0, dev, B_READ, xpminphys, uio);
	uio->uio_resid += diff;
	return(err);
}

/*
 * Process completed iopb's, if there are any. After freeing an iopb,
 * signal the upper level code with biodone(). Place any appropriate
 * new requests on the active list.
 */

xpintr(level) {
	register ctlr;
	register spl_t x;
	register struct xp_softc *sc;
#ifdef XPDEBUG
	register struct xpdevice *xpaddr;
#endif XPDEBUG

	ctlr = level - xpintr_base;
	sc = &xp_softc[ctlr];
	if(ctlr < 0 || ctlr >= xpmaxctlr) {
		printf("xpintr: Unknown interrupt source on level %d\n", level);
		/*
		 *+ An interrupt was recieved from an unconfigured
		 *+ controller.
		 */
		return;
	}
	x = p_lock(&sc->sc_lock, SPL5);
#ifdef XPDEBUG
	xpaddr = xp_softc[ctlr].sc_xpaddr;
	if(xpdebug > 1)
		printf("xpintr(%d): ctlr %d, csr 0x%x\n", level, ctlr,
								xpaddr->xpcsr);
	else if(xpdebug) {
		printf("I");
		if(xpaddr->xpcsr & XP_AACK)
			printf("A%x", xpaddr->xpcsr);
	}
	++xppint[l.me];				/* bump count this processor */
	if(xpaddr->xpcsr & XP_AACK)
		TRACE('A');
#endif	XPDEBUG
	xpintr_guts(ctlr);
	TRACE('X');
	v_lock(&sc->sc_lock, x);
}

xpintr_guts(ctlr)
int ctlr;
{
	register struct buf *bp;
	register struct xpdevice *xpaddr;
	register struct xp_unit *up;
	register struct xp_iopb *ip;
	register struct xp_iopb *next;
	struct xp_softc *sc;
	int compcode;
	int retry;
	int needlist;
	struct xpst *st;

	sc = &xp_softc[ctlr];
	sc->sc_need = 0;
	xpaddr = sc->sc_xpaddr;
	/*
	 * Find the iopb's that caused this interrupt, if any.
	 */
	for(ip = sc->sc_active; ip != NULL; ip = next) {
		next = ip->io_actf;
		if(ip->io_flag)
			continue;
		
		if((ip->io_status & XPS_DONE) == 0)
			continue;
		
#ifdef XPDEBUG
		if(xpdebug > 2)
			printf("processing iopb at 0x%x\n", ip);
		else if(xpdebug)
			printf("P");
#endif XPDEBUG
		TRACE('P');
		if((ip->io_comm & XP_COMMASK) == XP_DRESET) {
			xpprocessdrive(ip, sc);
			goto start_ctrl;
		}
		bp = ip->io_bp;
		up = &xpunits[VUNIT(bp->b_dev)];
		st = up->u_st;
#ifdef XPDEBUG
		/*
		 * This message should *NEVER* happen but a consistency
		 * check here is quite good to have (now and then).
		 */
		if(up->u_ctlr != ctlr) {
			printf("%s%d: incorrect mapping: expected %s%d\n",
						xy_driver.mbd_name, ctlr,
						xy_driver.mbd_name, up->u_ctlr);
			panic("xp map");
		}
		if(xpdebug > 2)
			printf("xpintr: dev = %x\n", bp->b_dev);
		if(xpdebug > 2)
			dumpiopb(ip);
#endif XPDEBUG
		if(ip->io_status & XPS_ERROR) {
			TRACE('E');
			compcode = ip->io_compcode;
			if(xpaddr->xpcsr & XP_DERR)
				retry = 1;
			else {
				retry = 0;
				switch(compcode) {

				/*
				 * Auto-seek retry and soft ECC errors
				 * aren't really errors.
				 */
				case XPC_ASR:
				case XPC_RECC:
					xperror(bp, ip, xpaddr->xpcsr, 0);
					goto noerror;

				case XPC_FAULT:
				case XPC_DNR:
				case XPC_HERR:
				case XPC_SEEKERR:
					xperror(bp, ip, xpaddr->xpcsr, 0);
					xpresetdrive(sc, up, st, ip);
					goto start_ctrl;
				/*
				 * Header not found probably means
				 * a revectored sector.
				 */
				case XPC_HNF:
#ifdef BADSCNT
					xpfixscnt(ip, up);
#endif BADSCNT
					if(xprevector(ip, up) == 0){
						ip->io_flag = D_REQUEUE|D_REVEC;
						ip->io_forw = sc->sc_done;
						sc->sc_done = ip;
						continue;
					}
					/* fall through */

				/*
				 * KLUDGE:  Retry everything for now.
				 * Once the driver is working correctly,
				 * all of the "default" errors should
				 * abort the transfer as a hard failure.
				 */
				default:
				case XPC_ISS:
				case XPC_HECC:
					retry = 1;
					break;

				case XPC_SLACK:
					xpc_slack++;
					retry = 1;
					if (ip->io_errcnt == 1)
						xperror(bp, ip, xpaddr->xpcsr, 0);
					break;

				case XPC_DSE:
					xpc_dse++;
					retry = 1;
					if (ip->io_errcnt == 1)
						xperror(bp, ip, xpaddr->xpcsr, 0);
					break;

				/*
				 * THIS IS A KLUDGE!!!  IPND and CONFLCT should
				 * not be retried.  Once the driver works
				 * correctly they should reflect serious errors.
				 * However, there are windows in that the
				 * driver may be unable to avoid, so these
				 * may need to be retried always.
				 */
				case XPC_IPND:
				case XPC_CONFLCT:
#ifdef XPDEBUG
					botch("xp: ipnd/conflict");
#endif XPDEBUG
					retry = 1;
					break;
				}
			}
			/*
			 * Retry the command, if appropriate.
			 */
			if (retry && ++ip->io_errcnt < xpmaxretry) {
				if(ip->io_errcnt == 1)
					if ((ip->io_compcode != XPC_SLACK) &&
					   (ip->io_compcode != XPC_DSE))
					xperror(bp, ip, xpaddr->xpcsr, 0);
				ip->io_flag = D_REQUEUE | D_RESET;
				ip->io_forw = sc->sc_done;
				sc->sc_done = ip;
#ifdef XPDEBUG
				if(xpdebug > 1) {
					printf("retrying - count %d\n",
								ip->io_errcnt);
					if(xpdebug > 2)
						dumpiopb(ip);
				} else if(xpdebug)
					printf("R");
#endif XPDEBUG
				TRACE('R');
				continue;
			}
			xperror(bp, ip, xpaddr->xpcsr, 1);
			bp->b_flags |= B_ERROR;
			bp->b_error = EIO;
		}
noerror:
		if(ip->io_type == I_REVEC && (bp->b_flags & B_ERROR) == 0) {
			if (ip->io_sscnt > 1) {
				ip->io_flag = D_REQUEUE | D_CONTINUE;
				ip->io_forw = sc->sc_done;
				sc->sc_done = ip;
				TRACE('c');
				continue;
			}
		}
		TRACE('D'); TRACE('['); TRACEN(bp, 16); TRACE(']');
		ip->io_flag = D_PROCESSED;
		ip->io_forw = sc->sc_done;
		sc->sc_done = ip;
		if (up->u_dk) {
			up->u_dk->dk_xfer++;
			up->u_dk->dk_blks += ip->io_iscnt;
			if (up->u_lastcyl != ip->io_diskaddr.io_cyl) {
				up->u_lastcyl = ip->io_diskaddr.io_cyl;
				up->u_dk->dk_seek++;
			}
		}
	}
	/*
	 * Process iopb chain manipulations
	 */
	needlist = sc->sc_done != NULL;
	if(needlist &&
	   ((xpaddr->xpcsr & XP_BUSY) == 0 || (xpaddr->xpcsr & XP_AACK))) {
		needlist = 0;
		for(ip = sc->sc_done; ip != NULL; ip = next) {
			next = ip->io_forw;
			if(xpupdateiopb(ip) == FREE_IOPB)
				xpfreeiopb(ip);
		}
		sc->sc_done = NULL;
		xpstart(sc, 0);
	}
start_ctrl:
	xpaddr->xpcsr = XP_IPND | XP_ERR;
	if((sc->sc_active != NULL) && ((xpaddr->xpcsr & XP_BUSY) == 0))
		xpgo(sc);
	else 
		if(sc->sc_active != NULL)
			mbad_reenable(sc->sc_desc, sc->sc_level);
}

/* This function assumes that the controller is not busy since we have
 * an error. It also assumes that the maximum length of a controller's
 * iopb chain is one.
 */

xpresetdrive(sc, up, st, ip)
register struct xp_softc *sc;
register struct xp_unit *up;
register struct xpst *st;
register struct xp_iopb *ip;
{
	register struct xp_iopb *res_ip;

	sc->sc_savechain = ip;
	res_ip = sc->sc_resiopb;
	res_ip->io_comm =  XP_DRESET | XP_RELO | XP_IEN | XP_AUD;
	res_ip->io_imode =  XPM_HDP | XPM_IERR | XPM_IEI;
	res_ip->io_throt = xpthrottle;  /* tunable */
	res_ip->io_drive = (st->st_type << 6) | up->u_drive;
	res_ip->io_niop = 0;
	res_ip->io_status = 0;
	res_ip->io_compcode = 0;
	res_ip->io_errcnt = 0;
	res_ip->io_bp = ip->io_bp;
	sc->sc_active = res_ip;
}

/* This function assumes that the controller is not busy since we have
 * an error. It also assumes that the maximum length of a controller's
 * iopb chain is one.
 */

xpprocessdrive(res_ip, sc)
register struct xp_iopb *res_ip;
register struct xp_softc *sc;
{
	register struct xp_iopb *ip;

	if(res_ip->io_status & XPS_ERROR) {
		if (++res_ip->io_errcnt < xpmaxretry) {
			printf("error: xp%d failed drive reset command ",
				VUNIT(res_ip->io_bp->b_dev));
			/*
			 *+ The xp controller has not responded to a reset
			 *+ command.
			 */
			if(res_ip->io_compcode <= NCOMPCODES) {
				printf("compcode=0x%x: %s\n",
					res_ip->io_compcode,
					xp_compcodes[res_ip->io_compcode]);
				/*
				 *+ The xp controller has not responded to
				 *+ a reset command. 
				 */
			} else {
				printf("compcode=0x%x: unknown error\n",
						res_ip->io_compcode);
				/*
				 *+ The xp controller has not responded to
				 *+ a reset command. 
				 */
			}
			res_ip->io_status = 0;
			res_ip->io_compcode = 0;
			return;
		}
		printf("HARD ERROR xp%d failed drive reset command ",
					VUNIT(res_ip->io_bp->b_dev));
		/*
		 *+ The xp driver has report a hard error.
		 *+ Consult the controller error guide.
		 */
		if(res_ip->io_compcode <= NCOMPCODES) {
			printf("compcode=0x%x: %s\n",
				res_ip->io_compcode,
					xp_compcodes[res_ip->io_compcode]);
			/*
			 *+ The xp driver has report a hard error.
			 *+ Consult the controller error guide.
			 */
		} else {
			printf("compcode=0x%x: unknown error\n",
						res_ip->io_compcode);
			/*
			 *+ The xp driver has report a hard error.
			 *+ Consult the controller error guide.
			 */
		}
		if(sc->sc_savechain != NULL) {
			ip = sc->sc_active = sc->sc_savechain;
			sc->sc_savechain = NULL;
			ip->io_bp->b_flags |= B_ERROR;
			ip->io_bp->b_error = EIO;
			xperror(ip->io_bp, ip, sc->sc_xpaddr->xpcsr, 1);
			xpfreeiopb(ip);
			xpstart(sc, 0);
			return;
		}
	}
	if(sc->sc_savechain != NULL) {
		sc->sc_active = sc->sc_savechain;
		xpresetiopb(sc->sc_active);
		sc->sc_savechain = NULL;
	}
}

xperror(bp, ip, csr, hard)
register struct buf *bp;
register struct xp_iopb *ip;
u_char csr;
{
	register struct xpst *st;
	int sector;
	int drive;

	drive = VUNIT(bp->b_dev);
	st = xpunits[drive].u_st;
	sector = (ip->io_diskaddr.io_cyl * st->st_nspc) +
				(ip->io_diskaddr.io_head * st->st_nsect) + ip->io_diskaddr.io_sect;

	printf("xp%d%c %s: ", drive, "abcdefgh"[VUNIT(bp->b_dev) & 7],
		hard ? "HARD ERROR" : "soft error");
	/*
	 *+ The xp drive indicated it has a hard error.
	 *+ Consult the controller error guide.
	 */

	if((ip->io_status & XPS_ERROR) == 0) 
		CPRINTF("(flakey) ");

	if(csr & XP_DERR)
		CPRINTF("(double error) ");

	CPRINTF("compcode=0x%x: %s\n", ip->io_compcode,
		ip->io_compcode <= NCOMPCODES ? xp_compcodes[ip->io_compcode] : "unknown error");

	CPRINTF("\tfilesystem blkno = %d\n", bp->b_blkno);

	CPRINTF("\t(for addbad) physical sector = %d\n", sector);

	CPRINTF("\t(for xpformat) cylinder = %d; head = %d; sector = %d\n",
			ip->io_diskaddr.io_cyl, ip->io_diskaddr.io_head, ip->io_diskaddr.io_sect);
}

/*
 * Actually start the controller.  If it's already busy,
 * post an attention request, otherwise just start it.
 */

xpgo(sc)
register struct xp_softc *sc;
{
	register struct xpdevice *xpaddr;

	xpaddr = sc->sc_xpaddr;
#ifdef XPDEBUG
	if(xpdebug > 2) {
		printf("xpgo called: xpaddr 0x%x\n", xpaddr);
		printf("iopb address 0x%x\n", GETIOPBADDR(xpaddr));
		printf("csr 0x%x before, ", xpaddr->xpcsr);
	} else if(xpdebug)
		printf("g");
#endif XPDEBUG
	TRACE('g');
	if(xpaddr->xpcsr & XP_IPND) {
#ifdef XPDEBUG
		if(xpdebug > 2)
			printf("\ninterrupt pending\n");
		else if(xpdebug)
			printf("p");
#endif XPDEBUG
		TRACE('p');
		return;
	}
	if(sc->sc_active != NULL) {
		if((xpaddr->xpcsr & (XP_BUSY | XP_IPND)) == 0) {
			ASSERT(sc->sc_active!=NULL, "xpgo: sc_active");
			/*
			 *+ Xpgo was called with no work on its active queue.
			 */
			SETIOPBADDR(sc->sc_xpaddr, IOPBMBADDR(sc->sc_active, sc));
			TRACE('S');
			xpaddr->xpcsr = XP_GO;
		} else if((xpaddr->xpcsr & XP_IPND) == 0) {
			xpaddr->xpcsr = XP_AREQ;
#ifdef XPDEBUG
			if(xpdebug > 2)
				printf("attention request\n");
			else if(xpdebug)
				printf("a");
#endif XPDEBUG
			TRACE('a');
		}
	}
#ifdef XPDEBUG
	if(xpdebug >  2)
		printf("after 0x%x 0x%x\n", xpaddr->xpcsr, GETIOPBADDR(xpaddr));
#endif XPDEBUG
	mbad_reenable(sc->sc_desc, sc->sc_level);
}

/* 
 * xpstart - If the resources are available, add an iopb to the active list.
 *
 * Assumes that the controller is locked when called.
 */

xpstart(sc, flag)
register struct xp_softc *sc;
char flag;
{
	register struct xpdevice *xpaddr;
	register struct xp_iopb *ip;
	int nmaps, basemap, nsecs;

	xpaddr = sc->sc_xpaddr;
#ifdef XPDEBUG
	if (xpdebug > 1)
		printf("xpstart called.\n");
	else if(xpdebug)
		printf("s");
#endif XPDEBUG
	TRACE('s');
	if(sc->sc_active != NULL)
		if((xpaddr->xpcsr & XP_BUSY)
				&& ((xpaddr->xpcsr & XP_AACK) == 0))
	return;
	while(sc->sc_free != NULL && sc->sc_bhead.av_forw != NULL) {
		nsecs = sc->sc_bhead.av_forw->b_bcount >> DEV_BSHIFT;
		nmaps = ((nsecs + MB_MRSIZE/DEV_BSIZE - 1) /
						(MB_MRSIZE/DEV_BSIZE)) + 1;

		if((basemap = xpmapalloc(sc, nmaps)) == -1)
			break;

		/* remove buffer from queue and place on active chain */

		ip = xpmakeiopb(sc, nsecs, nmaps, basemap);
		xpaddiopb(ip);
		if(flag) {
			sc->sc_newiopb++;
			flag = 0;
		}
		sc->sc_bhead.av_forw = sc->sc_bhead.av_forw->av_forw;
	}
}

struct xp_iopb *
xpmakeiopb(sc, nsecs, nmaps, basemap)
register struct xp_softc *sc;
int nmaps, basemap, nsecs;
{
	register struct xp_iopb *ip;
	register struct xp_unit *up;
	register struct xpst *st;
	register struct buf *bp;

	bp = sc->sc_bhead.av_forw;
	up = &xpunits[VUNIT(bp->b_dev)];
	st = up->u_st;

	ip = sc->sc_free;
	sc->sc_free = ip->io_forw;
	TRACE('i'); TRACE('['); TRACEN(bp->b_un.b_addr, 16); TRACE(':');
	TRACEN(nsecs, 10); TRACE(']');

	ip->io_diskaddr = *(struct xpdiskaddr *)&bp->b_diskaddr;
	ip->io_idiskaddr = ip->io_diskaddr;

	if(bp->b_flags & B_IOCTL)
		ip->io_comm = (u_char)bp->b_ioctl | XP_RELO | XP_IEN | XP_AUD;
	else
		ip->io_comm = ((bp->b_flags & B_READ) ? XP_READ : XP_WRITE)
						| XP_RELO | XP_IEN | XP_AUD;
	ip->io_imode = XPM_ECC2 /*| XPM_EEF */ | XPM_ASR | XPM_IERR | XPM_IEI;
	ip->io_throt = xpthrottle;  /* tunable */
	ip->io_drive = (st->st_type << 6) | up->u_drive;
	ip->io_iscnt = ip->io_scnt = nsecs;
	ip->io_ilbaddr = ip->io_lbaddr = mbad_setup(bp, sc->sc_desc,
					sc->sc_dmap + basemap, nmaps);
	ip->io_status = 0;
	ip->io_compcode = 0;
	ip->io_hdoff = 0;
	ip->io_niop = 0;
	ip->io_forw = ip->io_actf = ip->io_actb = NULL;
	ip->io_bp = bp;
	ip->io_errcnt = 0;
	ip->io_type = I_NORMAL;
	ip->io_ctlr = up->u_ctlr;
	ip->io_basemap = basemap;
	ip->io_nmaps = nmaps;
	ip->io_flag = 0;
#ifdef XPDEBUG
	if(xpdebug > 2)
		dumpiopb(ip);
#endif XPDEBUG
	return(ip);
}

xpresetiopb(ip)
register struct xp_iopb *ip;
{
	ip->io_diskaddr = ip->io_idiskaddr;
	ip->io_scnt = ip->io_iscnt;
	ip->io_lbaddr = ip->io_ilbaddr;
	ip->io_status = 0;
	ip->io_compcode = 0;
#ifdef XPDEBUG
	printf("reset to (%d, %d, %d) addr 0x%x, %d sectors\n",
		ip->io_diskaddr.io_cyl, ip->io_diskaddr.io_head, ip->io_diskaddr.io_sect, ip->io_lbaddr, ip->io_scnt);
	if (ip->io_errcnt <= 1) {
		dumpiopb(ip);
		printf("xptrace: %s\n", xptrace);
	}
#endif XPDEBUG
}

#ifdef BADSCNT
xpfixscnt(ip,up)
register struct xp_iopb *ip;
register struct xp_unit	*up;
{
	register struct buf *bp;
	register daddr_t endblk;
	register struct xpst *st = up->u_st;

	bp = ip->io_bp;
	endblk = ip->io_diskaddr.io_cyl * st->st_nspc + ip->io_diskaddr.io_head * st->st_nsect +
								ip->io_diskaddr.io_sect;
	endblk -= up->u_xppart->v_part[VPART(bp->b_dev)].p_start;
	ip->io_scnt = (bp->b_bcount >> DEV_BSHIFT) - (endblk - bp->b_blkno);
	ip->io_lbaddr = ip->io_ilbaddr + (ip->io_iscnt - ip->io_scnt)*512;
#ifdef XPDEBUG
	if (ip->io_errcnt || xpdebug > 2)
		printf("io_scnt reset to %d [%d - (%d - %d)]\n", ip->io_scnt,
			(bp->b_bcount >> DEV_BSHIFT), endblk, bp->b_blkno);
#endif XPDEBUG
}
#endif BADSCNT

#ifdef XPDEBUG
dumpiopb(ip)
register struct xp_iopb *ip;
{
	printf("iopb at 0x%x: cltr %d comm 0x%x imode 0x%x status 0x%x\n",
		ip, ip->io_ctlr, ip->io_comm, ip->io_imode, ip->io_status);
	printf("   compcode 0x%x throt 0x%x drive 0x%x\n",
		ip->io_compcode, ip->io_throt, ip->io_drive);
	printf("   head %d sect %d cyl %d scnt %d\n",
		ip->io_diskaddr.io_head, ip->io_diskaddr.io_sect, ip->io_diskaddr.io_cyl, ip->io_scnt);
	printf("   lbaddr 0x%x hdoff 0x%x niop 0x%x\n",
		ip->io_lbaddr, ip->io_hdoff, ip->io_niop);
	printf("   forw  0x%x bp 0x%x errcnt %d type %d\n",
		ip->io_forw, ip->io_bp, ip->io_errcnt, ip->io_type);
}
#endif

/*
 * Allocate the necessary maps if they are available.
 * Both xpmapalloc and xpmapfree must be called with
 * the lock held.
 */

xpmapalloc(sc, nmaps)
register struct xp_softc *sc;
register int nmaps;
{
	register char *cp;
	register i;
	register char *end;
	int basemap = -1;

	end = sc->sc_malloc + sc->sc_nmaps;
	for(cp = sc->sc_malloc; cp < end; cp++) {
		for(i = 0; i < nmaps; i++) {
			if(cp + i > end)
				return(-1);
			if(cp[i] != 0)
				break;
		}
		if(i != nmaps)
			continue;
		basemap = cp - sc->sc_malloc;
		break;
	}
	if(basemap >= 0) {
		cp = sc->sc_malloc + basemap;
		for(i = 0; i < nmaps; i++)
			cp[i] = 1;
		TRACE('A'); TRACEN(basemap, 10); TRACE(':'); TRACEN(nmaps, 10);
		return(basemap);
	}
	return(-1);
}

xpmapfree(sc, basemap, nmaps)
register struct xp_softc *sc;
u_char basemap, nmaps;
{
	register i;
	register char *cp;

	cp = sc->sc_malloc + basemap;
	for(i = 0; i < nmaps; i++)
		cp[i] = 0;
	TRACE('f'); TRACEN(basemap, 10); TRACE(':'); TRACEN(nmaps, 10);
}

/*
 * Free an iopb that's on the controller chain.  This is part of the
 * interrupt routine, so it's interlocked by the device, and it's
 * also got the lock.
 */

xpfreeiopb(ip)
register struct xp_iopb *ip;
{
	register struct xp_iopb *prev;
	register struct xp_iopb *next;
	register struct xp_softc *sc;
	register struct xp_unit *up;

#ifdef XPDEBUG
	if(xpdebug > 2)
		printf("freeing iopb at 0x%x\n", ip);
	else if(xpdebug)
		printf("F");
#endif XPDEBUG
	TRACE('F');
	prev = ip->io_actb;
	next = ip->io_actf;
	sc = &xp_softc[ip->io_ctlr];
	ASSERT(sc->sc_active!=NULL, "xpfreeiopb: sc_active");
	/*
	 *+ Xpfreeiopb was called with no work on its active queue.
	 */
	if(next != NULL)
		next->io_actb = prev;
	if(prev != NULL) {
#ifdef XPDEBUG
		if(xpdebug > 2)
			printf("freeing mid-queue\n");
		else if(xpdebug)
			printf("2");
#endif XPDEBUG
		TRACE('2');
		ASSERT(sc->sc_active!=ip, "xpfreeiopb: ip");
		/*
		 *+ xpfreeiopb detected a bad linked list of active buffer
		 *+ pointers.
		 */
		ASSERT(sc->sc_active->io_actb==NULL, "xpfreeiopb: io_actb");
		/*
		 *+ xpfreeiopb detected a bad linked list of active buffer
		 *+ pointers.
		 */
		prev->io_niop = ip->io_niop;
		prev->io_comm &= ~XP_CHEN;
		ASSERT(next==NULL||(ip->io_comm&XP_CHEN), "xpfreeiopb: next");
		/*
		 *+ xpfreeiopb detected a bad linked list of active buffer
		 *+ pointers.
		 */
		prev->io_comm |= ip->io_comm & XP_CHEN;
		prev->io_actf = next;
	} else {
#ifdef XPDEBUG
		if(xpdebug > 2)
			printf("freeing first\n");
		else if(xpdebug)
			printf("1");
#endif XPDEBUG
		TRACE('1');
		ASSERT(sc->sc_active==ip, "xpfreeiopb: sc_active 2");
		/*
		 *+ xpfreeiopb detected a bad linked list of active buffer
		 *+ pointers.
		 */
		sc->sc_active = next;
		if(next != NULL)
			next->io_actb = NULL;
	}
	ip->io_actf = NULL;
	ip->io_actb = NULL;
	up = &xpunits[VUNIT(ip->io_bp->b_dev)];
	if (up->u_dk && --up->u_active == 0)  {
		struct timeval elapsed;

		elapsed = time;
		timevalsub(&elapsed, &up->u_starttime);
		timevaladd(&up->u_dk->dk_time, &elapsed);
	}
	ip->io_bp->b_resid = 0;
	biodone(ip->io_bp);
	ip->io_comm = XP_NOP | XP_IEN;
	ip->io_status = XPS_DONE;
	ip->io_forw = sc->sc_free;
	sc->sc_free = ip;
	xpmapfree(sc, ip->io_basemap, ip->io_nmaps);

	/*
	 * Decrement I/O activity
	 */
	if (fp_lights) {
		(void)splhi();
		FP_IO_INACTIVE;
		(void)spl5();
	}
}

/*
 * Add an iopb to the controller chain.  Locking is the
 * same as for xpfreeiopb.
 */

xpaddiopb(ip)
	register struct xp_iopb *ip;
{
	register struct xp_iopb *head;
	struct xp_unit *up;
	struct xp_softc *sc;
	long iopbaddr;

	/*
	 * Increment I/O activity
	 */
	if (fp_lights) {
		spl_t	s_ipl;

		s_ipl = splhi();
		FP_IO_ACTIVE;
		splx(s_ipl);
	}

	sc = &xp_softc[ip->io_ctlr];
	iopbaddr = IOPBMBADDR(ip, sc);
#ifdef XPDEBUG
	if(xpdebug > 2)
		printf("adding iopb 0x%x cyl %d, MB addr 0x%x\n", ip, ip->io_diskaddr.io_cyl,
								iopbaddr);
	else if(xpdebug)
		printf("+");
#endif XPDEBUG
	TRACE('+');
	ip->io_flag = 0;
	up = &xpunits[VUNIT(ip->io_bp->b_dev)];
	if (up->u_dk && ++up->u_active == 1)  {
		up->u_starttime = time;
	}
	head = sc->sc_active;
	if(head == NULL) {
		/*
		 * the active list is empty
		 */
#ifdef XPDEBUG
		if(xpdebug > 2)
			printf("active list empty\n");
#endif XPDEBUG
		ip->io_forw = NULL;
		ip->io_actf = NULL;
		ip->io_actb = NULL;
		sc->sc_active = ip;
		TRACE('e');
		return;
	} else {
		while(head != NULL) {
			if(head->io_actf != NULL)
				head = head->io_actf;
			else
				break;
		}
#ifdef XPDEBUG
		if(xpdebug > 2)
			printf("adding after cyl %d\n", head->io_diskaddr.io_cyl);
#endif XPDEBUG
		head->io_niop = iopbaddr;
		ip->io_comm &= ~XP_CHEN;
		head->io_comm |= XP_CHEN;
		ip->io_actf = head->io_actf;
		head->io_actf = ip;
		ip->io_actb = head;
		TRACE('m');
	}
}

/*
 * Update an iopb and determine whether or not to free it.
 */

xpupdateiopb(ip)
register struct xp_iopb *ip;
{
	if(ip->io_flag & D_REQUEUE) {
		if(ip->io_flag & D_RESET)
			xpresetiopb(ip);
		if(ip->io_flag & D_REVEC)
			xpdorevec(ip);
		if(ip->io_flag & D_CONTINUE)
			if(xpcontinue(ip) == 0)
				return(FREE_IOPB);
		ip->io_flag = 0;
		return(0);
	}
	return(FREE_IOPB);
}

/*
 * Ioctl procedure implements various disk commands.
 *
 * Calls to the ioctl routine are serialized via a unit's ioctl buffer
 * which is sufficient to provide necessary mutual exclusion. This buffer
 * will then be passed to xpstrat() to do the work, just like any other
 * buffer.
 */

#include "../h/ioctl.h"
#include "../mbad/xpioctl.h"

/*ARGSUSED*/
xpioctl(dev, cmd, data, flag)
dev_t dev;
caddr_t data;
{
	register struct xp_ioctl *xpop;
	register struct buf *bp;
	register struct xpst *st;
	register struct xp_unit *up;
	register struct xp_softc *sc;
	int error;
	struct vtoc *vtp;

	if(VUNIT(dev) >= xpmaxunit)
		return(ENXIO);
	up = &xpunits[VUNIT(dev)];
	sc = up->u_sc;
	st = up->u_st;
	xpop = (struct xp_ioctl *) data;
	bp = &up->u_ctlbuf;
	switch(cmd) {

	case XPIOCZAPHEADER:
		/*
		 * Change the header of the specified cylinder/head/sector
		 * to be invalid, so that the driver will attempt to
		 * revector this sector if it is encountered.
		 *
		 * Must be on the "C" partition, and only by the super-user.
		 */
		if(!suser())
			return(EPERM);
		if((VUNIT(dev) & 07) != 02)
			return(EINVAL);
		p_sema(&bp->b_alloc, PZERO-1);
		bp->b_dev = dev;
		bp->b_bcount = 1 << 9;
		bp->b_bufsize = sizeof (struct xpheader);
		bp->b_iotype = B_FILIO;
		bp->b_un.b_addr = (caddr_t) sc->sc_header;
		/*
		 * take skew ("adaptive format") into account
		 */
		bp->b_blkno = xpop->x_cyl * st->st_nspc   +
			      xpop->x_head * st->st_nsect +
			      ((xpop->x_sector + xpop->x_head) % st->st_nsect);
		bp->b_flags = B_READ | B_IOCTL;
		bp->b_ioctl = (struct proc *)XP_XREAD;
		BIODONE(bp) = 0;
		xpstrat(bp);
		biowait(bp);
		if(error = geterror(bp)) {
			v_sema(&bp->b_alloc);
			return(error);
		}
		sc->sc_header->header = BADHEADER;
		bp->b_dev = dev;
		bp->b_bcount = 1 << 9;
		bp->b_bufsize = sizeof (struct xpheader);
		bp->b_iotype = B_FILIO;
		bp->b_un.b_addr = (caddr_t) sc->sc_header;
		bp->b_blkno = xpop->x_cyl * st->st_nspc   +
			      xpop->x_head * st->st_nsect +
			      xpop->x_sector;
		bp->b_flags = B_WRITE | B_IOCTL;
		bp->b_ioctl = (struct proc *)XP_XWRITE;
		BIODONE(bp) = 0;
		xpstrat(bp);
		biowait(bp);
		error = geterror(bp);
		v_sema(&bp->b_alloc);
		return(error);

	case V_READ:		/* Read VTOC info to user */
		vtp = *(struct vtoc **)data;
#ifdef DEBUG
		printf("copying out to user:0x%x\n", vtp);
#endif
		if (!readdisklabel(dev, xpcstrat, up->u_xppart, 
				 (struct cmptsize *)0, (daddr_t)0))
			return(EINVAL);
		return( copyout((caddr_t)up->u_xppart, (caddr_t)vtp,
						sizeof(struct vtoc)));

	case V_WRITE:		/* Write user VTOC to disk */
		vtp = (struct vtoc *)wmemall(sizeof(struct vtoc), 1);
		if (copyin(*((caddr_t *)data), (caddr_t)vtp, 
							  sizeof(struct vtoc))) {
			wmemfree((caddr_t)vtp, sizeof(struct vtoc));
			return(EFAULT);
		}
		if (!readdisklabel(dev, xpcstrat, up->u_xppart,
				up->u_st->st_size, (daddr_t)0)) {
			wmemfree((caddr_t)vtp, sizeof(struct vtoc));
			return(EINVAL);
		}
		if (error = setdisklabel(up->u_xppart, vtp, up->u_opens)) {
			wmemfree((caddr_t)vtp, sizeof(struct vtoc));
			return ((error > 0) ? error : EINVAL);
		}
		wmemfree((caddr_t)vtp, sizeof(struct vtoc));
		return(writedisklabel(dev, xpcstrat, up->u_xppart, (daddr_t)0));

	case V_PART:            /* Read partition info to user */
		vtp = *(struct vtoc **)data;
		if (!readdisklabel(dev, xpcstrat, up->u_xppart,
		         up->u_st->st_size, (daddr_t)0))
			return(EINVAL);
		return (copyout((caddr_t)up->u_xppart, (caddr_t)vtp,
			        sizeof(struct vtoc)));

	default:
		return(ENXIO);
	}
}

/*
 * xpsize()
 *	Used for swap-space partition calculation.
 */
 
xpsize(dev)
register dev_t dev;
{
	register struct xp_unit *up;

	up = &xpunits[VUNIT(dev)];
	if (VUNIT(dev) >= xpmaxunit
	   || up->u_xppart->v_part[VPART(dev)].p_size == 0
	   || up->u_alive == 0)
		return(-1);
	return(up->u_xppart->v_part[VPART(dev)].p_size);
}

xprevector(ip, up)
register struct xp_iopb *ip;
struct xp_unit *up;
{
#ifdef XPDEBUG
	if(xpdebug > 1)
		printf("looking to revector (%d, %d, %d)\n", ip->io_diskaddr.io_cyl,
			ip->io_diskaddr.io_head, ip->io_diskaddr.io_sect);
	else if(xpdebug)
		printf("v");
#endif XPDEBUG
	TRACE('v');
	if((ip->io_bbn = isbad(up->u_bad,
		(int)ip->io_diskaddr.io_cyl, (int)ip->io_diskaddr.io_head, (int)ip->io_diskaddr.io_sect)) < 0)
		return(1);
#ifdef	XPDEBUG
	if(xpdebug)
		printf("revectoring (%d, %d, %d) to %d\n", ip->io_diskaddr.io_cyl,
			ip->io_diskaddr.io_head, ip->io_diskaddr.io_sect, ip->io_bbn);
#endif	XPDEBUG
	return(0);
}

xpdorevec(ip)
register struct xp_iopb *ip;
{
	register struct buf *bp;
	register struct xpst *st;
	register int temp;
	register int sector;

	bp = ip->io_bp;
	st = xpunits[VUNIT(bp->b_dev)].u_st;
	if(ip->io_type == I_NORMAL) {
		ip->io_sdiskaddr = ip->io_diskaddr;
		ip->io_sscnt = ip->io_scnt;
	}
	sector = st->st_ncyl * st->st_ntrack * st->st_nsect - st->st_nsect
						- 1 - ip->io_bbn;
	ip->io_diskaddr.io_cyl = sector / (st->st_ntrack * st->st_nsect);
	temp = sector % (st->st_ntrack * st->st_nsect);
	ip->io_diskaddr.io_head = temp / st->st_nsect;
	ip->io_diskaddr.io_sect = temp % st->st_nsect;
	ip->io_idiskaddr = ip->io_diskaddr;
	ip->io_iscnt = ip->io_scnt = 1;
	if(ip->io_type == I_NORMAL)
		ip->io_ilbaddr = ip->io_lbaddr;
	else
		ip->io_lbaddr = ip->io_ilbaddr;
	ip->io_status = 0;
	ip->io_compcode = 0;
	ip->io_type = I_REVEC;
#ifdef	XPDEBUG
	if(xpdebug)
		printf("xprevect (%d, %d, %d)\n", ip->io_diskaddr.io_cyl,
			ip->io_diskaddr.io_head, ip->io_diskaddr.io_sect);
#endif	XPDEBUG
}

xpcontinue(ip)
register struct xp_iopb *ip;
{
	register struct buf *bp;
	register struct xpst *st;

	bp = ip->io_bp;
	st = xpunits[VUNIT(bp->b_dev)].u_st;
	ip->io_diskaddr = ip->io_sdiskaddr;
	ip->io_scnt = ip->io_sscnt;
	if(--ip->io_scnt != 0) {
		if(++ip->io_diskaddr.io_sect >= st->st_nsect) {
			ip->io_diskaddr.io_sect = 0;
			if(++ip->io_diskaddr.io_head >= st->st_ntrack) {
				ip->io_diskaddr.io_head = 0;
				ip->io_diskaddr.io_cyl++;
			}
		}
		ip->io_idiskaddr = ip->io_diskaddr;
		ip->io_iscnt = ip->io_scnt;
		ip->io_ilbaddr = ip->io_lbaddr;
		ip->io_status = 0;
		ip->io_compcode = 0;
		ip->io_type = I_NORMAL;
#ifdef XPDEBUG
		if(xpdebug)
			printf("xpcontinue (%d, %d, %d) for %d\n", ip->io_diskaddr.io_cyl,
				ip->io_diskaddr.io_head, ip->io_diskaddr.io_sect, ip->io_scnt);
		if(xpdebug > 2) {
			printf("continue after revector:\n");
			dumpiopb(ip);
		}
#endif XPDEBUG
		TRACE('C');
		return(1);
	}
	TRACE('N');
	return(0);
}

#ifdef XPDEBUG
botch(s)
char *s;
{
	register char *cp;

	printf("xp trace: %s\n", xptrace);
	printf("... : %s\n", xptracep+1);
	panic(s);
}
#endif XPDEBUG
