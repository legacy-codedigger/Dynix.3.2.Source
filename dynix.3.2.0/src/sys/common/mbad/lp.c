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
static	char	rcsid[] = "$Header: lp.c 1.24 1991/05/22 18:37:35 $";
#endif

/*
 *	driver for the mlp2000 (Systech line printer)
 *
 * TODO: 
 *	Reimplement double-buffering the DMA buffer.  Use separate buffer
 * pointers and counters.
 */

/* $Log: lp.c,v $
 *
 */

#ifdef DEBUG
int	lpdebug = 0;
#define LPDEBUG(x)	if (lpdebug) printf(x)
#else
#define LPDEBUG(x)
#endif DEBUG
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/errno.h"
#include "../balance/clock.h"
#include "../machine/intctl.h"
#include "../mbad/mbad.h"
#include "../machine/ioconf.h"
#include "../machine/gate.h"
#include "../h/uio.h"
#include "../h/buf.h"
#include "../mbad/lp.h"


int	lpprobe(), lpboot(), lpintr(), lp_timer();
struct	mbad_driver lp_driver = {
	"lp",				/* name */
	MBD_TYPICAL,			/* configuration flags */
	lpprobe,			/* probe procedure */
	lpboot,				/* boot procedure */
	lpintr,				/* intr procedure */
};


/*
 * On interrupts, the board number == interrupt level - stintr_base.
 */
extern	struct		lp_printer lpconfig[];
struct	lp_softc	**lp_softc;		/* software state */
extern	int		lpprinters;		/* # of printer configs */
int			lpintr_base;		/* base interrupt vector */
int			nlp;			/* maximum number of boards */

/*
 * see of there is a controller present
 */
lpprobe(mp)
	register struct mbad_probe *mp;
{
	register struct lpdevice *base;
	u_char i;

	base = (struct lpdevice *) &mp->mp_desc->mb_ios->mb_io[mp->mp_csr];
	base->lpd_cmdreg = (u_char) 0;		/* disable interrupts */
	i = base->lpd_intr;
#ifdef lint
	printf("interrupt register:0x%x\n", i);
	/*
	 *+ lint only.
	 */
#endif
	/*
	 * if you get a memory error, there is no board.
	 */
	return((base->lpd_intr&LPI_MEMERR)==0);
}

/*
 *	Locks are allocated *one per board*.
 *	One half of the mapping registers are allocated
 *	to each of the posible ports.
 */
lpboot(n, md)
	int	n;				/* # of boards configured */
	register struct mbad_dev *md;		/* pointer to first */
{
	register struct lp_softc *lp;
	int	board;
	int	unit;
	int	printer = 0;
	lock_t	*locks;
	int	prindex;
	extern	gate_t	lpgate;
#ifdef lint
	lpgate = (gate_t)0;
#endif
	
	nlp = n;
	lpintr_base = md->md_vector;
	/*
	 * allocate space for softc structure and one lock *per board*,
	 * ie, one lock for every two softc's.
	 */
	if (nlp) {
		lp_softc = (struct lp_softc **) 
			calloc( (int)(sizeof(struct lp_softc *) * nlp * 2));
		locks = (lock_t *) calloc((int)(sizeof(lock_t) * nlp));
	}

	/*
	 * for each controller ...
	 *	for each channel ...
	 *		allocate and initialize softc struct
	 */
	for (board = 0; board < nlp; md++, board++) {
		for (unit = 0; unit < 2; unit++) {
			lp = lp_softc[board * 2 + unit] =
				(struct lp_softc *) calloc((int)sizeof(struct lp_softc));
			lp->lp_unit = unit;
			lp->lp_base =
				(struct lpdevice *) &md->md_desc->mb_ios->mb_io[md->md_csr];
			if (!(lp->lp_alive = md->md_alive))
				continue;
			lp->lp_mblevel = md->md_level;
			lp->lp_mbdesc = md->md_desc;
			/*
			 * Half the maps to each unit, maximum 8k
			 * per line printer.  If odd number of maps,
			 * unit 0 gets one extra (arbitrary).
			 */
			lp->lp_nmaps = md->md_nmaps >> 1;
			if (unit == 0 && (md->md_nmaps & 01))
				lp->lp_nmaps++;
			lp->lp_basemap = md->md_basemap + (unit * lp->lp_nmaps);
			lp->lp_bufsz = lp->lp_nmaps > 8 ? (8 * MB_MRSIZE) : lp->lp_nmaps * MB_MRSIZE;
			if (lp->lp_nmaps < 1) {
				printf("lp%d: Line %d no maps, deconfigured, csr=0x%x\n",
					board, unit, lp->lp_base);
				/*
				 *+ No maps have been configured for the
				 *+ lp device.  Corrective action:  check the system and device 
				 *+ configuration files. 
				 */
				lp->lp_alive = 0;
				continue;
			}
			/*
			 * If there exist more printers than we configured,
			 * duplicate the last printer's configuration.
			 */
			if (printer >= lpprinters) {
				printf("lp%d: printer %d not configured, using printer %d's configuration\n", board, printer, lpprinters - 1);
				/*
				 *+ There are more printers than have been
				 *+ specified in the lpconfig table.  Excess
				 *+ printers will use the lpconfig data 
				 *+ specified for the last entry in the table.
				 */
				prindex = lpprinters - 1;
			} else
				prindex = printer;
			printer++;
			/*
			 * allocate the driver's internal buffer space
			 */
			callocrnd (MB_MRSIZE);
			lp->lp_nbuf = lp->lp_startbuf =
				(u_char *)calloc((int)lp->lp_bufsz);
			lp->lp_maddr = mbad_physmap(lp->lp_mbdesc,
				(int)lp->lp_basemap, (caddr_t)lp->lp_startbuf,
				(unsigned)lp->lp_bufsz, (int)lp->lp_nmaps);

			lp->lp_height = lpconfig[prindex].lp_height;
			lp->lp_width = lpconfig[prindex].lp_width;
			lp->lp_ops = lpconfig[prindex].lp_ops;
			lp->lp_lock = &locks[board];
			init_lock(lp->lp_lock, lpgate);
			init_sema(&lp->lp_waitsema, 0, 0, lpgate);
			lp->lp_base->lpd_mode = unit|LPMODE;
			LPSTOP(lp->lp_base->lpd_cmdreg);
		}
		if (lp->lp_alive) {
			lp->lp_base->lpd_csr |= (u_char)LPINTE;	/* enable interrupts */
			sendsoft(lp->lp_mbdesc->mb_slicaddr, (u_char)(1<<lp->lp_mblevel));
		}
	}
}

/*
 * Open the device. We initialize the data structures
 * after the open succeeds. This simplifies the close
 * routine in handling the case when the printer is
 * offline, the open blocks and we signal out of it.
 */
lpopen(dev)
	register dev_t dev;
{
	register struct lp_softc *lp = lp_softc[LPUNIT(dev)];
	spl_t	x;

	LPDEBUG("O");
	if (LPBOARD(dev) >= nlp || !lp->lp_alive)
		return(ENXIO);
	x = p_lock(lp->lp_lock, SPL5);
	if (lp->lp_state & (LP_OPEN|LP_WOPEN)) {	/* exclusive open */
		v_lock(lp->lp_lock, x);
		return(EBUSY);
	}
	while (LPOFFLINE(lp)) {
		lp->lp_state &= ~LP_ONLINE;
		lp->lp_state |= LP_WOPEN;
		timeout(lp_timer, (caddr_t)lp, 2*HZ);
		/*
		 * sleep in lpopen() until the printer comes online.
		 * semantically equivalent to a tty device.
		 */
		p_sema_v_lock(&lp->lp_waitsema, PZERO+1, lp->lp_lock, x);
		x = p_lock(lp->lp_lock, SPL5);
	}
	lp->lp_state &= ~LP_WOPEN;
	lp->lp_state |= LP_OPEN|LP_ONLINE;
	lp->lp_count = 0;
	lp->lp_col = lp->lp_lcol = lp->lp_line = 0;
	if (lp->lp_ops != LPRAW)
		lp_canon(lp, '\f');
	v_lock(lp->lp_lock, x);
	return(0);
}

/*
 * lpclose - close the line and free the buffering.
 */

lpclose(dev)
	dev_t dev; 
{
	spl_t	 x;
	register struct lp_softc *lp = lp_softc[LPUNIT(dev)];

	LPDEBUG("C");
	if (lp->lp_state & LP_WOPEN) {
		/*
	 	* This can happen only if we were partially
	 	* open (ie printer was offline on open) and
	 	* we signalled out of it.
	 	*/
		untimeout(lp_timer, (caddr_t)lp);
		lp->lp_state &= ~LP_WOPEN;
		return(0);
	}
	x = p_lock(lp->lp_lock, SPL5);
	if (lp->lp_ops != LPRAW)
		lp_canon(lp, '\f');
	lp_start(lp);
	while((lp->lp_state & LP_BUSY) || !(lp->lp_state & LP_ONLINE)) {
		/*
		 * Sleep at uninterruptible priority since we need
		 * to ensure that lp_state is cleared properly
		 * (which won't happen if we get signalled out). The
		 * real fix is to setjmp here and abort any pending
		 * dma and clear state if we longjmped back. Not
		 * clear if this is possible with the chip
		 * used on this controller.
		 */
		p_sema_v_lock(&lp->lp_waitsema, PZERO - 1, lp->lp_lock, x);
		x = p_lock(lp->lp_lock, SPL5);
	}
	lp->lp_state &= ~LP_OPEN;
	v_lock(lp->lp_lock, x);
	return(0);
}

/*
 * note that ibuf is used as a buffer between the uio struct and the FS buffer.
 * could be a bottleneck, but can't be bigger since we are on per-proc kernel
 * stack
 */
lpwrite(dev, uio)
	dev_t	 dev;
	struct	 uio	*uio;
{
	register unsigned n;
	register char *cp;
	register struct lp_softc *lp = lp_softc[LPUNIT(dev)];
	spl_t	 x;
	int	error;
	char	 ibuf[LPSIZE];

	while (n = MIN(LPSIZE, (unsigned)uio->uio_resid)) {
		cp = ibuf;
		error = uiomove(cp, (int)n, UIO_WRITE, uio);
		if (error)
			return(error);
		x = p_lock(lp->lp_lock, SPL5);
		do {
			lp_canon(lp, *cp++);
		} while (--n);
		v_lock(lp->lp_lock, x);
	}
	return(0);
}

/*
 * put a char in the buffer and start DMA if buffer is full
 */
lp_putc(lp, c)
	struct	lp_softc *lp;
	register c;
{
	spl_t	 x = SPL0;

	while (!(lp->lp_state & LP_ONLINE) || (lp->lp_state & LP_BUSY)) {
		p_sema_v_lock(&lp->lp_waitsema, PZERO - 1, lp->lp_lock, x);
		x = p_lock(lp->lp_lock, SPL5);
	}
	/*
	 * printer is now online and not in use
	 */
	if (lp->lp_count < lp->lp_bufsz) {
		lp->lp_count++;
		*lp->lp_nbuf++ = c;
	} else {
		panic("lp: lock confusion");
		/*
		 *+ The output count has exceeded the line printer buffer size.
		 *+ This count interlocks output operations so data
		 *+ may have been lost.
		 */
	}
	if (lp->lp_count == lp->lp_bufsz)
		lp_start(lp);
}

/*
 * if printer is offline, run every 2*HZ to see if the printer is
 * online yet.  (see lp_start and lpopen for details).
 */
lp_timer(lp)
	struct	lp_softc *lp;
{
	spl_t	x;

	LPDEBUG("T");
	/*
	 * device may have closed before coming online
	 */
	x = p_lock(lp->lp_lock, SPL5);
	if (LPOFFLINE(lp)) {
		timeout(lp_timer, (caddr_t)lp, 2*HZ);
		v_lock(lp->lp_lock, x);
		return;
	}
	if (lp->lp_state&LP_ONLINE) {		/* sanity */
		v_lock(lp->lp_lock, x);
		return;
	}
	lp->lp_state |= LP_ONLINE;
	/* start output only if device is already open */
	if (lp->lp_state & LP_OPEN)
		lp_start(lp);
	v_lock(lp->lp_lock, x);
	vall_sema(&lp->lp_waitsema);
}

u_char	 tc[2]	= { LPS_TC0, LPS_TC1 };

/*
 * an interrupt is posted whenever a DMA transfer ends.
 * No interrupts when printer goes offline.
 */

lpintr(level)
	int	level;
{
	register u_char	status;
	register offset = (level - lpintr_base) * 2;
	register struct lp_softc *lp;
	spl_t	 x;
	int	 unit;

	LPDEBUG("I");
	lp = lp_softc[offset];
	x = p_lock(lp->lp_lock, SPL5);
	/*
	 * EOP can be lost - the terminal count (TC) is a better indicator
	 * that DMA is through.  Any write to interrupt register clears it.
	 */
	status = lp->lp_base->lpd_intr;
	lp->lp_base->lpd_intr = XX;		/* reset the interrupt */
	status |= (lp->lp_base->lpd_cmdreg & (LPS_TC0|LPS_TC1));
	if (status == LPI_REQUEST) {		/* ignore spurious interrupt */
		v_lock(lp->lp_lock, x);
		mbad_reenable(lp->lp_mbdesc, lp->lp_mblevel);
		return;
	}
	if (status & LPI_MEMERR) {
		printf("lp%d: multibus memory error\n", level - lpintr_base);
                /*
                 *+ The lp driver attempted a transfer to an illegal MULTIBUS
                 *+ address.
                 */
	}
	
	/*
	 * check both halves of the controller for a completion.
	 */
	for (unit = 0; unit < 2; unit++) {
		lp = lp_softc[offset++];
		if (!(lp->lp_state & (LP_ONLINE | LP_BUSY)))	/* sanity */
			continue;
		if (status & tc[unit]) {
			lp->lp_state &= ~LP_BUSY;
			lp->lp_count = 0;
			lp->lp_nbuf = lp->lp_startbuf;
			vall_sema(&lp->lp_waitsema);	/* signal I/O is complete */
		}
	}
	v_lock(lp->lp_lock, x);
	mbad_reenable(lp->lp_mbdesc, lp->lp_mblevel);
}

/*
 * assumes that caller has the board locked.
 */
lp_start(lp)
	register struct lp_softc *lp;
{
	register int	cnt;
	register int	addr;
	register int	unit = lp->lp_unit;

	LPDEBUG("S");
	if ((lp->lp_state & LP_BUSY)
		|| !(lp->lp_state & LP_ONLINE)
		|| (lp->lp_count == 0))
			return;

	if (LPOFFLINE(lp)) {
		lp->lp_state &= ~LP_ONLINE;
		timeout(lp_timer, (caddr_t)lp, 2*HZ);
		return;
	}
	lp->lp_state |= LP_BUSY;

	/*
	 * figure multibus address and current buffer count
	 */
	addr = (int)lp->lp_maddr;
	cnt = lp->lp_count - 1;
	LPSTOP(lp->lp_base->lpd_cmdreg);
	/*
	 * to transfer the LSB first, must instruct the 8237 (clear byte ptr)
	 */
	lp->lp_base->lpd_clbp = XX;
	if (unit == 0) {
		lp->lp_base->lpd_cnt0 = (u_char)cnt;
		lp->lp_base->lpd_cnt0 = (u_char)(cnt>>8);
		lp->lp_base->lpd_addr0 = (u_char)addr;
		lp->lp_base->lpd_addr0 = (u_char)(addr>>8);
		lp->lp_base->lpd_hiaddr0 = (u_char)(addr>>16);
	} else {
		lp->lp_base->lpd_cnt1 = (u_char)cnt;
		lp->lp_base->lpd_cnt1 = (u_char)(cnt>>8);
		lp->lp_base->lpd_addr1 = (u_char)addr;
		lp->lp_base->lpd_addr1 = (u_char)(addr>>8);
		lp->lp_base->lpd_hiaddr1 = (u_char)(addr>>16);
	}
	/*
	 * request that a write be started on the right unit.  This
	 * tells the DMA that software requests can happen on this
	 * unit.  It only needs to be done once per channel.
	 */
	if ((lp->lp_state&LP_NO_REQ)==0) {
		lp->lp_state |= LP_NO_REQ;
		lp->lp_base->lpd_req = (u_char)(LPR_SET | unit);
	}
	/*
	 * interrupt enable latch gets cleared on self-test.
	 */
	lp->lp_base->lpd_csr |= (u_char)LPINTE;		/* enable interrupts */
	lp->lp_base->lpd_bmask = (u_char)unit;		/* clear mask bit */
	LPSTART(lp->lp_base->lpd_cmdreg);
	mbad_reenable(lp->lp_mbdesc, lp->lp_mblevel);
}

/*
 * cannonical output - do some editing unless raw.
 * Must be called with the lp locked.
 */
lp_canon(lp, c)
	register struct lp_softc *lp;
	register int c;
{
	register int	logcol;

	if (lp->lp_ops == LPRAW) {
		lp_putc(lp, c);
		return;
	}

	logcol = lp->lp_lcol;

	if (lp->lp_ops == LPCAPS) {
		register c2;

		if (c >= 'a' && c <= 'z')
			c += 'A'-'a'; else
		switch (c) {

		case '{':
			c2 = '(';
			goto esc;

		case '}':
			c2 = ')';
			goto esc;

		case '`':
			c2 = '\'';
			goto esc;

		case '|':
			c2 = '!';
			goto esc;

		case '~':
			c2 = '^';

		esc:
			lp_canon(lp, c2);
			c = '-';
		}
	}

	if (c==' ')
		logcol++;
	else
		switch(c) {

	case '\t':
		logcol = (logcol + 8) & ~7;
		break;

	case '\b':
		if (logcol > 0)
			logcol--;
		break;

	case '\f':
		if (lp->lp_line == 0 && lp->lp_col == 0)
			break;
		/* falls into ... */

	case '\n':
		lp_putc(lp, c);
		if (c == '\f')
			lp->lp_line = 0;
		else
			lp->lp_line++;
		lp->lp_col = 0;
		/* falls into ... */

	case '\r':	
		logcol = 0;
		break;

	default: 	/* must be a printable character */
		/*
		* Have we backspaced over the beginning of
		* line?
		*/
		if (logcol < lp->lp_col) {
			lp_putc(lp, '\r');
			lp->lp_col = 0;
		}

		if (logcol < lp->lp_width) {
			while (logcol > lp->lp_col) {
				lp_putc(lp, ' ');
				lp->lp_col++;
			}
			lp_putc(lp, c);
			lp->lp_col++;
		}
		logcol++;
	}
	lp->lp_lcol = logcol;

	if (lp->lp_lcol > LPLONGLINE)	/* ignore long lines */
		lp->lp_lcol = LPLONGLINE;
}
