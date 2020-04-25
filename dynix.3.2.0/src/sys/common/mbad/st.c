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
static	char	rcsid[] = "$Header: st.c 2.25 90/12/13 $";
#endif

/*
 * Systech MTI 800/1600/1650 Driver
 */

/* $Log:	st.c,v $
 *
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/kernel.h"
#include "../h/conf.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/systm.h"
#include "../h/clist.h"
#include "../h/file.h"
#include "../h/vmmac.h"
#include "../h/vmmeter.h"
#include "../h/vmsystm.h"
#include "../h/uio.h"
#include "../h/cmn_err.h"

#include "../machine/ioconf.h"
#include "../machine/gate.h"
#include "../machine/pte.h"
#include "../machine/vmparam.h"
#include "../machine/plocal.h"
#include "../machine/intctl.h"
#include "../machine/hwparam.h"

#include "../balance/clock.h"
#include "../balance/slic.h"

#include "../mbad/mbad.h"
#include "../mbad/st.h"


# ifdef DEBUG
int	stdebug = 0;	/* 0=none,1=little,2=more,3=lots,4=all */
#define STDEBUG(x)	if (stdebug) CPRINTF("x")
# else
#define STDEBUG(x)
# endif DEBUG

struct	cblock *getcf();
int	ttrstrt(), ststart(), stprobe(), stboot(), stintr();

struct	mbad_driver st_driver =
	{ "st", MBD_TYPICAL|MBD_CLIST, stprobe, stboot, stintr };

/* Length of responses, commands, and config commands */
static u_char resplen[] =   { 3,2,2,0,0,0,0,2,6,6,0,0,5,0,0,0 };
static u_char cmdlen[] =    { 1,1,1,1,2,2,1,5,6,7,1,1,6,1,1,0 };
static u_char configlen[] = { 5,8,0,9,3,5,3,3,5,2,0,0,0,0,0,0 };

/* Mapping of timer constants to ms. */
static short timer_ms[] = { 530,353,241,197,177,88,44,22,15,13,11,7,5,4,3,1 };

/*
 * Table for configuring input and output buffer sizes.
 * Rules enforced by the MTI firmware are:
 *  1) Sum of buffer sizes must not exceed 16*1024
 *  2) Each buffer size must be a multiple of 8
 *  3) Each buffer size must be >= 32
 * Configure the minimum for output, the rest for input.
 */
#define	OS	(CBSIZE * 2)	/* min output size */
#define IS	(1024 - OS)	/* rest for input */
static u_short bufsz_tbl[] = {
	IS, OS, IS, OS, IS, OS, IS, OS, IS, OS, IS, OS, IS, OS, IS, OS,
	IS, OS, IS, OS, IS, OS, IS, OS, IS, OS, IS, OS, IS, OS, IS, OS,
};

struct	stinfo	**stinfo;	/* per board data */
int	stbase;			/* base interrupt vector */
int	nst;			/* number of configured boards */
struct	stinfo *st_timer;	/* systech providing timer */
bool_t	st_timer_on;		/* true when timer is on */
int	st_timer_ms;		/* measured frequency of timer in ms */

extern	int	sttimerrate;	/* frequency at which sttimer() runs */
extern	struct	stlh stlh[];	/* block input low/high water */
extern	int	sttimerave;	/* how fast rate changes */
extern	int	stvdwait;	/* number of loops to wait for valid data */
extern	int	stprintoverflow;/* non-zero prints message on input overflow */
extern	int	ststopbits;	/* default number of stop bits */
extern	int	stfifotimeout;	/* timeout count on fifo */

/*
 * Probe for the existance of a board.
 * NB: The read following the write is necessary 
 * to sync up with the hardware write buffer.
 */

stprobe(mp)
	struct mbad_probe *mp;
{
	struct stdevice *staddr;

	staddr = (struct stdevice *) &mp->mp_desc->mb_ios->mb_io[mp->mp_csr];
	staddr->stie = 0;				/* Mask Interrupts */
	return((staddr->ststat & ST_ERR) == 0);		/* Errors? */
}

/*
 * Allocate driver data structures
 */
stboot(n, md)
	int n;				/* configured boards */
	struct mbad_dev *md;		/* multibus descriptor */
{
	register struct stdevice *staddr;
	register struct stinfo *ip;
	register int line;
	register struct tty *tp;
	register int board;
	extern	gate_t	stgate;
	u_char	mask;
	int	nbi = 0;

	stbase = md->md_vector;
	if (n) {
		nst = n;
		stinfo = (struct stinfo **)calloc(sizeof(struct stinfo *)*nst);
	}

	/*
	 * For each configured board ...
	 *   allocate info data structure (for live boards)
	 *   For each line ...
	 *     set it up
	 */
	for (board = 0; board < nst; board++,md++) {
		stinfo[board] = NULL;
		if (md->md_alive == 0)
			continue;
		ip = stinfo[board] =
		    (struct stinfo *) calloc(sizeof(struct stinfo));
		staddr =
		    (struct stdevice *) &md->md_desc->mb_ios->mb_io[md->md_csr];
		ip->st_addr = staddr;
		init_lock(&ip->st_lock, stgate);
		ip->st_cflags = md->md_flags;
		ip->st_size = 0;
		ip->st_vector = md->md_vector;
		ip->st_mblevel = md->md_level;
		ip->st_mbdesc = md->md_desc;
		ip->st_response_index = 0;
		ip->st_response_length = 0;
		/*
		 * Detect revision of firmware
		 */
		if (st_wait(staddr, ST_READY)) {
			printf("st%d not responding.\n", board);
                        /*
                         *+ The Systech board is not responding to the driver.
                         *+ The user will not be allowed to access the device.
                         */
			continue;
		}
		staddr->stcmd = ST_RBDATA;		/* illegal on old FW */
		staddr->stgo = 1;
		if (st_wait(staddr, ST_READY)) {
			printf("st%d not responding.\n", board);
                        /*
                         *+ The Systech board is not responding to the driver.
                         *+ The user will not be allowed to access the device.
                         */
			continue;
		}
		if (staddr->ststat & ST_ERR) {
			staddr->stcmd = ST_RERR;
			staddr->stgo = 1;
			if (st_wait(staddr, ST_READY) || st_flush(staddr)) {
				printf("st%d not responding.\n", board);
                                /*
                                 *+ The Systech board is not responding to the 
				 *+ driver.
                                 *+ The user will not be allowed to access the
                                 *+ device.
                                 */

				continue;
			}
		} else {
			unsigned int mb_addr; 
			ip->st_cflags |= STF_BI;
			/*
			 * Borrow the last mbad map register to
			 * configure input/output buffers.
			 */
			mb_addr = mbad_physmap(md->md_desc, MB_MAPS-2,
				(caddr_t) bufsz_tbl, sizeof(bufsz_tbl), 2);
			staddr->stcmd = ST_CONFIG;
			staddr->stcmd = STC_BUFSIZ;
			staddr->stcmd = mb_addr;
			staddr->stcmd = mb_addr >> 8;
			staddr->stcmd = mb_addr >> 16;
			staddr->stgo = 1;
			if (st_wait(staddr, ST_READY)) {
				printf("st%d not responding.\n", board);
                                /*
                                 *+ The Systech board is not responding to the 
				 *+ driver.
                                 *+ The user will not be allowed to access the
                                 *+ device.
                                 */
				continue;
			}
			if (staddr->ststat & ST_ERR) {
				printf("st%d bad bufsz_tbl config.\n", board);
                                /*
                                 *+ The Systech board responded to the 
				 *+ 'configure buffer sizes' command with an 
				 *+ error.
                                 */
				staddr->stcmd = ST_RERR;
				staddr->stgo = 1;
				if (st_wait(staddr, ST_READY) 
				  || st_flush(staddr)) {
					printf("st%d not responding.\n", board);
                                        /*
                                         *+ The Systech board is not 
					 *+ responding to the driver.
                                         *+ The user will not be allowed to 
					 *+ access the device.
                                         */
					continue;
				}
			}
		}

		/*
		 * Init each line on a board.
		 * Any errors here (other than MTI-800 detect) cause us to
		 * disable the entire board.
		 */
		for (tp = ip->st_tty,line = 0; line < 16; tp++,line++) {
			/*
			 * Turn off each usart modem control outputs
			 * (RTS, DTR) and disable usart xmit and receive.
			 */
			staddr->stcmd = (ST_WCMD | line);
			staddr->stcmd = 0;
			staddr->stgo = 1;
			/* no resp expected */
			if (st_wait(staddr, ST_READY))
				break;
			/*
			 * A command error will occur on non-existent lines.
			 * Use this fact to detect 8 line MTI-800 boards.
			 */
			if (staddr->ststat & ST_ERR) {
				staddr->stcmd = ST_RERR;
				staddr->stgo = 1;
				(void) st_wait(staddr, ST_READY);
				(void) st_flush(staddr);
				break;
			}
			ttyinit(tp, stgate);
			tp->t_cmask = DCD;
			/*
			 * Configure block output
			 */
			staddr->stcmd = (ST_CONFIG | line);
			staddr->stcmd = STC_OUTPUT;
			staddr->stcmd = 1;
			staddr->stgo = 1;
			/* no resp expected */
			if (st_wait(staddr, ST_READY))
				break;
			/*
			 * Configure single char input (unless block input).
			 */
			if ((ip->st_cflags & STF_BI) == 0) {
				staddr->stcmd = (ST_ESCI | line);
				staddr->stgo = 1;
				/* no resp expected */
				if (st_wait(staddr, ST_READY))
					break;
			}
			/*
			 * Configure modem status changes to be reported.
			 */
			staddr->stcmd = (ST_CONFIG | line);
			staddr->stcmd = STC_MODEM;
			staddr->stcmd = 1;
			staddr->stgo = 1;
			/* no resp expected */
			if (st_wait(staddr, ST_READY))
				break;
		}
		if (line != 8 && line != 16) {
			printf("st%d: unusable, line %d bad\n", board, line);
                        /*
                         *+ The Systech board has one or more unusable lines,
                         *+ making the board inaccessible to the user.
                         */
			continue;
		}
		ip->st_size = line;
		if (ip->st_cflags & STF_BI)
			++nbi;
		ip->st_addr->stie = STI_RA;	/* interrupt enable mask */
		mask = 1 << ip->st_mblevel;
		sendsoft(ip->st_mbdesc->mb_slicaddr, mask);
	}
	if (nst && nbi)
		st_picktimer();
}

/*
 * Pick a board to use to time out block
 * input commands.  The actual timer accuracy
 * varies greatly between boards so we look for
 * the one nearest the desired timeout.
 */
st_picktimer()
{
	register struct stinfo *ip;
	register struct stdevice *staddr;
	register struct cpuslic *sl = va_slic;
	int near = 0x7fffffff;
	u_char reload_value;
	int i, n, time_in_ms;

	/* 
	 * setup SLIC timer for 10 ms.
	 */
	sl->sl_tctl = 0;
	reload_value = (sys_clock_rate * 1000000) / (SL_TIMERDIV * 100) - 1;
	sl->sl_trv = reload_value;

	for (i = 0; i < nst; i++) {
		if ((ip = stinfo[i]) == NULL || ip->st_size == 0)
			continue;
		staddr = stinfo[i]->st_addr;

		/* load board timer */
		if (st_wait(staddr, ST_READY))
			continue;
		staddr->stcmd = ST_CONFIG | sttimerrate;
		staddr->stcmd = STC_TIMER;
		staddr->stgo = 1;
		if (st_wait(staddr, ST_READY))
			continue;
		n = staddr->stclrtim;		/* clear timer */
		while ((staddr->ststat & ST_TIMER) == 0)
			/* void */;
		n = staddr->stclrtim;		/* clear timer */

		/* load slic timer */
		sl->sl_tcont = reload_value;

		/*
		 * Loop until timer on board ticks again
		 * and check slic timer to see how much
		 * time has gone by.
		 */
		time_in_ms = 0;
		while ((staddr->ststat & ST_TIMER) == 0) {
			if (sl->sl_tcont == 0) {
				time_in_ms += 10;	/* wrap every 10ms */
				while (sl->sl_tcont == 0)
					/* spin */;
			}
		}
		time_in_ms += (reload_value - sl->sl_tcont) / 
			((sys_clock_rate * 1000000) / (SL_TIMERDIV * 1000));
		n = time_in_ms - timer_ms[sttimerrate];
		if (n < 0)
			n = -n;
		if (n < near) {
			st_timer = stinfo[i];
			st_timer_ms = time_in_ms;
			near = n;
		}
	}
	/*
	 * If the timer picked is more than 
	 * 10 percent out of spec, scale the values
	 */
	if ((10*near) > timer_ms[sttimerrate]) {
		for (i=0; i < 16; i++) {
			stlh[i].st_high = (stlh[i].st_high * st_timer_ms) / 
						timer_ms[sttimerrate];
			stlh[i].st_low = (stlh[i].st_low * st_timer_ms) / 
						timer_ms[sttimerrate];
		}
	}
}

/*
 * Flush a response from the response fifo.
 * Return 0 for success, 1 if timeout occurs.
 * Called only if a response is expected.
 * Called from stboot() only.
 */
st_flush(staddr)
	register struct stdevice *staddr;
{
	register int n, temp;

	if (st_wait(staddr, ST_RA))
		return (1);
	temp = staddr->stcra;
	temp = staddr->stresp;
	n = resplen[temp >> 4];
	while (--n > 0) {
		if (st_wait(staddr, ST_VD))
			return (1);
		/* discard the response itself */
		temp = staddr->stresp;
	}
	return (0);
}

/*
 * Open a line.
 */
/*ARGSUSED*/
stopen(dev, flag)
	dev_t dev;
	int flag;
{
	register struct tty *tp;
	register struct stinfo *ip;
	register int line = STLINE(dev);
	register int board = STBOARD(dev);
	spl_t s;
	struct cblock *cb;
	int error;

	STDEBUG(O);
	if ( stinfo == NULL || board >= nst || (ip = stinfo[board]) == NULL
	  || line >= ip->st_size)
		return (ENXIO);
	tp = &ip->st_tty[line];
	s = p_lock(&tp->t_ttylock, SPLTTY);
	if ((tp->t_state & TS_XCLUDE) && u.u_uid != 0) {
		v_lock(&tp->t_ttylock, s);
		return (EBUSY);
	}
	tp->t_nopen++;
	while (tp->t_state & TS_LCLOSE) {	/* wait on last close */
		tp->t_state |= TS_WOPEN;
		p_sema_v_lock(&tp->t_rawqwait, TTIPRI, &tp->t_ttylock, s);
		s = p_lock(&tp->t_ttylock, SPLTTY);
	}
	/*
	 * If this is first open, initialize tty state to default.
	 */
	if ((tp->t_state & TS_ISOPEN) == 0) {
		tp->t_dev = dev;
		tp->t_oproc = ststart;
		tp->t_addr = (caddr_t)ip->st_addr;
		if (tp->t_ispeed == 0) {
			extern int stflags, stspeed;
			/* default line characteristics */
			tp->t_ospeed = tp->t_ispeed = stspeed;
			tp->t_flags = stflags;
		}
		ttychars(tp);
		(void) st_param(ip, line);	/* setup default baud rate */
		if (ip->st_cflags & STF_BI) {
			/*
			 * Block input will be used on input so
			 * get a cblock and start up a block input.
			 */
			if ((cb = ip->st_cblocks[line]) == NULL) {
				int addr;
				while ((cb = getcf()) == NULL) {
					v_lock(&tp->t_ttylock, s);
					p_sema(&lbolt, TTOPRI);
					s = p_lock(&tp->t_ttylock, SPLTTY);
				}
				ip->st_cblocks[line] = cb;
				st_cmd(ip, ST_CONFIG|line, STC_INPUT|0x08, 1);
				addr = CLTOMB(ip->st_mbdesc, cb->c_info);
				st_cmd(ip, ST_BLKIN|line,
					addr, addr>>8, addr>>16, 1, 0);
			}
			/*
			 * Start up timer to handle block input timouts
			 * No mutex because losing the race is harmless
			 */
			if (!st_timer_on) {
				st_timer_on = 1;
				st_timer->st_addr->stie = (STI_RA|STI_TIMER);
			}
		}
	}
	/*
	 * Wait for carrier unless O_NDELAY flag is set.
	 */
	if ((tp->t_softcarr & SOFT_CARR) || (flag & O_NDELAY)) {
		tp->t_state |= TS_CARR_ON;
		st_cmd(ip, ST_WCMD|line, RTS|DTR|RXEN|TXEN);
	} else {
		st_cmd(ip, ST_WCMD|line, RTS|DTR|RXEN|TXEN);
		st_cmd(ip, ST_RSTAT|line);
		while ((tp->t_state & TS_CARR_ON) == 0) {
			tp->t_state |= TS_WOPEN;
			p_sema_v_lock(&tp->t_rawqwait, PZERO+1,
				&tp->t_ttylock, s);
			s = p_lock(&tp->t_ttylock, SPLTTY);
		}
	}
	error = (*linesw[tp->t_line].l_open)(dev, tp);
	if (error)
		tp->t_nopen--;
	v_lock(&tp->t_ttylock, s);
	return (error);
}

/*
 * Close a line.
 */
/*ARGSUSED*/
stclose(dev, flag)
	dev_t dev;
	int flag;
{
	register int line = STLINE(dev);
	register struct stinfo *ip = stinfo[STBOARD(dev)];
	register struct tty *tp = &ip->st_tty[line];
	spl_t s;

	STDEBUG(C);
	s = p_lock(&tp->t_ttylock, SPLTTY);
	if (--tp->t_nopen > 0 || (tp->t_state & TS_LCLOSE)) {
		v_lock(&tp->t_ttylock, s);
		return;
	}
	tp->t_state |= TS_LCLOSE;
	(*linesw[tp->t_line].l_close)(tp);
	if ((tp->t_state & TS_ISOPEN) == 0 || (tp->t_state & TS_HUPCLS)) {
		if (tp->t_softcarr&SOFT_CARR)
			st_cmd(ip, ST_WCMD|line, RXEN|TXEN|RTS);
		else
			st_cmd(ip, ST_WCMD|line, RXEN|TXEN);
	}
	ttyclose(tp);
	v_lock(&tp->t_ttylock, s);
}

stread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;
	int error;
	spl_t s;

	tp = &stinfo[STBOARD(dev)]->st_tty[STLINE(dev)];
	s = p_lock(&tp->t_ttylock, SPLTTY);
	error = (*linesw[tp->t_line].l_read)(tp, uio);
	v_lock(&tp->t_ttylock, s);
	return (error);
}

stwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;
	int error;
	spl_t s;

	tp = &stinfo[STBOARD(dev)]->st_tty[STLINE(dev)];
	s = p_lock(&tp->t_ttylock, SPLTTY);
	error = (*linesw[tp->t_line].l_write)(tp, uio);
	v_lock(&tp->t_ttylock, s);
	return (error);
}

/*
 * Interrupt service routine.
 */
stintr(vector)
	int vector;
{
	register struct stdevice *staddr;
	register u_char *p;
	register int n, h;
	register int temp;
	register int board = vector - stbase;
	int nresponses;
	struct stinfo *ip;

	STDEBUG(I);
	if ((unsigned)board >= nst || (ip = stinfo[board]) == NULL) {
		printf("st%d: unknown interrupt, vector %d\n", board, vector);
                /*
                 *+ An interrupt was received from unknown source. 
		 *+  The interrupt is ignored.
                 */
		return;
	}
	staddr = ip->st_addr;
	temp = staddr->stcra;
	p = ip->st_response;
	n = ip->st_response_index;
	h = ip->st_response_length;
	for (nresponses = 0; nresponses < 100; ) {
		if (ip == st_timer && (staddr->ststat & ST_TIMER)) {
			temp = staddr->stclrtim;	/* clear timer */
			sttimer();
		}
		temp = 0;
		while ((staddr->ststat & ST_VD) == 0) {	/* valid data? */
			if (++temp > stvdwait)
				goto out;
		}
		p[n] = staddr->stresp;			/* read response byte */
		if (n == 0) {
			if ((h = resplen[p[0]>>4]) == 0) {
				printf("st%d: response 0x%x", board, p[0]);
                                /*
                                 *+ The Systech board responded to a driver 
				 *+ command with an unexpected response length.
				 *+ The response is discarded.
                                 */
				continue;
			}
		}
		if (++n >= h) {
			n = 0;
			++nresponses;
			stresponse(ip, board);
		}
	}
	/*
	 * Done when no responses in response queue,
	 * processed 100 responses, or board is taking
	 * too long to put bytes into the response fifo.
	 * Save where we are so if we have a partial
	 * response, we can resume later.
	 */
out:
	ip->st_response_index = n;
	ip->st_response_length = h;
	mbad_reenable(ip->st_mbdesc, ip->st_mblevel);
	STDEBUG(D);
}

/*
 * Process a response.
 */
stresponse(ip, board)
	register struct stinfo *ip;
	int board;
{
	register u_char *buf = ip->st_response;
	int line = (buf[0] & 0x0f);
	register struct tty *tp = &ip->st_tty[line];
	register u_char *cp;
	struct cblock *cb;
	register int n;
	int cnt, addr;
	spl_t s;

	switch (buf[0] & 0xf0) {

	case ST_ESCI:			/* Single Char Input */
		if (stprintoverflow && (buf[1] & OE)) {
			printf("st%d: silo overflow on line %d\n", board, line);
			/*
			 *+ A Systech input silo overflowed;
			 *+ the Systech lost data.
			 */
		}

		s = p_lock(&tp->t_ttylock, SPLTTY);
		if (buf[1] & FE) {	/* framing error */
			if (tp->t_flags & RAW)
				buf[2] = 0;	/* for getty */
			else
				buf[2] = tp->t_intrc;
		}
		if (tp->t_state & TS_ISOPEN)
			(*linesw[tp->t_line].l_rint)(buf[2], tp);
		v_lock(&tp->t_ttylock, s);
		break;

	case ST_BLKOUT:		/* Block Output */
		if (buf[2] & STE_MEMERR) {
			printf("st%d: block output memory error, line %d",
				board, line);
			/*
			 *+ The Systech board reported a block input memory
			 *+ address error.
			 */

			st_unexp(buf);
		}
		cnt = buf[3] | (buf[4] << 8);
		if (cnt < 0 || cnt > CBSIZE) {
			printf("st%d: block output count %d line %d\n",
				board, cnt, line);
			/*
			 *+ The Systech board reported a block ouput count
			 *+ that was out of range.
			 */

			st_unexp(buf);
			break;
		}
		s = p_lock(&tp->t_ttylock, SPLTTY);
		tp->t_state &= ~TS_BUSY;
		if (tp->t_state & TS_FLUSH)
			tp->t_state &= ~TS_FLUSH;
		else {  /* delete chars xfered from queue */
			ndflush(&tp->t_outq, (int)(buf[3]|(buf[4]<<8)));
		}
		if (tp->t_line)
			(*linesw[tp->t_line].l_start)(tp);
		else
			ststart(tp);
		v_lock(&tp->t_ttylock, s);
		break;

	case ST_BLKIN:		/* block input */
		cnt = buf[3] | (buf[4] << 8);
		if (tp->t_state & TS_ISOPEN) {
			if (stprintoverflow && (buf[1] & OE)) {
				printf("st%d: silo overflow on line %d\n",
					board, line);
				/*
				 *+ A Systech input silo overflowed;
				 *+ the Systech lost data.
				 */
			}
			if (cnt < 0 || cnt > CBSIZE) {
				printf("st%d: block input count %d line %d\n",
					board, cnt, line);
				/*
				 *+ The Systech board reported a block input 
				 *+ count that was out of range.
				 *+ The data is discarded.
				 */
				st_unexp(buf);
				cnt = 0;
			}
			if (buf[2] & STE_MEMERR) {
				printf("st%d: block input mem err on line %d,",
					board, line);
                                /*
                                 *+ The Systech board reported a block input
				 *+ memory address error.
                                 */
				st_unexp(buf);
			}
		}
		if ((cb = ip->st_cblocks[line]) == NULL) {
			printf("st%d: response on uninitialized line %d\n",
				board, line);
			/*
			 *+ The Systech board responded on a line that has
			 *+ not been enabled.
			 */
			st_unexp(buf);
			break;
		}
		s = p_lock(&tp->t_ttylock, SPLTTY);
		cp = (u_char *)cb->c_info;
		if (buf[1] & FE) {
			cnt = 1;
			if (tp->t_flags & RAW)
				*cp = 0;	/* for getty */
			else
				*cp = tp->t_intrc;
		}
		n = 1;
		if (cnt > 0) {
			ip->st_chars[line] += cnt;
			if (tp->t_state & TS_ISOPEN) {
				/*
				 * Attempt to use fastpath input.
				 * Loop on ttyinput if that fails.
				 */
				cnt = stfastin(&ip->st_cblocks[line], tp, cnt);
				while (cnt-- > 0)
					(*linesw[tp->t_line].l_rint)(*cp++, tp);
			}
			if (ip->st_mode[line] == FAST) {
				n = CBSIZE;
			} else {
				cnt = stlh[tp->t_ispeed].st_high;
				if (ip->st_chars[line] > cnt) {
					ip->st_rate[line] = cnt << 1;
					ip->st_mode[line] = FAST;
					n = CBSIZE;
				}
			}
		}
		addr = CLTOMB(ip->st_mbdesc, ip->st_cblocks[line]->c_info);
		st_cmd(ip, ST_BLKIN|line, addr, addr>>8, addr>>16, n, n>>8);
		v_lock(&tp->t_ttylock, s);
		break;

	case ST_RERR:		/* Command Error */
		printf("st%d: command error", board);
                /*
                 *+ The Systech board reported a command error.
                 */
		st_unexp(buf);
		break;

	case STC_MODEM:		/* modem bits changed */
	case ST_RSTAT:		/* read modem status */
		s = p_lock(&tp->t_ttylock, SPLTTY);
		if (buf[1] & tp->t_cmask) {
			tp->t_state |= TS_CARR_ON;
			vall_sema(&tp->t_rawqwait);	/* wake every body up */
		} else {
			if (((tp->t_state & (TS_WOPEN|TS_ISOPEN)) == TS_ISOPEN)
			&& ((tp->t_flags & NOHANG) == 0)) {
				gsignal(tp->t_pgrp, SIGHUP);
				gsignal(tp->t_pgrp, SIGCONT);
				ttyflush(tp, FREAD|FWRITE);
			}
			tp->t_state &= ~TS_CARR_ON;
		}
		v_lock(&tp->t_ttylock, s);
		break;

	default:
		printf("st%d: unexpected", board);
                /*
                 *+ An unexpected response was received from 
		 *+ the Systech board.
                 */

		st_unexp(buf);
		break;
	}
}

/*
 * Process ioctls
 * Assumes tp is unlocked.
 */
stioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	int flag;
	caddr_t	data;
{
	int line = STLINE(dev);
	register struct stinfo *ip = stinfo[STBOARD(dev)];
	register struct tty *tp = &ip->st_tty[line];
	register int error;
	spl_t s;

	STDEBUG(L);
	s = p_lock(&tp->t_ttylock, SPLTTY);
	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
	if (error >= 0) {
		v_lock(&tp->t_ttylock, s);
		return (error);
	}
	error = ttioctl(tp, cmd, data, flag);
	if (error >= 0) {
		switch (cmd) {
		case OLD_TIOCSETP:
		case TIOCSETP:
		case OLD_TIOCSETN:
		case TIOCSETN:
		case TIOCLBIS:
		case TIOCLBIC:
		case TIOCLSET:
			error = st_param(ip, line);
			break;
		}
		v_lock(&tp->t_ttylock, s);
		return (error);
	}

	/*
	 * Process special stuff
	 */
	error = 0;
	switch (cmd) {
		case TIOCSBRK:	/* set break on */
			st_cmd(ip, ST_WCMD|line, SBRK|RTS|DTR|RXEN|TXEN);
			break;
		case TIOCCBRK:	/* clear break off */
			st_cmd(ip, ST_WCMD|line, RTS|DTR|RXEN|TXEN);
			break;
		case TIOCSDTR:	/* Turn on dtr rts */
			if (tp->t_softcarr&SOFT_CARR)
				tp->t_state |= TS_CARR_ON;
			st_cmd(ip, ST_WCMD|line, RTS|DTR|RXEN|TXEN);
			break;
		case TIOCCDTR:	/* turn off dtr rts */
			if (tp->t_softcarr&SOFT_CARR)
				st_cmd(ip, ST_WCMD|line, RXEN|TXEN|RTS);
			else
				st_cmd(ip, ST_WCMD|line, RXEN|TXEN);
			break;
		default:
			error = ENOTTY;
	}
	v_lock(&tp->t_ttylock, s);
	return (error);
}

/*
 * Select routine.
 * Assumes tp is unlocked.
 */
stselect(dev, rw)
	dev_t dev;
	int rw;
{
	register struct tty *tp = &stinfo[STBOARD(dev)]->st_tty[STLINE(dev)];
	int error;
	spl_t s;

	s = p_lock(&tp->t_ttylock, SPLTTY);
	error = (*linesw[tp->t_line].l_select)(tp, rw);
	v_lock(&tp->t_ttylock, s);
	return (error);
}

/*
 * This table defines the ST command bits for setting asynchronous
 * baud rates.  '-1' is the signal to turn off the DTR and RTS signals.
 * '-2' indicates that a baud rate is not available on the ST.  The
 * 'Mxxx' entries are for the 'rate' parameter in calls to
 * 'st_conf_async.'
 */

int st_mrates[] = {
	-1,			/* B0	  (hangup)	*/
	STB_M50,		/* B50			*/
	STB_M75,		/* B75			*/
	STB_M110,		/* B110			*/
	STB_M134,		/* B134			*/
	STB_M150,		/* B150			*/
	-2,			/* B200	  (not avail.)	*/
	STB_M300,		/* B300			*/
	STB_M600,		/* B600			*/
	STB_M1200,		/* B1200		*/
	STB_M1800,		/* B1800		*/
	STB_M2400,		/* B2400		*/
	STB_M4800,		/* B4800		*/
	STB_M9600,		/* B9600		*/
	STB_M19200,		/* B19200		*/
	-2			/* EXTB	  (not used)	*/
};

/*
 * Set line parameters on the hardware.
 * Assumes tp is locked.
 */
st_param(ip, line)
	register struct stinfo *ip;
	int line;
{
	register struct tty *tp = &ip->st_tty[line];
	register int speed = st_mrates[tp->t_ispeed];
	int stopbits;
	u_char par, len;

	STDEBUG(P);
	if (speed == -2)
		return (EINVAL);
	if (speed == -1) {
		tp->t_state |= TS_HUPCLS;
		if (tp->t_softcarr&SOFT_CARR)
			st_cmd(ip, ST_WCMD|line, RXEN|TXEN|RTS);
		else
			st_cmd(ip, ST_WCMD|line, RXEN|TXEN);
		return (0);				/* ok */
	}
	if (speed == STB_M134) {
		len = STB_BITS6;
		par = STB_ODD_PARITY;
	} else if (tp->t_flags & (RAW|LITOUT|PASS8)) {
		len = STB_BITS8;
		par = STB_NO_PARITY;
	} else if ((tp->t_flags & (EVENP|ODDP)) == EVENP) {
		len = STB_BITS7;
		par = STB_EVEN_PARITY;
	} else if ((tp->t_flags & (EVENP|ODDP)) == ODDP) {
		len = STB_BITS7;
		par = STB_ODD_PARITY;
	} else {
		len = STB_BITS8;
		par = STB_NO_PARITY;
	}
	stopbits = (speed == STB_M110) ? STB_MSTOP2 : ststopbits;
	st_cmd(ip, ST_CONFIG|line, STC_ASYNC, (CCA2|stopbits|par|len),
		(CCA3|speed), CCA4);
	return (0);
}

/*
 * Start (restart) transmission on the given line.
 * Assumes tp locked.
 */
ststart(tp)
	register struct tty *tp;
{
	register int addr;
	register int nch;
	register struct stinfo *ip;
	u_char line;

	STDEBUG(s);
	/*
	 * If the line is already working, or if it is waiting on
	 * a delay, then nothing is done right now.
	 */
	if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP))
		return;

	/*
	 * If the output queue has emptied to the low threshold, and
	 * if anyone is sleeping on this queue, wake them up.
	 */
	if (tp->t_outq.c_cc <= TTLOWAT(tp)) {
		if (tp->t_state & TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			vall_sema(&tp->t_outqwait);
		}
		if (tp->t_wsel) {
			selwakeup(tp->t_wsel, tp->t_state&TS_WCOLL);
			tp->t_state &= ~TS_WCOLL;
			tp->t_wsel = (struct proc *)NULL;
		}
	}

	if (tp->t_outq.c_cc == 0) 	/* Nothing to process */
		return;

	/*
	 * DMA from clists directly.
	 */
	if (tp->t_flags & (RAW|LITOUT|PASS8)) {
		nch = ndqb(&tp->t_outq, 0);
	} else {
		/*
		 * not raw so check for timeout chars and if we
		 * have dma'd out to one then do the timeout.
		 */
		nch = ndqb(&tp->t_outq, 0200);
		if (nch == 0) {
			nch = getc(&tp->t_outq);
			timeout(ttrstrt, (caddr_t)tp, ((nch & 0x7f) + 6));
			tp->t_state |= TS_TIMEOUT;
			return;
		}
	}
	ip = stinfo[STBOARD(tp->t_dev)];
	line = STLINE(tp->t_dev);
	addr = CLTOMB(ip->st_mbdesc, tp->t_outq.c_cf);
	st_cmd(ip, ST_BLKOUT|line, addr, addr>>8, addr>>16, nch, nch>>8);
	tp->t_state |= TS_BUSY;
}

/*
 * Stop output on a line, e.g. for ^S/^Q or output flush.
 * Assumes tp locked.
 */
/*ARGSUSED*/
ststop(tp, flag)
	register struct tty *tp;
{
	dev_t dev = tp->t_dev;

	STDEBUG(S);
	if (tp->t_state & TS_BUSY) {
		/*
		 * Device is transmitting; abort the block output.
		 */
		if ((tp->t_state & TS_TTSTOP) == 0)
			tp->t_state |= TS_FLUSH;
		st_cmd(stinfo[STBOARD(dev)], ST_ABORTOUT | STLINE(dev));
	}
}

/*
 * Write command to the command fifo.
 * Mutex the board with 'st_lock'.
 */
/*VARARGS1*/
/*ARGSUSED*/
st_cmd(ip, a, b, c, d, e, f, g)
	struct stinfo *ip;
{
	register struct stdevice *staddr = ip->st_addr;
	register int *p = &a;
	register int n;
	int board;
	spl_t s;

	n = ((a & 0xf0) == ST_CONFIG) ? configlen[b>>4] : cmdlen[a>>4];
#ifdef DEBUG
	if (stdebug > 2) {
		int i;
		printf("st_cmd: ");
		for(i=0; i<n; i++) printf("0x%x ", p[i] & 0xff);
		printf("\n");
	}
#endif DEBUG
	s = p_lock(&ip->st_lock, SPLTTY);
	/*
	 * Wait for board to become ready to accept cmds.
	 */
	if (st_wait(staddr, ST_READY)) {
		board = ip->st_vector - stbase;
		printf("st%d: cmd fifo timeout\n", board);
                /*
                 *+ The Systech board was not ready to accept commands after
                 *+ driver wait.
                 */
		v_lock(&ip->st_lock, s);
		return;
	}
	if (staddr->ststat & ST_ERR) {
		/*
		 * Previous command was a command error so clear it.
		 */
		staddr->stcmd = ST_RERR;
		staddr->stgo = 1;
		if (st_wait(staddr, ST_READY)) {
			board = ip->st_vector - stbase;
			printf("st%d: cmd fifo timeout\n", board);
                        /*
                         *+ The Systech board was not ready to accept commands
                         *+ after driver wait.
                         */
			v_lock(&ip->st_lock, s);
			return;
		}
	}
	/*
	 * Load 'n' command bytes into the cmd fifo.
	 * Then return without waiting.
	 */
	while (n-- > 0)
		staddr->stcmd = (u_char) *p++;
	ip->st_addr->stgo = 1;
	v_lock(&ip->st_lock, s);
}

/*
 * Print out an unexpected response.
 */
st_unexp(buf)
	register u_char *buf;
{
	register int n = resplen[*buf>>4];

	CPRINTF(" response = ");
	while (n-- > 0)
		CPRINTF(" 0x%x", *buf++);
	CPRINTF("\n");
}

/*
 * Timer routine.
 * Called from stintr() to abort lines in FAST mode that have not
 * received any data since last timer tick.
 * Changes the mode from FAST to SLOW if rate slows enough.
 */
sttimer()
{
	register struct stinfo *ip;
	register int i, board;
	int stlow;

	for (board = 0; board < nst; board++) {
		ip = stinfo[board];
		if (ip == NULL || (ip->st_cflags & STF_BI) == 0)
			continue;
		for (i = 0; i < ip->st_size; i++) {
			if (ip->st_mode[i] == FAST) {
			        ave(ip->st_rate[i],ip->st_chars[i],sttimerave);
				stlow = stlh[ip->st_tty[i].t_ispeed].st_low;
				if (ip->st_rate[i] < stlow)
					ip->st_mode[i] = SLOW;
				if (ip->st_mode[i] == SLOW || !ip->st_chars[i])
					st_cmd(ip, ST_RBDATA|i);
			}
			if (ip->st_chars[i])
				ip->st_chars[i] = 0;
		}
	}
}

/*
 * getcf - get a free cblock from the cfreelist.
 * return NULL if can't get one.
 */
struct cblock *
getcf()
{
	register struct cblock *bp;
	GATESPL(s);

	P_GATE(G_CFREE, s);
	if ((bp = cfreelist) != NULL) {
		cfreelist = bp->c_next;
		cfreecount -= CBSIZE;
		bp->c_next = NULL;
	}
	V_GATE(G_CFREE, s);
	return (bp);
}

/*
 * Fast path serial input.
 * Purpose is to avoid looping on ttyinput().
 * Assumes tp locked.
 * Returns the number of chars to input the old way.
 */
stfastin(cb, tp, n)
	register struct cblock **cb;
	register struct tty *tp;
{
	register struct clist *q;
	register struct cblock *x, *nb;
	extern int ttyinput();

	if (linesw[tp->t_line].l_rint != ttyinput)
		return (n);
	if ((tp->t_flags & (TANDEM|RAW|ECHO)) != RAW)
		return (n);
	if (tp->t_rawq.c_cc > TTYHOG) {
		ttyflush(tp, FREAD|FWRITE);
		return (0);
	}
 	q = &tp->t_rawq;
	if ((x = (struct cblock *)q->c_cl) == NULL || q->c_cc < 0) {
		if ((nb = getcf()) == NULL)
			return (n);
		q->c_cc = n;
		q->c_cf = &(*cb)->c_info[0];
		q->c_cl = &(*cb)->c_info[n];
		l.cnt.v_ttyin += n;
		ttwakeup(tp);
		*cb = nb;
		return (0);
	} else if (((int)x & CROUND) == 0) {
		if ((nb = getcf()) == NULL)
			return (n);
		q->c_cc += n;
		x[-1].c_next = (*cb);
		q->c_cl = &(*cb)->c_info[n];
		l.cnt.v_ttyin += n;
		ttwakeup(tp);
		*cb = nb;
		return (0);
	}
	return (n);
}

/*
 * Wait for a bit to be set in the status register.
 * Return 0 for success, 1 if timeout occurs.
 */
st_wait(staddr, bit)
	register struct stdevice *staddr;
	register int bit;
{
	register int temp;

	temp = stfifotimeout;
	while ((staddr->ststat & bit) == 0) {
		DELAY(50);
		if (temp-- <= 0)
			return (1);
	}
	return (0);
}
