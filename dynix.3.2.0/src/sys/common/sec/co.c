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

#ifndef lint
static char rcsid[]= "$Header: co.c 2.15 90/12/13 $";
#endif

/*
 * SCSI/Ether console driver
 *
 * There are two consoles per SEC card.  The SEC attached to the
 * front panel is distinguished.  The port selected as console on this
 * sec can be accessed through minor device 64 (0x40).  All ports can
 * be accessed through minor device numbers 0 through 2*n, for n sec
 * boards.  The minor device number 64 is an alias for one of these
 * devices.
 */

/* $Log:	co.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/systm.h"
#include "../h/clist.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/proc.h"
#include "../h/cmn_err.h"

#include "../balance/cfg.h"

#include "../machine/ioconf.h"
#include "../machine/intctl.h"

#include "../sec/sec.h"
#include "../sec/co.h"

#define	CO_TTYLOCK(tp)	p_lock(&(tp)->t_ttylock, SPLTTY);
#define CO_TTYUNLOCK(tp,spl)	v_lock(&(tp)->t_ttylock, (spl));
#define	CO_FLAG	0x40

int coprobe(), coboot(), cointr();

struct sec_driver co_driver={
/*	name	base	flags		probe		boot	intr	*/
	"co",	3,	SED_TYPICAL,	coprobe,	coboot,	cointr
};

int 	co_max_unit = -1;
u_char 	co_base_vec;
struct 	co_state *co_state;
struct	co_state *co_costate;

extern int coflags;
extern char cospeed;
struct sec_dev_prog * CO_GET_Q_TAIL();
struct sec_dev_prog * CO_GET_Q_HEAD();


/*
 * coprobe - probe a SCSI/Ether for a console device.
 *
 * Look at the flags returned by the powerup firmware.
 */

coprobe(probe)
	struct sec_probe *probe;
{
	int failure_flag;

	switch(probe->secp_chan) {
	case SDEV_CONSOLE0IN:
	case SDEV_CONSOLE0OUT:
		failure_flag = CFG_PORT0;
		break;
	case SDEV_CONSOLE1IN:
	case SDEV_CONSOLE1OUT:
		failure_flag = CFG_PORT1;
		break;
	}

	if(probe->secp_desc->sec_diag_flags & failure_flag) {
#ifdef	DEBUG
		if(co_debug) printf("P-");
#endif	DEBUG
		return(0);
	}
#ifdef	DEBUG
	if(co_debug) printf("P+");
#endif	DEBUG
	return(1);
}

/*
 * coboot - allocate data structures, etc at beginning of time.
 *
 * Called by autoconfig with an array of configured devices and their
 * number.  We allocate the necessary soft descriptions and fill them
 * with the various fields from the devs[] array.  Due to the dual 
 * device-channel nature of SEC devices, each devs[] entry describes
 * input side or output side of a console device.
 */

coboot(ndevs, devs)
	struct sec_dev *devs;
{
	register int i;

	/* 
	 * find out the maximum potential unit number, and
	 * allocate array of soft states.
	 */

	for (i = 0; i < ndevs; ++i) {
		if (co_max_unit < 2*devs[i].sd_sec_idx + 1)
			co_max_unit = 2*devs[i].sd_sec_idx + 1;
	}

	co_state = (struct co_state *)calloc((co_max_unit + 1) *
			sizeof(struct co_state));
	
	co_base_vec = devs[0].sd_vector;


	/*
	 * now boot each configured device.
	 */

	for (i = 0; i < ndevs; ++i) {
		register struct sec_dev *devp;
		register int unit;

		devp = &devs[i];
		if (devp->sd_alive == 0)
			continue;

		unit = UNITMINOR(i);

		if (unit >= co_bin_config_count) {
			printf("%s%d: console device not binary configured.\n",
				co_driver.sed_name, unit);
			/*
                         *+ The console device is not configured.  Corrective
                         *+ action:  edit your conf file to include all
                         *+ SCED console devices.
                         */
			continue;
		}

		co_boot_one(&co_state[unit], devp, &co_bin_config[unit]);
	}
}

/*
 * co_boot_one - combine binary config, autoconfig information into
 *		 softstate, for convenient access.
 *
 * Allocate device program, etc.  The device program queues were allocated
 * by autoconfig.
 *
 * status pointers for cibs point to data in the soft state.
 *
 * No locking needs to be done here, as we are still running config
 * code single processor.
 */

co_boot_one(softp, sd, bc)
	register struct co_state *softp;
	register struct sec_dev *sd;
	register struct co_bin_config *bc;
{
	struct sec_cib *pu_cib;

#ifdef DEBUG
	if (co_debug > 2)
		printf("B%d.\n", sd->sd_unit);
#endif DEBUG

	if (!softp->ss_initted) {

		ttyinit(&softp->ss_tty, bc->cobc_gate);
		softp->ss_alive = 1;
		softp->ss_initted = 1;
		if (sd->sd_desc->sec_is_cons) {
			switch(sd->sd_desc->sec_is_cons) {

			case CDSC_LOCAL:
#if DEBUG
				printf("local console\n");
#endif DEBUG
				if (sd->sd_chan == SDEV_CONSOLE0IN ||
					sd->sd_chan == SDEV_CONSOLE0OUT)
					co_costate = softp;
				break;

			case CDSC_REMOTE:
#if DEBUG
				printf("remote console\n");
#endif DEBUG
				if (sd->sd_chan == SDEV_CONSOLE1IN ||
					sd->sd_chan == SDEV_CONSOLE1OUT)
					co_costate = softp;
				break;

			default:
				panic("console: invalid device as console \n");
				/*
				 *+ No legal console device can be found.
				 *+ Check the hardware configuration.
				 *+ (this panic is unlikly to be printed 
				 *+ anywhere).
				 */
				break;
			}
		}

	}

	if (sd->sd_chan == SDEV_CONSOLE0IN || sd->sd_chan == SDEV_CONSOLE1IN) {
		ASSERT(!softp->is_initted, "co_boot_one: bad dev");
                /*
                 *+  A SCED console input port has already been initialized.
                 *+  This port has been passed to co_boot_one to be
                 *+  initialized again.
                 */

		softp->is_sd = sd;
		softp->is_ovc = 0;
		softp->is_parc = 0;

		softp->is_reqq.sq_size = sd->sd_req_size;
		softp->is_reqq.sq_progq = sd->sd_requestq;

		softp->is_doneq.sq_size = sd->sd_doneq_size;
		softp->is_doneq.sq_progq = sd->sd_doneq;

		SEC_fill_progq(softp->is_reqq.sq_progq,
			       (int)softp->is_reqq.sq_size,
			       (int)sizeof(struct sec_dev_prog));

		softp->is_initted = 1;

	} else 
	if (sd->sd_chan==SDEV_CONSOLE0OUT || sd->sd_chan==SDEV_CONSOLE1OUT) {
		ASSERT(!softp->os_initted, "co_boot_one: output initted");
                /*
                 *+  A SCED console output port has already been initialized.
                 *+  This port has been passed to co_boot_one to be
                 *+  initialized again.
                 */

		softp->os_sd = sd;

		softp->os_reqq.sq_size = sd->sd_req_size;
		softp->os_reqq.sq_progq = sd->sd_requestq;

		softp->os_doneq.sq_size = sd->sd_doneq_size;
		softp->os_doneq.sq_progq = sd->sd_doneq;

		SEC_fill_progq(softp->os_reqq.sq_progq,
				(int)softp->os_reqq.sq_size,
				(int)sizeof(struct sec_dev_prog));

		softp->os_busy_flag = 0;
		init_sema(&softp->os_busy_wait, 0, 0, bc->cobc_gate);

		pu_cib = &softp->os_sd->sd_desc->sec_powerup->pu_cib;
#ifdef	DEBUG
		if (co_debug > 1){
			printf("coboot doing getmode\n");
		}
#endif	DEBUG
		SEC_startio(SINST_GETMODE, &softp->os_smode.sm_status, 
			sd->sd_bin,
			sd->sd_chan,
			pu_cib,
			sd->sd_desc->sec_slicaddr);
		if (softp->os_smode.sm_status & (~SINST_INSDONE))
			CPRINTF("co: bad get_mode: %x\n", softp->os_smode.sm_status);
#ifdef	DEBUG
		if (co_debug > 1)
			CPRINTF("coboot getmode done flags %x, baud %x\n",
			softp->os_smode.CM_FLAGS, softp->os_smode.CM_BAUD);
#endif	DEBUG

		softp->os_smode.CM_FLAGS &= ~(SCONS_DZMODE );
 
#ifdef	DEBUG
		if (co_debug > 1)
			CPRINTF("coboot doing setmode\n");
#endif	DEBUG
		SEC_startio(SINST_SETMODE, &softp->os_smode.sm_status, 
			sd->sd_bin,
			sd->sd_chan,
			pu_cib,
			sd->sd_desc->sec_slicaddr);
		if (softp->os_smode.sm_status & (~SINST_INSDONE))
		CPRINTF("co: bad set_mode: %x\n", softp->os_smode.sm_status);
#ifdef	DEBUG
		if (co_debug > 1)
			CPRINTF("coboot setmode done flags %x, baud %x\n",
			softp->os_smode.CM_FLAGS, softp->os_smode.CM_BAUD);
#endif	DEBUG

		softp->os_initted = 1;

	} else {
		CPRINTF("%s %d: invalid device chan %d(0x%x) in boot routine\n",
			co_driver.sed_name, sd->sd_sec_idx,
			sd->sd_chan, sd->sd_chan);
	}
}

int costart();

/*ARGSUSED*/
coopen(dev, flag)
	register dev_t	dev;
		 int	flag;
{
	register struct tty *tp;
	register struct co_state *softp;
	register unit;
	spl_t	tplock;
	int	retval;

#ifdef DEBUG
	if(co_debug)
		printf("O");
#endif DEBUG
	/*
	 * Check for errors
	 */

	unit = minor(dev);
	if (unit & CO_FLAG) {
		softp = co_costate;
		dev = makedev(major(dev), UNITMINOR(softp->is_sd->sd_unit));
	} else {
		softp = &co_state[unit];
		if(unit > co_max_unit || softp->ss_alive == 0)
			return(ENXIO);
	}

	/*
	 * Lock the device and initialize the channel
	 */
	
	tp = &softp->ss_tty;
	tplock = CO_TTYLOCK(tp);

	tp->t_nopen++;
	while(tp->t_state&TS_LCLOSE) {		/* In process close so hang! */
		tp->t_state |= TS_WOPEN;
		p_sema_v_lock(&tp->t_rawqwait, TTIPRI, &tp->t_ttylock, tplock);
		tplock = CO_TTYLOCK(tp);
	}

	if((tp->t_state & TS_ISOPEN) == 0) {
		tp->t_oproc = costart;
		tp->t_addr = (caddr_t)softp;
		if(tp->t_ispeed == 0) {
			if (softp == co_costate)
				tp->t_ospeed = tp->t_ispeed = 
						softp->os_smode.CM_BAUD;	
			else
				tp->t_ospeed = tp->t_ispeed = cospeed;
			tp->t_flags = coflags;
		}
		ttychars(tp);				/* setup default signal chars */
		/*
		 * over ride some of those defaults with those from
		 * the cfg structure.  Only for /dev/console
		 */
		if (softp == co_costate) {
			tp->t_erase = CD_LOC->c_erase;
			tp->t_kill = CD_LOC->c_kill;
			tp->t_intrc = CD_LOC->c_interrupt;
		}

		co_param(softp);	/* setup default baud rate */

		co_abort_read(softp);
		softp->is_restart_read = 1;
		co_start_read(softp);
	}else{
		/*
		 * Someone has an exclusive open on the device.
		 * Return error EBUSY
		 */
		if((tp->t_state & TS_XCLUDE) && u.u_uid != 0) {
			tp->t_nopen--;
			CO_TTYUNLOCK(tp, tplock);
			return(EBUSY);
		}
	}


	/*
	 * Set up modem status.
	 */
	softp->os_smode.CM_FLAGS &= ~(	SCONS_CLEAR_RTS |
						SCONS_CLEAR_DTR |
						SCONS_IGN_CARRIER);
	co_wset_modes(softp);

	/*
	 * Block waiting for carrier.
	 */
	co_get_modes(softp);
	if ((softp->os_smode.CM_FLAGS & SCONS_CARRIER_CLEAR)==0)
		tp->t_state |= TS_CARR_ON;
	if (flag & O_NDELAY)
		tp->t_state |= TS_CARR_ON;
	while((tp->t_state&TS_CARR_ON) == 0) {
		tp->t_state |= TS_WOPEN;
#ifdef	DEBUG
		if (co_debug > 1)
			printf("coopen: waiting for carrier\n");
#endif	DEBUG
		p_sema_v_lock(&tp->t_rawqwait, PZERO+1, &tp->t_ttylock, tplock);
		tplock = CO_TTYLOCK(tp);
	}
#ifdef	DEBUG
	if (co_debug > 1)
		printf("coopen: got carrier\n");
#endif	DEBUG
	

	retval = (*(linesw[tp->t_line].l_open))(dev, tp);
#ifdef	DEBUG
	if (co_debug > 1)
		printf("coopen: open finished\n");
#endif	DEBUG
	CO_TTYUNLOCK(tp, tplock);
	return(retval);
}

/*
 * To "close" an CO channel, we need to kill off the read that is
 * pending all the time.  If the TS_HUPCLS flag is set,
 * the RTS and DTR lines are turned off after all of the output queues are
 * flushed. Note: ttyclose makes tp->t_state = 0;
 *
 * NOTE: The driver will get called on every close and must keep track
 *	 of when last close happens to know when to call linesw close.
 */
/*ARGSUSED*/
coclose(dev, flag)
	dev_t dev;
	int flag;
{
	register struct tty *tp;
	register struct co_state *softp;
	register unit;
	spl_t	tplock;

#ifdef DEBUG
	if(co_debug>2)
		printf("co_close dev=%x\n", dev);
	else if(co_debug)
		printf("C");
#endif DEBUG
	unit = minor(dev);
	if (unit & CO_FLAG) 
		softp = co_costate;
	else {
		softp = &co_state[unit];
		ASSERT(unit <= co_max_unit, "co_close: bad unit");
		/*
 		 *+ An attempt has been made to pass co_close a device
		 *+ number that is illegal.
		 */
		ASSERT(softp->ss_alive == 1, "co_close: not alive");
		/*
 		 *+ An attempt has been made to pass co_close a device
		 *+ number that is illegal.
		 */
	}

	tp = &softp->ss_tty;
	tplock = CO_TTYLOCK(tp);
	if(--tp->t_nopen > 0 || (tp->t_state&TS_LCLOSE)) {
		CO_TTYUNLOCK(tp, tplock);
		return;
	}
	ASSERT(tp->t_nopen>=0,"co: too many closes");
	/*
	 *+ The close count for co_close hasn;t matched the open count.
	 */
	tp->t_state |= TS_LCLOSE;
	(*linesw[tp->t_line].l_close)(tp);
	if((tp->t_state & TS_ISOPEN) == 0 || (tp->t_state&TS_HUPCLS)) {
		/*
		 * Turn h/w off on hupcls on last close (new line disc).
		 * Assumes all output flushed before here.
		 */
		softp->os_smode.CM_FLAGS |= (SCONS_CLEAR_DTR | 
						SCONS_CLEAR_RTS );
						
		softp->os_smode.CM_FLAGS &= ~(SCONS_SET_BREAK);
		co_wset_modes(softp);
	}
	softp->is_restart_read = 0;
	co_abort_read(softp);
	ttyclose(tp);
	CO_TTYUNLOCK(tp, tplock);
}

/*
 * Standard read.
 *
 * Mutual exclusion: called procedures assume locked tp.
 */
coread(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{	
	register struct co_state *softp;
	register struct tty *tp;
	register int retval;
	int unit;
	spl_t	tplock;

#ifdef DEBUG
	if(co_debug)
		printf("r");
#endif DEBUG
	unit = minor(dev);
	if (unit & CO_FLAG)
		softp = co_costate;
	else {
		softp = &co_state[unit];
		ASSERT(unit <= co_max_unit, "coread: bad unit");
		/*
 		 *+ An attempt has been made to pass coclose a device
		 *+ number that is illegal.
		 */
		ASSERT(softp->ss_alive==1, "coread: not alive");
		/*
 		 *+ An attempt has been made to pass coread a device
		 *+ number that is illegal.
		 */
	}


	
	tp = &softp->ss_tty;
	tplock = CO_TTYLOCK(tp);
	retval = ((*(linesw[tp->t_line].l_read))(tp, uio));
	CO_TTYUNLOCK(tp, tplock);
	return(retval);
}

/*
 * Standard write.
 *
 * Mutual exclusion: called procedures assume locked tp.
 */
cowrite(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	register struct co_state *softp;
	register struct tty *tp;
	register int retval;
	int unit;
	spl_t	tplock;

#ifdef DEBUG
	if(co_debug)
		printf("w");
#endif DEBUG

	unit = minor(dev);
	if (unit & CO_FLAG)
		softp = co_costate;
	else {
		softp = &co_state[unit];
		ASSERT(unit <= co_max_unit, "cowrite: bad unit");
		/*
 		 *+ An attempt has been made to pass cowrite a device
		 *+ number that is illegal.
		 */
		ASSERT(softp->ss_alive == 1, "cowrite: not alive");
		/*
 		 *+ An attempt has been made to pass cowrite a device
		 *+ number that is illegal.
		 */
	}

	tp = &softp->ss_tty;
	tplock = CO_TTYLOCK(tp);
	retval = ((*(linesw[tp->t_line].l_write))(tp, uio));
	CO_TTYUNLOCK(tp, tplock);
	return(retval);
}


cointr(vector)
	int vector;
{
	register int count;
	register caddr_t bptr;
	register int status;
	register struct tty *tp;
	register struct co_state *softp;
	int unit;
	spl_t	tplock;
	int is_read;
	struct sec_dev_prog *completion;

	unit = UNITMINOR(vector - co_base_vec);
	is_read = (((vector - co_base_vec) % 2) == 0);
#ifdef	DEBUG
	if (co_debug)
		printf("C");
#endif	DEBUG
	softp = &co_state[unit];

	if (unit < 0 || unit > co_max_unit) {
		printf("co: stray interrupt, unit out of range %x\n",unit);
                /*
                 *+ An interrupt was received from an unknown SCED port. 
		 *+ The interrupt was discarded.
                 */
		return;
	}

	if (softp->ss_alive == 0) {
		printf("co: stray interrupt, dead unit %x\n", unit);
                /*
                 *+ An interrupt was received from an unknown SCED port. 
		 *+ The interrupt was discarded.
                 */
		return;
	}

	tp = &softp->ss_tty;
	tplock = CO_TTYLOCK(tp);

	if (is_read) {
	while (!CO_TEST_Q_EMPTY(&softp->is_doneq)) {
		completion = CO_GET_Q_TAIL(&softp->is_doneq);
		status = (int)completion->dp_status1;
#ifdef	DEBUG
		if (co_debug)
			printf("I");
		if (co_debug > 1)
			printf("%x ", status);
#endif	DEBUG

		if(status & SCONS_CARR_DET) {
			co_get_modes(softp);
			if (softp->os_smode.CM_FLAGS & SCONS_CARRIER_CLEAR) {
				if (tp->t_state & TS_CARR_ON) {
					tp->t_state &= ~TS_CARR_ON;
					if(((tp->t_state&TS_WOPEN) == 0)
					&&(tp->t_state&TS_ISOPEN)
					&&((tp->t_flags&NOHANG) == 0)) {
						gsignal(tp->t_pgrp, SIGHUP);
						gsignal(tp->t_pgrp, SIGCONT);
						ttyflush(tp, FREAD|FWRITE);
					}
				}
			} else {
				if ((tp->t_state & TS_CARR_ON) == 0) {
					tp->t_state |= TS_CARR_ON;
					vall_sema(&tp->t_rawqwait);
				}
			}
		}
		if (!(status & SCONS_FLUSHED)) {
			if(status & SCONS_PARITY_ERR)
				softp->is_parc++;
			if(status & SCONS_OVRFLOW)
				softp->is_ovc++;
			if(status & SCONS_BREAK_DET)
			     if(tp->t_flags&RAW)
				   (*linesw[tp->t_line].l_rint)(0, tp); 
			     else
				   (*linesw[tp->t_line].l_rint)(tp->t_intrc, tp); 
			count = completion->dp_count;
#ifdef	DEBUG
			if (co_debug > 1)
				printf("%x\n", count);
#endif	DEBUG
			bptr = softp->is_buffer;
			while(count-- > 0) {
#ifdef	DEBUG
				/*
				 * Handle debug stuff.
				 */
	
				if (gc_last == DBGCHAR) {
					gc_last = '\0';
					debugit(*bptr);
				} else {
					gc_last = *bptr;
					if (*bptr != DBGCHAR) 
						(*linesw[tp->t_line].l_rint)(*bptr, tp); 
				}
				bptr++;
#else	!DEBUG
	
				(*linesw[tp->t_line].l_rint)(*bptr, tp); 
				bptr++;
#endif	DEBUG
			}
		}
		CO_INCR_Q_TAIL(&softp->is_doneq);
	}
	if(softp->is_restart_read)
		co_start_read(softp);
	} else {
	while (!CO_TEST_Q_EMPTY(&softp->os_doneq)) {
		completion = CO_GET_Q_TAIL(&softp->os_doneq);
		status = (int)completion->dp_status1;
		count = completion->dp_count;
#ifdef	DEBUG
		if (co_debug)
			printf("O");
		if (co_debug > 1)
			printf("%x %x\n", status, count);
#endif	DEBUG

		tp->t_state &= ~TS_BUSY;

		if(tp->t_state&TS_FLUSH)
			tp->t_state &= ~TS_FLUSH;
		else
			ndflush(&tp->t_outq, count);

		CO_INCR_Q_TAIL(&softp->os_doneq);

		if(tp->t_line)
			(*linesw[tp->t_line].l_start)(tp);
		else
			costart(tp);
	}
	}
	CO_TTYUNLOCK(tp, tplock);
}

/*
 * stioctl - io controls
 *
 * Mutex: assumes called with a non-locked tp.
 */
coioctl(dev, cmd, data, flag)
	dev_t	dev;
	register cmd;
	int	flag;
	caddr_t	data;
{	
	register struct tty *tp;
	register error;
	register int	unit;
	register struct co_state *softp;
	spl_t	 tplock;

#ifdef DEBUG
	if(co_debug)
		printf("coioctl: dev=0x%x, cmd=0x%x, flag=0x%x, data=0x%x\n", 
			dev, cmd, flag, data);
#endif DEBUG

	unit = minor(dev);
	if (unit & CO_FLAG)
		softp = co_costate;
	else {
		softp = &co_state[unit];
		ASSERT(unit <= co_max_unit, "coioctl: bad unit");
		/*
 		 *+ An attempt has been made to pass coioctl a device
		 *+ number that is illegal.
		 */
		ASSERT(softp->ss_alive == 1, "coioctl: not alive");
		/*
 		 *+ An attempt has been made to pass coioctl a device
		 *+ number that is illegal.
		 */
	}

	tp = &softp->ss_tty;

	tplock = CO_TTYLOCK(tp);

	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
	if(error >= 0) {
		CO_TTYUNLOCK(tp, tplock);
		return(error);
	}

	error = ttioctl(tp, cmd, data, flag);
	if(error >= 0) {
		if (cmd == TIOCSETP || cmd == TIOCSETN || cmd == TIOCLBIS ||
		    cmd == OLD_TIOCSETP || cmd == OLD_TIOCSETN ||
		    cmd == TIOCLBIC || cmd == TIOCLSET)
			co_param(softp);
		CO_TTYUNLOCK(tp, tplock);
		return(error);
	}

	/*
	 * Process special stuff...
	 */
	error = 0;
	switch(cmd) {
		case TIOCSBRK:	/* set break on */
			softp->os_smode.CM_FLAGS |= SCONS_SET_BREAK;
			co_wset_modes(softp);
			break;
		case TIOCCBRK:	/* clear break off */
			softp->os_smode.CM_FLAGS &= ~SCONS_SET_BREAK;
			co_wset_modes(softp);
			break;
		case TIOCSDTR:	/* Turn on dtr rts */
			softp->os_smode.CM_FLAGS &= ~(SCONS_CLEAR_DTR | SCONS_CLEAR_RTS);
			co_wset_modes(softp);
			break;
		case TIOCCDTR:	/* turn off dtr rts */
			softp->os_smode.CM_FLAGS |= (SCONS_CLEAR_DTR | SCONS_CLEAR_RTS);
			co_wset_modes(softp);
			break;
		default:
			error = ENOTTY;
	}
	CO_TTYUNLOCK(tp, tplock);
	return(error);
}



/*
 * coselect
 *
 * Mutual exclusion: called procedures assume locked tp.
 */
coselect(dev, rw)
	dev_t	dev;
	int	rw;
{
	register struct tty *tp;
	int	retval;
	spl_t	tplock;
	int 	unit;

	unit = minor(dev);
	if (unit & CO_FLAG)
		tp = &co_costate->ss_tty;
	else
		tp = &co_state[unit].ss_tty;
	tplock = CO_TTYLOCK(tp);
	retval = (*linesw[tp->t_line].l_select)(tp, rw);
	CO_TTYUNLOCK(tp, tplock);
	return(retval);
}

/*
 * This table defines the CO command bits for setting asynchronous
 * baud rates.  '-1' is the signal to turn off the DTR and RTS signals.
 * '-2' indicates that a baud rate is not available on the CO.  The
 * 'Mxxx' entries are for the 'rate' parameter in calls to co_setmodes();
 */


/*
 * co_param - set line parameters on the hardware.
 *
 * Mutex: assumed called with locked tp to insure stable data
 * being read.
 */
co_param(softp)
	register struct co_state *softp;
{
	register struct tty *tp;
	u_char lpar, len, stop, parflag;

#ifdef DEBUG
	if(co_debug>1)
		printf(" co_param: enter  ");
	else if(co_debug)
		printf("P");
#endif DEBUG

	tp = &softp->ss_tty;

	if(tp->t_ispeed == 0) {
		tp->t_state |= TS_HUPCLS;
		softp->os_smode.CM_FLAGS |= (SCONS_CLEAR_DTR | 
						SCONS_CLEAR_RTS);
		softp->os_smode.CM_FLAGS &= ~(SCONS_SET_BREAK);
		co_wset_modes(softp);
		return;
	}

	if (tp->t_flags & (RAW|LITOUT|PASS8)) {
		len = SCONS_DATA8;
		parflag = 0;
	} else {
		 len = SCONS_DATA7;
		 parflag = 1;
	}
	lpar = 0;
	if (parflag)
		if ((tp->t_flags&EVENP) == 0)
			lpar = SCONS_ODD_PARITY;
		else
			lpar = SCONS_EVEN_PARITY;
	if ((tp->t_ospeed) == B110)
		stop = SCONS_STOP2;
	else
		stop = SCONS_STOP1;

	softp->os_smode.CM_BAUD = tp->t_ospeed;
	softp->os_smode.CM_FLAGS &= ~(SCONS_STOP1P5 | SCONS_STOP2 |
					SCONS_DATA7 |
					SCONS_EVEN_PARITY |
					SCONS_ODD_PARITY);
	softp->os_smode.CM_FLAGS |= (stop | len | lpar);
	co_wset_modes(softp);
}
int ttrstrt();

/*
 * costart - start a character out on the interface.
 *
 * This procedure starts a character out by calculating the maximum
 * number of characters it can get from the clist and then programming
 * the scsi to send that many characters from the clist.
 * When the device program completes there will
 * be included in the completion a byte count which is what was transmitted.
 *
 * Assumes: caller locks the tp before calling costart().
 */
costart(tp)
	register struct tty *tp;
{
	register int nch;
	register struct co_state *softp;
	u_char *addr;

	/*
	 * If the channel is already working, or if it is waiting on
	 * a delay, then nothing is done right now.
	 */
#ifdef DEBUG
	if(co_debug>1)
		printf("c\n");
#endif DEBUG

	if(tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP)) {
		return;
	}

	/*
	 * let those wanting to do setmodes get a chance first.
	 */

	softp = (struct co_state *)tp->t_addr;
	if(softp->os_busy_flag) {
		softp->os_busy_flag = 0;
		vall_sema(&softp->os_busy_wait);
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
		if(tp->t_wsel) {
			selwakeup(tp->t_wsel, tp->t_state&TS_WCOLL);
			tp->t_state &= ~TS_WCOLL;
			tp->t_wsel = (struct proc *)NULL;
		}
	}

	if(tp->t_outq.c_cc == 0) { 	/* Nothing to process */
		return;
	}


	/*
	 * Dma from clist direct.
	 *
	 * Find where on clist and tell board to dma from there. When the interrupt
	 * comes in that signals the block output done all we have to do 
	 * is ndflush the clist which is much less overhead then many getc's.
	 */
	if(tp->t_flags & (RAW|LITOUT|PASS8))
		nch = ndqb(&tp->t_outq, 0);
	else{	
		/*
		 * not raw so check for timeout chars and if we 
		 * have dma'd out to one then do the timeout.
		 */
		nch = ndqb(&tp->t_outq, 0200);
		if(nch==0) {
			nch = getc(&tp->t_outq);
			timeout(ttrstrt, (caddr_t)tp, ((nch & 0x7f) + 6));
			tp->t_state |= TS_TIMEOUT;
			return;
		}
	}

	addr = (u_char *)tp->t_outq.c_cf;
#ifdef DEBUG
	if(co_debug>2)
		printf("BO: %d\n", nch);
	else if(co_debug)
		printf("X");
#endif DEBUG
	ASSERT(((nch>0) && (nch<=CBSIZE)), "co:Bad dma count");
	/*
	 *+ An attempt has been made to dma more characters than the sec 
	 *+ can support.
         */
	co_start_write(softp, nch, addr);
	tp->t_state |= TS_BUSY;
}

/*
 * costop - Abort an in progress dma on a channel.
 *
 * This procedure aborts output on a line.
 * Curious note:  No driver seems to ever use the flag parameter.
 *
 * Mutex: Assumes caller has locked the tp prior to
 *	  calling this procedure.
 */
/*ARGSUSED*/
costop(tp, flag)
	register struct tty *tp;
{
	register struct co_state *softp;

#ifdef DEBUG
	if(co_debug)
		printf("S");
#endif DEBUG

	if (tp->t_state & TS_BUSY) {
		/*
		 * Device is transmitting; stop output.
		 * We will clean up later
		 * by examining the xfer count for a completion.
		 *
		 * The TS_FLUSH flag is used to tell us HOW to clean
		 * up.  If it is set, the characters have 
		 * aleady been cleaned off the t_outq by ttyflush().
		 * Otherwise, we need to clean off the ones that have
		 * been transmitted.
		 */
		if ((tp->t_state&TS_TTSTOP)==0)
			tp->t_state |= TS_FLUSH;

		softp = (struct co_state *)tp->t_addr;
		co_abort_write(softp);
	}
}



/*
 * co_start_write   -queue a write request to the output channel.
 *
 * assume request queue and status are locked prior to entry
 * also assumes that the request queue is not full.
 */
co_start_write(softp, nch, addr)
	register struct co_state *softp;
	register int nch;
	register u_char *addr;
{
	register struct sec_dev_prog *request;

	request = CO_GET_Q_HEAD(&softp->os_reqq);
	request->dp_status1 = 0;
	request->dp_count = 0;
	request->dp_un.dp_data = addr;
	request->dp_data_len = nch;
	request->dp_next = (struct sec_dev_prog*)0;
	CO_INCR_Q_HEAD(&softp->os_reqq);
	sec_startio(SINST_STARTIO, &softp->os_status, softp->os_sd);
	if (softp->os_status & (~SINST_INSDONE)) {
		printf("co: bad start_write: %x\n", softp->os_status);
                /*
                 *+ The SCED failed to acknowledge a start-write command.
                 */
	}
}

/*
 * co_start_read   -queue a read request to the output channel.
 *
 * assume request queue and status are locked prior to entry
 * also assumes that the request queue is not full.
 */
co_start_read(softp)
	register struct co_state * softp;
{
	register struct sec_dev_prog *request;

	request = CO_GET_Q_HEAD(&softp->is_reqq);
	request->dp_status1 = 0;
	request->dp_count = 0;
	request->dp_un.dp_data = (u_char *)softp->is_buffer;
	request->dp_data_len = CBSIZE;
	request->dp_next = (struct sec_dev_prog*)0;
	CO_INCR_Q_HEAD(&softp->is_reqq);
	sec_startio(SINST_STARTIO, &softp->is_status, softp->is_sd);
	if (softp->is_status & (~SINST_INSDONE)) {
		printf("co: bad start_read: %x\n", softp->is_status);
                /*
                 *+ The SCED failed to acknowledge a start-read command.
                 */
	}
}

/*
 * co_wset_modes - set console modes.
 *
 * This procedure assumes that access to the os_smode structure and
 * to the cib are locked.  It waits until the transmitting channel is
 * not busy, then does the set modes.  This ensures that the last of
 * previous output has gone to the display before changing anything.
 */
co_wset_modes(softp)
	register struct co_state *softp;
{
	register struct tty *tp;

	tp = &softp->ss_tty;
	while (tp->t_state & TS_BUSY) {
		softp->os_busy_flag = 1;
		p_sema_v_lock(&softp->os_busy_wait, TTIPRI, &tp->t_ttylock, SPLTTY);
		(void) CO_TTYLOCK(tp);
	}

	sec_startio(SINST_SETMODE, &softp->os_smode.sm_status, softp->os_sd);
	if (softp->os_smode.sm_status & (~SINST_INSDONE)) {
		printf("co: bad set_mode: %x\n", softp->os_smode.sm_status);
                /*
                 *+ The SCED failed to acknowledge a set_mode command.
                 */
	}
#ifdef	DEBUG
	if (co_debug > 1)
		printf("setmode done flags %x, baud %x\n",
		softp->os_smode.CM_FLAGS, softp->os_smode.CM_BAUD);
#endif	DEBUG
	costart(tp);
}

/*
 * co_get_modes - get console modes.
 *
 * This procedure assumes that access to the os_smode structure and
 * to the cib are locked.  
 */
co_get_modes(softp)
	register struct co_state *softp;
{
	sec_startio(SINST_GETMODE, &softp->os_smode.sm_status, softp->os_sd);
	if (softp->os_smode.sm_status & (~SINST_INSDONE)) {
		printf("co: bad get_mode: %x\n", softp->os_smode.sm_status);
                /*
                 *+ The SCED failed to acknowledge a get_mode command.
                 */
	}
#ifdef	DEBUG
	if (co_debug > 1)
		printf("getmode done flags %x, baud %x\n",
		softp->os_smode.CM_FLAGS, softp->os_smode.CM_BAUD);
#endif	DEBUG
}



/*
 * co_abort_write - abort any pending or current write device programs
 *
 * This procedure assumes that access to the output structures are locked.  
 */
co_abort_write(softp)
	register struct co_state *softp;
{
	sec_startio(SINST_FLUSHQUEUE, &softp->os_status, softp->os_sd);
	if (softp->os_status & (~SINST_INSDONE)) {
		printf("co: bad abort_write: %x\n", softp->os_status);
                /*
                 *+ The SCED failed to acknowledge a abort-write command.
                 */
	}
}

/*
 * co_abort_read - abort any pending or current read device programs
 *
 * This procedure assumes that access to the output structures are locked.  
 */
co_abort_read(softp)
	register struct co_state *softp;
{
	sec_startio(SINST_FLUSHQUEUE, &softp->is_status, softp->is_sd);
	if (softp->is_status & (~SINST_INSDONE)) {
		printf("co: bad abort_read: %x\n", softp->is_status);
                /*
                 *+ The SCED failed to acknowledge a abort-write command.
                 */
	}
}

/*
 * Program queue management routines.  These may be candidates for
 * implementation as macros.  Or, they may be moved in line and
 * implemented more efficiently later.
 *
 * CO_TEST_Q_EMPTY  - return true if program queue is empty
 */

CO_TEST_Q_EMPTY(sq)
	struct sec_pq	*sq;
{
	struct sec_progq *pq;

	pq = sq->sq_progq;
	ASSERT(pq->pq_tail < sq->sq_size, "co_testq: bad tail");
        /*
         *+ The SCED console driver queue manipulation routine
         *+ encountered a bad tail pointer.
         */
	ASSERT(pq->pq_head < sq->sq_size, "co_testq: bad head");
        /*
         *+ The SCED console driver queue manipulation routine
         *+ encountered a bad head pointer.
         */

	return(pq->pq_head == pq->pq_tail);
}

/*
 * CO_GET_Q_HEAD  - return pointer to first available "empty" device
 *			program.
 */

struct sec_dev_prog *
CO_GET_Q_HEAD(sq)
	struct sec_pq	*sq;
{
	struct sec_progq *pq;

	pq = sq->sq_progq;
	ASSERT(pq->pq_tail < sq->sq_size, "co_getq: bad tail");
        /*
         *+ The SCED console driver queue manipulation routine
         *+ encountered a bad tail pointer.
         */
	ASSERT(pq->pq_head < sq->sq_size, "co_getq: bad head");
        /*
         *+ The SCED console driver queue manipulation routine
         *+ encountered a bad head pointer.
         */

	return(pq->pq_un.pq_progs[pq->pq_head]);
}

/*
 * CO_INCR_Q_HEAD  - increment the head of the queue, thus adding a
 *			device program to the request queue.
 */

CO_INCR_Q_HEAD(sq)
	struct sec_pq	*sq;
{
	struct sec_progq *pq;

	pq = sq->sq_progq;
	ASSERT(pq->pq_tail < sq->sq_size, "co_incrq: bad tail");
        /*
         *+ The SCED console driver queue manipulation routine
         *+ encountered a bad tail pointer.
         */
	ASSERT(pq->pq_head < sq->sq_size, "co_incrq: bad head");
        /*
         *+ The SCED console driver queue manipulation routine
         *+ encountered a bad head pointer.
         */

	pq->pq_head = (pq->pq_head + 1) % sq->sq_size;
}

/*
 * CO_GET_Q_TAIL  - return pointer to first completed device
 *			program.
 */

struct sec_dev_prog *
CO_GET_Q_TAIL(sq)
	struct sec_pq	*sq;
{
	struct sec_progq *pq;

	pq = sq->sq_progq;
	ASSERT(pq->pq_tail < sq->sq_size, "co_get_q_tail: bad tail");
        /*
         *+ The SCED console driver queue manipulation routine
         *+ encountered a bad tail pointer.
         */
	ASSERT(pq->pq_head < sq->sq_size, "co_get_q_tail: bad head");
        /*
         *+ The SCED console driver queue manipulation routine
         *+ encountered a bad head pointer.
         */

	return(pq->pq_un.pq_progs[pq->pq_tail]);
}

/*
 * CO_INCR_Q_TAIL  - increment the tail of the queue, thus deleting a
 *			device program to the completion queue.
 */

CO_INCR_Q_TAIL(sq)
	struct sec_pq	*sq;
{
	struct sec_progq *pq;

	pq = sq->sq_progq;
	ASSERT(pq->pq_tail < sq->sq_size, "co_incr_q_tail: bad tail");
        /*
         *+ The SCED console driver queue manipulation routine
         *+ encountered a bad tail pointer.
         */
	ASSERT(pq->pq_head < sq->sq_size, "co_incr_q_tail: bad head");
        /*
         *+ The SCED console driver queue manipulation routine
         *+ encountered a bad head pointer.
         */

	pq->pq_tail = (pq->pq_tail + 1) % sq->sq_size;
}
