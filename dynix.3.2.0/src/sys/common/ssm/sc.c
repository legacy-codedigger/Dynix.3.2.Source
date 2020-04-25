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

#ifndef lint
static char rcsid[]= "$Header: sc.c 1.13 1991/07/22 21:36:32 $";
#endif

/*
 *     SSM console driver software
 *
 */

/* $Log: sc.c,v $
 *
 */

#include "../h/param.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/systm.h"
#include "../h/clist.h"
#include "../h/file.h"
#include "../h/mutex.h"
#include "../h/uio.h"
#include "../h/proc.h"
#include "../h/cmn_err.h"
#include "../balance/cfg.h"
#include "../balance/slic.h"
#include "../machine/intctl.h"
#include "../ssm/ioconf.h"
#include "../ssm/ssm.h"
#include "../ssm/ssm_misc.h"
#include "../ssm/sc.h"

int scstart();
extern int ttrstrt();

extern char scspeed;			/* defined in conf_sc.c */
extern int scflags;
extern int scflow;
extern u_long scrxtime;


int scprobe(), scboot(), scintr();
static int scmaxdev;
static int sc_base_vector;
static struct sc_info **sc_info;
static struct sc_cons sc_cons;		/* State information for /dev/console */

struct ssm_driver sc_driver = {
	/* driver prefix, configuration flags, probe(),   boot(), interrupt() */
	      "sc",        SDR_TYPICAL_CONS,       scprobe,  scboot,  scintr};

/*
 * scprobe - probe an SSM for a console device.
 *
 * Look at the flags returned by the powerup firmware.
 */
scprobe(probe)
	struct ssm_probe *probe;
{
	if ((probe->sp_unit != CCB_LOCAL) && (probe->sp_unit != CCB_REMOTE))
		return(0);
	if (((probe->sp_unit == CCB_LOCAL)
	     && (probe->sp_desc->ssm_diag_flags & CFG_SSM_LOCAL))
        || ((probe->sp_unit == CCB_REMOTE)
	     && (probe->sp_desc->ssm_diag_flags & CFG_SSM_REMOTE)))
		return(0);
	else
		return(1);
}

/*
 * scboot - allocate data structures, etc at beginning of time.
 *
 * Called by autoconfig with an array of configured devices and their
 * number.  We allocate the necessary structures and fill them
 * with the various fields from the devs[] array.   
 */
scboot(ndevs, devp)
	int ndevs;
	register struct ssm_dev	*devp;
{
	struct sc_info	*infop;
	struct tty *tp;
	int dev, num_cons = 0;

	sc_base_vector = devp->sdv_sw_intvec;

	/* Allocate an array of pointer to info structures */
	sc_info = (struct sc_info **)calloc(ndevs * sizeof(struct sc_info *));
	/* If no memory is available, report error */
	if (!sc_info) {
		CPRINTF("sc: no memory for state info pointers\n");
		/*
		 *+ An attempt to allocate memory for the driver failed.
		 */
		return;
	}

	/* Set up info structure for each SSM device in system */
	for (dev = 0;  dev < ndevs; dev++, devp++){
		if (!devp->sdv_alive)
			continue;

	/* If this SSM is not the console, don't boot its console ports */
		if (!devp->sdv_desc->ssm_is_cons)
			continue;
		
		sc_info[dev] = infop = (struct sc_info *)
				         ssm_alloc(sizeof(struct sc_info),
					         SSM_ALIGN_BASE, SSM_BAD_BOUND);
		
		/* If no memory for info struct, report error */
		if (!infop) {     
			CPRINTF("sc: no memory for info structure\n");
			/*
			 *+ An attempt to allocate memory for the driver failed.
			 */
			return;
		}
		infop->sc_tty = tp = (struct tty *)calloc(sizeof(struct tty));
		
		/* If no memory for tty structure, report error */
		if (!tp) {
			CPRINTF("sc: no memory for tty structure\n");
			/*
			 *+ An attempt to allocate memory for the driver failed.
			 */
			return;
		}
		infop->sc_minor = dev;
		infop->sc_dev = devp;
		infop->parity_errs = 0;
		infop->frame_errs = 0;
		infop->overruns = 0;
		infop->devno = devp->sdv_unit;
		infop->os_busy_flag = 0;
		infop->sc_iflow = scflow;
		infop->sc_oflow = scflow;
		
		/* 
 		 * If this port is on the front panel SSM save away 
		 * a pointer to its info structure for future 
		 * reference when we open /dev/console, which follows 
		 * the SSM firmware monitor.
		 */
		if (devp->sdv_desc->ssm_is_cons) {
			if (devp->sdv_unit == CCB_LOCAL) 
				sc_cons.local = infop; 
			else 
				sc_cons.remote = infop;
		}
	
		init_ssm_cons_dev(devp->sdv_desc->ssm_slicaddr,
				  devp->sdv_desc->ssm_cons_cbs, devp->sdv_bin,
				  devp->sdv_sw_intvec, devp->sdv_unit);
		
		/* The next 3 lines of code represent this call to ttyinit:
			  ttyinit(infop->sc_tty, sc_gate); 
		   This line of code was expanded so that the macro definitions of
		   init_lock and init_sema would be picked up for i386
		   machines which do not use sc_gate. */
		init_lock(&tp->t_ttylock, sc_gate);
		init_sema(&tp->t_rawqwait, 0, 0, sc_gate);
		init_sema(&tp->t_outqwait, 0, 0, sc_gate);

		init_sema(&infop->os_busy_wait, 0, 0, sc_gate);
		num_cons++;
	}
	scmaxdev = --ndevs;
	/*
	 * scboot() may be called even if no SSMs are in the system.
	 * So, num_cons will show exactly how many live SSM console ports
	 * are present.  If none are alive, don't check for sc_cons info
	 * to be setup.
	 */
	if (num_cons) {
		if (!sc_cons.local) 
			CPRINTF("sc: Warning!  Local port for /dev/console not configured\n");
			/*
		 	 *+ Local SSM console port was not found after all devices
		   	 *+ were scanned.
		 	 */
		if (!sc_cons.remote) 
			CPRINTF("sc: Warning!  Remote port for /dev/console not configured\n");
			/*
		 	 *+ Remote SSM console port was not found after all devices
		 	 *+ were scanned.
		 	 */
	}

	/*
	 * Initialize synchronization mechanisms for
	 * access to the special device /dev/console.
	 */
	init_lock(&sc_cons.sync, sc_gate);
	init_sema(&sc_cons.wait, 0, 0, sc_gate);
}

/*
 * sc_open_console - locate /dev/console and open it.
 */
/*ARGSUSED*/
static
sc_open_console(dev, flag)
	dev_t	dev;
	int flag;
{
	spl_t s, tplock;
	int retval;
	register struct sc_info *old_ptr, *infop;
	
	ASSERT(minor(dev) == CTLR_MINOR, "sc_open_console: unexpected minor #");
	/*
	 *+ The minor device for /dev/console is incorrect.  This should
	 *+ never happen since this function is only called if the minor is
	 *+ correct.
	 */

	/* Single stream access through this function */
	s = p_lock(&sc_cons.sync, SPLTTY);
	while (sc_cons.busy) {
		p_sema_v_lock(&sc_cons.wait, TTIPRI, &sc_cons.sync, s);
		s = p_lock(&sc_cons.sync, SPLTTY);
	}
	sc_cons.busy = 1;
	v_lock(&sc_cons.sync, s);

	/* 
	 * Locate the console based on front panel settings.
	 */
	if ((infop = (ssm_get_fpst(SM_LOCK) & FPST_LOCAL) 
		     ? sc_cons.local : sc_cons.remote) == NULL) {
		CPRINTF("sc: Error - console not configured\n");
		/*
		 *+ No console port was found at boot time.
		 */
		sc_cons.busy = 0;
		vall_sema(&sc_cons.wait);
		return(ENXIO);
	}

	/* Save the original location of console */
	old_ptr = sc_cons.cur;

	if (sc_cons.nopen == 0 || sc_cons.cur != infop) {  
		/* Want to use a new channel; open it. */
		if (setjmp(&u.u_qsave)) {
			/* Recover from signal */
			sc_cons.busy = 0;
			vall_sema(&sc_cons.wait);
			return(EIO);
		}
		retval = scopen(makedev(major(dev), infop->sc_minor), flag);
		if (retval != 0) { 
			/* open failed - backout and return error */
			sc_cons.busy = 0;
			vall_sema(&sc_cons.wait);
			return(retval);
		}
		/*
		 * If /dev/console was previously open, save
		 * its former process group leader in the new
		 * tty structure.  This fixes stuff for tty.c
		 */
		if (sc_cons.nopen != 0)
			infop->sc_tty->t_pgrp = old_ptr->sc_tty->t_pgrp;
	} else {
		/*
		 * Make sure we call l_open().  Since we aren't going to
		 * call sc_open(), this needs to be done.
		 */
		tplock = p_lock(&infop->sc_tty->t_ttylock, SPLTTY);
		retval = (*linesw[infop->sc_tty->t_line].l_open)
			(makedev(major(dev), infop->sc_minor), infop->sc_tty);
		v_lock(&infop->sc_tty->t_ttylock, tplock);
	}

	/* Note the location of console and that its open */
	sc_cons.cur = infop;
	sc_cons.nopen++;

	/* Close the former port if it has changed */
	if (old_ptr && old_ptr != sc_cons.cur)
		scclose(makedev(major(dev), old_ptr->sc_minor), flag);

	/* Wakeup any waiting processes */
	sc_cons.busy = 0;
	vall_sema(&sc_cons.wait);

	ASSERT(infop != NULL, "scopen: /dev/console not established");
	/*
	 *+ /dev/console was not set up even though the SSM is the system
	 *+ controller.
	 */
	ASSERT(sc_cons.nopen > 0, "scopen: /dev/console open count error");
	/*
	 *+ Only one process should have opened /dev/console at this point.
	 */
	return(0);
}

/*
 * scopen - open an SSM console port
 */
/*ARGSUSED*/
scopen(dev, flag)
	register dev_t	dev;
	int flag;
{
	register struct cons_rcb *rptr;
	spl_t tplock, s;
	int retval;
	int unit = minor(dev);
	register struct sc_info *ip;
	register struct tty *tp;
	
	if (unit == CTLR_MINOR)
		/* A special function handles /dev/console */
		return(sc_open_console(dev, flag));

	if (unit > scmaxdev || !(ip = sc_info[unit]) || !ip->sc_dev->sdv_alive)
		return(ENXIO); 	/* non-existence device. */

	tp = ip->sc_tty;

	/*
	 * Lock the device.
	 */
	tplock = p_lock(&tp->t_ttylock, SPLTTY);

	tp->t_nopen++;
	while (tp->t_state & TS_LCLOSE) {	/* In process close so hang! */
		tp->t_state |= TS_WOPEN;
		p_sema_v_lock(&tp->t_rawqwait, TTIPRI, &tp->t_ttylock, tplock);
		tplock = p_lock(&tp->t_ttylock, SPLTTY);
	}

	/*
	 *  If the device is not already open, initialize it.
	 */
	if((tp->t_state & TS_ISOPEN) == 0) {
		tp->t_oproc = scstart;
		tp->t_addr = (caddr_t)ip;
		tp->t_ospeed = tp->t_ispeed = scspeed;
		ip->sc_modem = CCM_RTS | CCM_DTR;
		ip->sc_iflow = scflow;
		ip->sc_oflow = scflow;
		tp->t_flags = scflags;
		ttychars(tp);			/* setup default signal chars */
		/*
		 * Send a message to the SSM to establish line
		 * settings and baud rate of this console port.
		 */
		sc_param(ip);
		/*
		 * Send a SLIC interrupt to the SSM so that the 
		 * SSM's rx buffer will be flushed.  Wait for the flush
		 * to complete.
		 */
		rptr = (struct cons_rcb *)
			CONS_BASE_CB(ip->sc_dev->sdv_desc->ssm_cons_cbs, 
			             ip->devno) + CCB_RECV_CB;
		rptr->rcb_status = CCB_BUSY;
		s = splhi();
		mIntr(ip->sc_dev->sdv_desc->ssm_slicaddr, CONS_BIN,
		      (u_char) CONS_FLUSH(ip->sc_dev->sdv_unit, CCB_RECV_CB));
		splx(s);
		while (rptr->rcb_status == CCB_BUSY) ;
		/*
		 * Start a read operation on this SSM console port.
		 */
		ip->restart_read = 1;
		sc_start_read(ip);
	} else {
		/*
		 * Someone has an exclusive open on the device.
		 * Return error EBUSY
		 */
		if((tp->t_state & TS_XCLUDE) && u.u_uid != 0) {
			tp->t_nopen--;
			v_lock(&tp->t_ttylock, tplock);
			return(EBUSY);
		}
	}

	/*
	 * If console carrier detect is off, wait for it to come on. 
	 */
 	sc_get_modes(ip);	
	if (ip->sc_modem & CCM_DCD)
		tp->t_state |= TS_CARR_ON;
	while ((tp->t_state & TS_CARR_ON) == 0) {
		tp->t_state |= TS_WOPEN;
		p_sema_v_lock(&tp->t_rawqwait, PZERO+1, &tp->t_ttylock, tplock);
		tplock = p_lock(&tp->t_ttylock, SPLTTY);
	}
	retval = (*linesw[tp->t_line].l_open)(dev, tp);
	v_lock(&tp->t_ttylock, tplock);
	return (retval);
}

/*
 * scclose - close an SSM console port
 *
 * If the TS_HUPCLS flag is set in the TTY struct,
 * the RTS and DTR lines are turned off after the SSM tx buffer is 
 * flushed. Note: ttyclose makes tp->t_state = 0;
 * The driver will get called on every close and must keep track
 * of when last close happens to know when to call linesw close.
 */
/*ARGSUSED*/
scclose(dev, flag)
	dev_t dev;
	int flag;
{
	register struct cons_rcb *rptr;
	register unit = minor(dev);
	register struct sc_info *ip;
	register struct tty *tp;
	spl_t	tplock, s;

	if (unit == CTLR_MINOR) {
		/* 
		 * Treat /dev/console special since it can
		 * be attached to either SSM rs232 port.
		 */
		ip = sc_cons.cur;
		ASSERT(ip != NULL, "scclose: /dev/console not established");
		/*
		 *+ A close of /dev/console cannot continue since the info
		 *+ structure for /dev/console does not exist.
		 */

		/* Single stream access through this code */
		s = p_lock(&sc_cons.sync, SPLTTY);
		while (sc_cons.busy) {
			p_sema_v_lock(&sc_cons.wait, TTIPRI, &sc_cons.sync, s);
			s = p_lock(&sc_cons.sync, SPLTTY);
		}
		sc_cons.busy = 1;
		v_lock(&sc_cons.sync, s);

		/* Close /dev/console */
		if (sc_cons.nopen > 0 && --sc_cons.nopen == 0) {
			if (setjmp(&u.u_qsave)) {
				/* Recover from signal */
				sc_cons.busy = 0;
				vall_sema(&sc_cons.wait);
				return;
			}
			scclose(makedev(major(dev), sc_cons.cur->sc_minor), 
				flag);
		} 
		/* Wakeup any waiting processes */
		sc_cons.busy = 0;
		vall_sema(&sc_cons.wait);
		return;
	} 

	/* Close a standard SSM rs232 port */
	ip = sc_info[unit];
	
	tp = ip->sc_tty;
	tplock = p_lock(&tp->t_ttylock, SPLTTY);
	if(tp->t_nopen == 0 || --tp->t_nopen > 0 || (tp->t_state & TS_LCLOSE)){
		v_lock(&tp->t_ttylock, tplock);
		return;
	}
	tp->t_state |= TS_LCLOSE;
	(*linesw[tp->t_line].l_close)(tp);
	if ((tp->t_state & TS_ISOPEN) == 0 || (tp->t_state & TS_HUPCLS)) {
		/*
		 * Turn h/w off on hupcls on last close (new line disc).
		 * Assumes all output flushed before here.
		 */
		ip->sc_modem &= ~(CCM_DTR | CCM_RTS | CCM_BREAK);
		sc_set_modes(ip);
	}

	ip->restart_read = 0;
	/*
	 * Send a SLIC interrupt to the SSM so that the SSM will flush
	 * its RX port.  Wait for the flush to complete.
	 */
	rptr = (struct cons_rcb *)
		CONS_BASE_CB(ip->sc_dev->sdv_desc->ssm_cons_cbs, ip->devno)
		             + CCB_RECV_CB;
	rptr->rcb_status = CCB_BUSY;
	s = splhi();
	mIntr(ip->sc_dev->sdv_desc->ssm_slicaddr, CONS_BIN,
	      (u_char) CONS_FLUSH(ip->sc_dev->sdv_unit, CCB_RECV_CB));
	splx(s);
	while (rptr->rcb_status == CCB_BUSY)
		;
	ttyclose(tp);
	v_lock(&tp->t_ttylock, tplock);
}

/*
 * scread -  standard console read.
 *
 * Mutual exclusion: called procedures assume locked tp.
 */
scread(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{	
	int retval;
	int unit = minor(dev);
	spl_t	tplock, s;
	register struct tty *tp;
	
	if (unit == CTLR_MINOR) {
		/* 
		 * The tty-driver should take care of a close 
		 * occuring while we are working with this tty.
		 * Single streaming here could cause other such
		 * code to wait indefinitely for data to be input.
		 */
		s = p_lock(&sc_cons.sync, SPLTTY);
		tp = sc_cons.cur->sc_tty;
		v_lock(&sc_cons.sync, s);
	} else
		tp = sc_info[unit]->sc_tty;

	tplock = p_lock(&tp->t_ttylock, SPLTTY);
	retval = (*linesw[tp->t_line].l_read)(tp, uio);
	v_lock(&tp->t_ttylock, tplock);
	return (retval);
}

/*
 * scwrite - standard console write.
 *
 * Mutual exclusion: called procedures assume locked tp.
 */
scwrite(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	int retval;
	int unit = minor(dev);
	spl_t tplock, s;
	register struct tty *tp;

	if (unit == CTLR_MINOR) {
		/* 
		 * The tty-driver should take care of a close 
		 * occuring while we are working with this tty.
		 * Single streaming here could cause other such
		 * code to wait indefinitely while to write data.
		 */
		s = p_lock(&sc_cons.sync, SPLTTY);
		tp = sc_cons.cur->sc_tty;
		v_lock(&sc_cons.sync, s);
	} else
		tp = sc_info[unit]->sc_tty;

	tplock = p_lock(&tp->t_ttylock, SPLTTY);
	retval = (*linesw[tp->t_line].l_write)(tp, uio);
	v_lock(&tp->t_ttylock, tplock);
	return (retval);
}

/*
 * scintr - SSM interrupt service routine
 */
scintr(vector)
	int vector;
{
	register int count;
	register caddr_t bptr;
	struct cons_rcb *rptr;
	struct cons_xcb *xptr;
	int unit = (vector -= sc_base_vector) >> CCB_INTRSHFT;
	int func = vector & 1;
	struct sc_info *ip;
	register struct tty *tp;
	spl_t tplock;

	if (unit < 0 || unit > scmaxdev) {
		CPRINTF("sc%d: stray interrupt, unit out of range.\n",unit);
		/*
		 *+ An interrupt was received from an SSM console port that
		 *+ is out of the range of the known SSM console ports.
		 */
		return;
	}
	if (!(ip = sc_info[unit]) || !ip->sc_dev->sdv_alive) {
		CPRINTF("sc%d: stray interrupt, dead unit.\n", unit);
		/*
		 *+ An interrupt was received from an SSM console port that
		 *+ is considered to be non-functional.
		 */
		return;
	}
	tp = ip->sc_tty;
	tplock = p_lock(&tp->t_ttylock, SPLTTY);
	
	/*
	 * Handle read-CB-done interrupts.
	 */
	if (func == CCB_RECV_CB) {
		rptr = (struct cons_rcb *)
			CONS_BASE_CB(ip->sc_dev->sdv_desc->ssm_cons_cbs,     
				     ip->devno) + CCB_RECV_CB;
		
		switch (rptr->rcb_status) {
		case CCB_BUSY:
			/* If the CB is still busy, ignore this */
			v_lock(&tp->t_ttylock, tplock);
			return;

		case CCB_MS_CHG:
			/*
		 	* If this port's DCD* line changed state, and the tty code
		 	*  did not know about it, change the state bits and do 
		 	*  some signalling.
		 	*/
			sc_get_modes(ip);
			if (!(ip->sc_modem & CCM_DCD)) {
				if (tp->t_state & TS_CARR_ON) {
					tp->t_state &= ~TS_CARR_ON;
					if (((tp->t_state & TS_WOPEN) == 0)
					&& (tp->t_state & TS_ISOPEN)
					&& ((tp->t_flags & NOHANG) == 0)) {
						gsignal(tp->t_pgrp, SIGHUP);
						gsignal(tp->t_pgrp, SIGCONT);
						ttyflush(tp, FREAD|FWRITE);
					}
				}
			} else if ((tp->t_state & TS_CARR_ON) == 0) {
					tp->t_state |= TS_CARR_ON;
					vall_sema(&tp->t_rawqwait);
				}
			break;
		
		case CCB_PERR:
			ip->parity_errs++;
			break;	
		
		case CCB_OVERR:
			ip->overruns++;
			break;	
		
		case CCB_FERR:
			ip->frame_errs++;
			break;	
		
		case CCB_BREAK:
			if (tp->t_flags & RAW)
				(*linesw[tp->t_line].l_rint)(0, tp); 
			else
				(*linesw[tp->t_line].l_rint)(tp->t_intrc, tp); 
	 		break;	
		
		case CCB_TIMEO:
		case CCB_OK:	
	                break;	
		}
		
		count = CBSIZE - rptr->rcb_count;
		/* Send the received chars to the tty-managed buffers */
		bptr = ip->rx_buf;
		for (; count > 0; bptr++, count--)
			(*linesw[tp->t_line].l_rint)(*bptr, tp);

		if (ip->restart_read)
			sc_start_read(ip);

	} else {  
		/* Handle completed write operations */
		xptr = (struct cons_xcb *) 
			CONS_BASE_CB(ip->sc_dev->sdv_desc->ssm_cons_cbs,
				     ip->devno) + CCB_XMIT_CB;
		
		/* If CB still busy, ignore spurious interrupt */	
		if (xptr->xcb_status == CCB_BUSY) {
			v_lock(&tp->t_ttylock, tplock);
			return;
		}
		/* # chars sent = # chars requested to be sent - # remaining */
		count = ip->tx_count - xptr->xcb_count;

		tp->t_state &= ~TS_BUSY;
		if (tp->t_state & TS_FLUSH)
			tp->t_state &= ~TS_FLUSH;
		else
			ndflush(&tp->t_outq, count);
		if (tp->t_line)
			(*linesw[tp->t_line].l_start)(tp);
		else
			scstart(tp);
	}
	v_lock(&tp->t_ttylock, tplock);
}

/*
 * scioctl - io controls change service
 *
 * Mutex: assumes called with a non-locked tp.
 */
scioctl(dev, cmd, data, flag)
	dev_t dev;
	register cmd;
	int flag;
	caddr_t	data;
{	
	register error;
	int unit = minor(dev);
	struct sc_info *ip;
	spl_t tplock, s;
	struct tty *tp;

	if (unit == CTLR_MINOR) {
		/* Single stream access through this code */
		s = p_lock(&sc_cons.sync, SPLTTY);
		while (sc_cons.busy) {
			p_sema_v_lock(&sc_cons.wait, TTIPRI, &sc_cons.sync, s);
			s = p_lock(&sc_cons.sync, SPLTTY);
		}
		sc_cons.busy = 1;
		v_lock(&sc_cons.sync, s);
		ip = sc_cons.cur;
		if (setjmp(&u.u_qsave)) {
			/* Recover from signal */
			sc_cons.busy = 0;
			vall_sema(&sc_cons.wait);
			return(EIO);
		}
	} else
		ip = sc_info[unit];
	tp = ip->sc_tty;

	tplock = p_lock(&tp->t_ttylock, SPLTTY);
	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
	if (error >= 0) {
		v_lock(&tp->t_ttylock, tplock);
		if (unit == CTLR_MINOR) {
			sc_cons.busy = 0;
			vall_sema(&sc_cons.wait);
		}
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
			sc_param(ip);
		}
		v_lock(&tp->t_ttylock, tplock);
		if (unit == CTLR_MINOR) {
			sc_cons.busy = 0;
			vall_sema(&sc_cons.wait);
		}
		return (error);
	}

	/*
	 * Process special stuff...
	 */
	error = 0;
	switch(cmd) {
	case TIOCSBRK:	/* set break on */
		ip->sc_modem |= CCM_BREAK;
		sc_set_modes(ip);
		break;
	case TIOCCBRK:	/* clear break off */
		ip->sc_modem &= ~CCM_BREAK;
		sc_set_modes(ip);
		break;
	case TIOCSDTR:	/* Turn on dtr rts */
		ip->sc_modem |= CCM_DTR | CCM_RTS;
		sc_set_modes(ip);
		break;
	case TIOCCDTR:	/* turn off dtr rts */
		ip->sc_modem &= ~(CCM_DTR | CCM_RTS);
		sc_set_modes(ip);
		break;
	default:
		error = ENOTTY;
	}
	v_lock(&tp->t_ttylock, tplock);
	if (unit == CTLR_MINOR) {
		sc_cons.busy = 0;
		vall_sema(&sc_cons.wait);
	}
	return (error);
}

/*
 * scselect - check for pending I/O
 *
 * Mutual exclusion: called procedures assume locked tp.
 */
scselect(dev, rw)
	dev_t dev;
	int rw;
{
	int retval;
	label_t oqsave;
	spl_t tplock, s;
	int unit = minor(dev);
	register struct tty *tp;

	if (unit == CTLR_MINOR) {
		/* Single stream access through this code */
		s = p_lock(&sc_cons.sync, SPLTTY);
		while (sc_cons.busy) {
			p_sema_v_lock(&sc_cons.wait, TTIPRI, &sc_cons.sync, s);
			s = p_lock(&sc_cons.sync, SPLTTY);
		}
		sc_cons.busy = 1;
		v_lock(&sc_cons.sync, s);
		tp = sc_cons.cur->sc_tty;
		oqsave = u.u_qsave;
		if (setjmp(&u.u_qsave)) {
			/* Recover from signal */
			sc_cons.busy = 0;
			u.u_qsave = oqsave;
			vall_sema(&sc_cons.wait);
			return(0);
		}
	} else
		tp = sc_info[unit]->sc_tty;

	tplock = p_lock(&tp->t_ttylock, SPLTTY);
	retval = (*linesw[tp->t_line].l_select)(tp, rw);
	v_lock(&tp->t_ttylock, tplock);
	if (unit == CTLR_MINOR) {
		sc_cons.busy = 0;
		vall_sema(&sc_cons.wait);
		u.u_qsave = oqsave;
	}
	return (retval);
}

/*
 * sc_param - set line parameters on the SSM console hardware.
 *
 * Mutex: assumed called with locked tp to insure stable data
 * being read.
 */
sc_param(ip)
	register struct sc_info *ip;
{
	register struct tty *tp = ip->sc_tty;
	u_char parflag;

	if(tp->t_ispeed == B0) {
		tp->t_state |= TS_HUPCLS;
		ip->sc_modem &= ~(CCM_RTS | CCM_BREAK | CCM_DTR);
		sc_set_modes(ip);
		return;
	} else {

		/* Clear parity/char size bits to avoid confusion */
		ip->sc_parsiz &= ~CCP_CSMASK;
		ip->sc_parsiz &= ~CCP_PMASK;
		ip->sc_parsiz &= ~CCP_STMASK;

		if (tp->t_flags & (RAW|LITOUT|PASS8)) {
			ip->sc_parsiz |= CCP_CSIZ8;
			parflag = 0;
		} else {
		 	ip->sc_parsiz |= CCP_CSIZ7;
		 	parflag = 1;
		}
		if (parflag)
			if ((tp->t_flags & EVENP) == 0)
				ip->sc_parsiz |= CCP_ODDP;
			else 
				ip->sc_parsiz |= CCP_EVENP;
		if (tp->t_ospeed == B110)
			ip->sc_parsiz |= CCP_ST2;
		else
			ip->sc_parsiz |= CCP_ST1;
	}

	/* Send the new console parameters to the SSM */
	sc_set_modes(ip);
}

/*
 * scstart - Send characters to the console output.
 *
 * This procedure starts a character out by calculating the maximum
 * number of characters it can get from the clist and then programming
 * the SSM to send that many characters from the clist.
 *
 * Assumes: caller locks the tp before calling scstart().
 */
scstart(tp)
	register struct tty *tp;
{
	register int nch;
	u_char *addr;
	register struct sc_info *ip;

	/*
	 * If the SSM console is already working, or if it is waiting on
	 * a delay, then nothing is done right now.
	 */
	if(tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP)) 
		return;

	/*
	 * If anyone is sleeping, waiting for the port activity
	 * to stop so that a baud rate, line status, etc. change
	 * can be made, wake them up now before io is started.
	 */
	ip = (struct sc_info *)tp->t_addr;
	if (ip->os_busy_flag) {
		ip->os_busy_flag = 0;
		vall_sema(&ip->os_busy_wait);
		return;
	}

	/*
	 * If the output queue has emptied to the low threshold, and
	 * if anyone is sleeping on this queue, wake them up.
	 */
	if(tp->t_outq.c_cc <= TTLOWAT(tp)) {
		if(tp->t_state & TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			vall_sema(&tp->t_outqwait);
		}
		if (tp->t_wsel) {
			selwakeup(tp->t_wsel, tp->t_state & TS_WCOLL);
			tp->t_state &= ~TS_WCOLL;
			tp->t_wsel = (struct proc *)NULL;
		}
	}
	if (tp->t_outq.c_cc == 0)   	/* Nothing to process */
		return;

	/*
	 * DMA directly from the CLIST. When the tx-done interrupt is
	 * received, we must call ndflush() to flush the clist. 
	 */
	if (tp->t_flags & (RAW|LITOUT))
		nch = ndqb(&tp->t_outq, 0);
	else {	
		/*
		 * not raw so check for timeout chars and if we 
		 * have dma'd out to one then do the timeout.
		 */
		nch = ndqb(&tp->t_outq, 0200);
		if(nch == 0) {
			nch = getc(&tp->t_outq);
			timeout(ttrstrt, (caddr_t)tp, ((nch & 0x7f) + 6));
			tp->t_state |= TS_TIMEOUT;
			return;
		}
	}
	addr = (u_char *)tp->t_outq.c_cf;
	ASSERT((nch > 0) && (nch <= CBSIZE), "scstart:Bad dma count");
	/*
	 *+ The driver is being asked to send more than a cblock or less than
	 *+ zero characters to the output port.
	 */
	sc_start_write((struct sc_info *)tp->t_addr, nch, addr);
	tp->t_state |= TS_BUSY;
}

/*
 * scstop - Abort an in progress dma on a channel.
 *
 * This procedure aborts output on a line.
 * Curious note:  No driver seems to ever use the flag parameter.
 *
 * Mutex: Assumes caller has locked the tp prior to
 *	  calling this procedure.
 */
/*ARGSUSED*/
scstop(tp, flag)
	register struct tty *tp;
	int flag;
{
	register struct cons_xcb *xptr;
	struct sc_info *ip;
	spl_t s;

	if (tp->t_state & TS_BUSY) {
		/*
		 * Device is transmitting; stop output.
		 * We will clean up later by examining the xfer 
		 * count for a completion.
		 *
		 * The TS_FLUSH flag is used to tell us HOW to clean
		 * up.  If it is set, the characters have 
		 * aleady been cleaned off the t_outq by ttyflush().
		 * Otherwise, we need to clean off the ones that have
		 * been transmitted.
		 */
		if ((tp->t_state & TS_TTSTOP)==0)
			tp->t_state |= TS_FLUSH;

		/*
		 * Send a SLIC interrupt to the SSM so it will abort 
		 * the current TX operation, if any.
		 */
		ip = (struct sc_info *)tp->t_addr;
		xptr = (struct cons_xcb *)
			CONS_BASE_CB(ip->sc_dev->sdv_desc->ssm_cons_cbs,
				     ip->devno) + CCB_XMIT_CB;
		xptr->xcb_status = CCB_BUSY;
		s = splhi();
		mIntr(ip->sc_dev->sdv_desc->ssm_slicaddr, CONS_BIN,
		      (u_char) CONS_FLUSH(ip->sc_dev->sdv_unit, CCB_XMIT_CB));
		splx(s);
		while (xptr->xcb_status == CCB_BUSY)
			;
		tp->t_state &= ~TS_BUSY;
	}
}

/*
 * sc_ssm_send - send an SSM SLIC interrupt 
 */
sc_ssm_send(ip, func)
	struct sc_info *ip;
	int func;
{
	spl_t s = splhi();
	mIntr(ip->sc_dev->sdv_desc->ssm_slicaddr, CONS_BIN,
	      (u_char)COVEC(ip->sc_dev->sdv_unit, func));
	splx(s);
}

/*
 * sc_start_read - Start an SSM read request and do not wait for the
 *                  request to complete.
 */
sc_start_read(ip)
	struct sc_info	*ip; 
{
	struct cons_rcb *rptr;
	
	rptr = (struct cons_rcb *)
		CONS_BASE_CB(ip->sc_dev->sdv_desc->ssm_cons_cbs,
			     ip->devno) + CCB_RECV_CB;
	rptr->rcb_cmd = CCB_RECV | CCB_IENABLE | CCB_TERM_MS;
	rptr->rcb_status = CCB_BUSY;
	rptr->rcb_count = CBSIZE;
	rptr->rcb_addr = (u_long)ip->rx_buf;
	rptr->rcb_timeo = scrxtime;
	sc_ssm_send(ip, CCB_RECV_CB);
}

/*
 * sc_start_write - Start an SSM write request and do not wait for the
 *                   request to complete.
 */
sc_start_write(ip, nch, addr)
	struct sc_info *ip;
	register int nch;
	register u_char *addr;
{
	register struct cons_xcb *xptr;
	xptr = (struct cons_xcb *)
		CONS_BASE_CB(ip->sc_dev->sdv_desc->ssm_cons_cbs,
			     ip->devno) + CCB_XMIT_CB;
	xptr->xcb_cmd = CCB_XMIT | CCB_IENABLE | CCB_TERM_MS;
	xptr->xcb_status = CCB_BUSY;
	xptr->xcb_addr = addr;
	xptr->xcb_count = ip->tx_count = (u_short)nch;
	sc_ssm_send(ip, CCB_XMIT_CB);
}

/*
 * sc_set_modes - Set console serial port settings.      
 *
 */
sc_set_modes(ip)
	struct sc_info *ip;
{
	struct tty *tp = ip->sc_tty;
	struct cons_mcb *mptr;
	
	/*
	 * If io on this port is active, sleep until no
	 * io in progress then do mode change.
	 */
	while (tp->t_state & TS_BUSY) {
		ip->os_busy_flag = 1;
		p_sema_v_lock(&ip->os_busy_wait, TTIPRI, 
			      &tp->t_ttylock, SPLTTY);
		(void) p_lock(&tp->t_ttylock, SPLTTY);
	}
	mptr = (struct cons_mcb *)
		CONS_BASE_CB(ip->sc_dev->sdv_desc->ssm_cons_cbs,
			     ip->devno) + CCB_MSG_CB;
	mptr->mcb_cmd = CCB_STTY;
	mptr->mcb_status = CCB_BUSY;
	mptr->mcb_modem = ip->sc_modem;

	if (tp->t_ospeed != B0)
		mptr->mcb_baud = tp->t_ospeed;

	mptr->mcb_parsiz = ip->sc_parsiz; 
	mptr->mcb_iflow = ip->sc_iflow;
	mptr->mcb_oflow = ip->sc_oflow;
	mptr->mcb_oxoff = tp->t_stopc;
	mptr->mcb_oxon = tp->t_startc;
	sc_ssm_send(ip, CCB_MSG_CB);
	while (mptr->mcb_status == CCB_BUSY)
		;
	scstart(tp);			/* Get output going again */
}

/*
 * sc_get_modes - Get current console serial port settings.
 */
sc_get_modes(ip)
	struct sc_info *ip;
{
	struct cons_mcb *mptr;
	struct tty *tp = ip->sc_tty;

	mptr = (struct cons_mcb *)
		CONS_BASE_CB(ip->sc_dev->sdv_desc->ssm_cons_cbs,
			     ip->devno) + CCB_MSG_CB;
	mptr->mcb_cmd = CCB_GTTY;
	mptr->mcb_status = CCB_BUSY;
	sc_ssm_send(ip, CCB_MSG_CB);
	while (mptr->mcb_status == CCB_BUSY)
		;
	ip->sc_modem = mptr->mcb_modem;
	tp->t_ispeed = tp->t_ospeed = mptr->mcb_baud;
	ip->sc_parsiz = mptr->mcb_parsiz;
	ip->sc_iflow = mptr->mcb_iflow;
	ip->sc_oflow = mptr->mcb_oflow;
}

/*
 * init_ssm_cons_dev - initialize SSM console port 
 *
 * Initializes an SSM console port by sending a CCB_INIT 
 * message to the SSM.
 * Assumes that mIntr() retries messages until they succeed.
 */
void
init_ssm_cons_dev(slic, cons_cbs, bin, basevec, unit)
	u_char	slic;
	struct cons_cb 	*cons_cbs;
	u_char bin, basevec;
	short 	unit;
{
	struct cons_icb *iptr;
	
	iptr = (struct cons_icb *) (CONS_BASE_CB(cons_cbs, unit) + CCB_MSG_CB);
	iptr->icb_cmd = CCB_INIT;
	iptr->icb_dest = SL_GROUP | TMPOS_GROUP;
	iptr->icb_basevec = basevec;
	iptr->icb_scmd = SL_MINTR | bin;
	iptr->icb_status = CCB_BUSY;

	mIntr(slic, CONS_BIN, (u_char)COVEC(unit, CCB_MSG_CB));
	while (iptr->icb_status == CCB_BUSY)
		;
}
