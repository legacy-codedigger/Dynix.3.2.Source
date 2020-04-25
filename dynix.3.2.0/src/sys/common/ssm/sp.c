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
static char rcsid[]= "$Header: sp.c 1.5 90/11/08 $";
#endif

/*
 *     SSM parallel printer driver software
 *
 */

/* $Log:	sp.c,v $
 */
#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/systm.h"
#include "../balance/clock.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/proc.h"
#include "../balance/cfg.h"
#include "../balance/slic.h"
#include "../machine/intctl.h"
#include "../ssm/ioconf.h"
#include "../ssm/ssm.h"
#include "../ssm/ssm_misc.h"
#include "../ssm/sp.h"

int spprobe(), spboot(), spintr(), sp_timer();
static int spmaxdev;
static int sp_base_vector;
static struct sp_info **sp_info;

/*
 * External variables declared in conf_sp.c
 */
extern struct sp_printer spconfig[];	/* binary configuration table */
extern int spprinters;			/* no. of entries in spconfig */ 

struct ssm_driver sp_driver = {
	/* driver prefix, configuration flags, probe(),   boot(), interrupt() */
	      "sp",       SDR_TYPICAL_PRNT, spprobe,  spboot,  spintr
};

/*
 * spprobe - probe an SSM for a printer device.
 *
 * Look at the flags returned by the powerup firmware.
 */
static
spprobe(probe)
	struct ssm_probe *probe;
{
	/*
	 * Check for bad unit number, or diagnostic test
	 * failure.
	 */
	if (probe->sp_unit != PCB_PORT0 
	    || probe->sp_desc->ssm_diag_flags & CFG_SSM_PRINT)
		return(0);
	return(1);
}

/*
 * spboot - allocate data structures, etc at beginning of time.
 *
 * Called by autoconfig with an array of configured devices and their
 * number.  We allocate the necessary structures and fill them
 * with the various fields from the ssm_dev array.   
 */
static
spboot(ndevs, devp)
	int ndevs;
	register struct ssm_dev	*devp;
{
	struct sp_info	*sp;
	int pindex; 
	int dev;
	char *ssm_alloc();
	
	sp_base_vector = devp->sdv_sw_intvec;
	
	/* Allocate an array of pointer to info structures */
	sp_info = (struct sp_info **)calloc(ndevs * sizeof(struct sp_info *));
	
	/* Set up info structure for each SSM device in system */
	for (dev = 0;  dev < ndevs; dev++, devp++){
		if (!devp->sdv_alive)
			continue;
		
		/* ignore SSM printer ports on slave SSM boards */
		if (!devp->sdv_desc->ssm_is_cons)
			continue;
		
		sp_info[dev] = sp = (struct sp_info *)
				         calloc(sizeof(struct sp_info));
		sp->sp_buffer = ssm_alloc(SP_BUFSIZE, SSM_ALIGN_BASE,
					SSM_BAD_BOUND);
		sp->sp_dev = devp;
		sp->sp_devno = devp->sdv_unit;
		if (dev >= spprinters) {
			/*
			 * If there are more printers than there are
			 * entries in spconfig, use the last entry in
			 * the table for the remaining devices.
			 */
			printf ("printer %d not configured, using ");
			printf ("printer %d's configuration\n", 
					dev, spprinters-1);
			pindex = spprinters-1;
		} else
			pindex = dev;
		sp->sp_width = (SPLONGLINE < spconfig[pindex].sp_width) ?
				SPLONGLINE : spconfig[pindex].sp_width;
		sp->sp_ops = spconfig[pindex].sp_ops;
		sp->sp_state = 0;
		sp->sp_line = 0;
		init_ssm_prnt_dev(devp, spconfig[pindex].sp_interface);
		init_sema(&sp->sp_wait, 0, 0, spgate);
		init_sema(&sp->sp_bufsema, 1, 0, spgate);
		init_lock(&sp->sp_lock, spgate);
	}
	spmaxdev = --ndevs;
}

/*
 * spopen - open an SSM printer port
 *
 * Open the device for exclusive use, initialize fields in device
 * structure, and wait for printer to be ready before returning.
 */
/*ARGSUSED*/
spopen(dev, flag)
	register dev_t	dev;
	int flag;
{
	register struct sp_info *sp;
	int unit = minor(dev);


	/*
	 * Check for bad device number. 
	 */
	if (unit > spmaxdev || (sp = sp_info[unit]) == NULL || 
			!sp->sp_dev->sdv_alive)
		return(ENXIO);
	
	/*
	 * Lock the device.
	 */
	sp->sp_spl = p_lock(&sp->sp_lock, SP_SPL);

	/*
	 * If device is open or in process of being opened, return EBUSY.
	 */
	if (sp->sp_state & (SP_OPEN | SP_WOPEN)) {
		v_lock(&sp->sp_lock, sp->sp_spl);
		return(EBUSY);
	}
	if (sp_ready(sp))
		sp->sp_state |= SP_READY;
	else
		sp->sp_state &= ~SP_READY;
	if (sp->sp_state & SP_DATAOUT || !(sp->sp_state & SP_READY)) {
		/*
		 * printer not ready so sleep until it is
		 */
		sp->sp_state |= SP_WOPEN;	/* partial open */
		if (!(sp->sp_state & SP_READY))
			timeout(sp_timer, (caddr_t)sp, 2*HZ);
		while ((sp->sp_state & SP_DATAOUT )
		       || !(sp->sp_state & SP_READY)) {
			/*
			 * Must loop until ready because could be woken
			 * as a result of SIGSTOP/SIGCONT
			 */
			p_sema_v_lock(&sp->sp_wait, PZERO+1, &sp->sp_lock, 
					sp->sp_spl);
			sp->sp_spl = p_lock(&sp->sp_lock, SP_SPL);
		}
	}

	sp->sp_state = SP_OPEN | SP_READY;

	sp->sp_curp = sp->sp_buffer; 
	sp->sp_count = 0;
	sp->sp_col = sp->sp_lcol = 0;
	if (sp->sp_ops != SPRAW)
		sp_canon(sp, FORMFEED); 
	v_lock(&sp->sp_lock, sp->sp_spl);
	return(0);
}

/*
 * spclose - close an SSM printer port
 *
 * Flush remaining buffer contents and mark fields to indicate device
 * is closed.  Also check for and cleanup after signals.
 */
/*ARGSUSED*/
spclose(dev, flag)
	dev_t dev;
	int flag;
{
	register struct sp_info *sp = sp_info[minor(dev)];
	register struct print_xcb *xcb;
	spl_t s;

	sp->sp_spl = p_lock(&sp->sp_lock, SP_SPL);
	if (sp->sp_state & SP_WOPEN) {
		/*
		 * This means signal occurred while waiting in
		 * spopen(), so cleanup
		 * (must unlock before calling untimeout
		 *  to avoid deadlock)
		 */
		v_lock(&sp->sp_lock, sp->sp_spl);
		untimeout(sp_timer, (caddr_t)sp);
		sp->sp_state = 0;
		return;
	}
#ifdef SPDEBUG
	if (!(sp->sp_state & SP_OPEN)) {
		/*
		 * sanity check -- shouldn't happen
		 */
		 printf("spclose: printer %d not open\n", dev);
		 v_lock(&sp->sp_lock, sp->sp_spl);
		 return;
	}
#endif SPDEBUG
	if (setjmp(&u.u_qsave)) {
		/*
		 * cleanup if signal arrives during close
		 */
		u.u_error = EINTR;
		sp->sp_spl = p_lock(&sp->sp_lock, SP_SPL);
		xcb = (struct print_xcb *) 
	   	    PRINT_BASE_CB(sp->sp_dev->sdv_desc->ssm_prnt_cbs,
		    sp->sp_devno) + PCB_XMIT_CB;
		xcb->xcb_cmd = PCB_FLUSH;
		s = splhi();
		mIntr(sp->sp_dev->sdv_desc->ssm_slicaddr, PRINT_BIN,
		      (u_char)PRVEC(PCB_FLUSH_V));
		splx(s);	
		while (xcb->xcb_status == PCB_BUSY)
			continue;
		v_sema(&sp->sp_bufsema);
		sp->sp_state = 0; 
		v_lock(&sp->sp_lock, sp->sp_spl);
		return;
	}
	/*
	 * get exclusive use of buffer
	 */
	p_sema_v_lock(&sp->sp_bufsema, PZERO-1, &sp->sp_lock, sp->sp_spl);
	sp->sp_spl = p_lock(&sp->sp_lock, SP_SPL);
	if (sp->sp_ops != SPRAW)
		sp_canon(sp, FORMFEED); 
	sp_output(sp, 1);
	v_sema(&sp->sp_bufsema);
	sp->sp_state &= SP_DATAOUT;	/* zero except for DATAOUT bit */ 
	v_lock(&sp->sp_lock, sp->sp_spl);
	return;
}

/*
 * spread
 *
 * This merely returns a zero status, since there is no
 * read capability on the printer port
 */
/*ARGSUSED*/
spread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	return(0);
}


/*
 * spwrite - write to SSM printer port
 *
 * Move data from user buffer to printer buffer, doing the specified
 * character mapping, and transmitting when printer buffer is full.
 * (Transmission occurs inside sp_canon)
 */
spwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct sp_info *sp = sp_info[minor(dev)];
	register unsigned n;
	register char *cp;
	struct print_xcb *xcb;
	int error;
	spl_t s;
	char rawbuf[SPRBSIZE];

	if (setjmp(&u.u_qsave)) {
		/*
		 * cleanup if signal arrives during transmit
		 */
		xcb = (struct print_xcb *)
		      PRINT_BASE_CB(sp->sp_dev->sdv_desc->ssm_prnt_cbs,
		      sp->sp_devno) + PCB_XMIT_CB;
		sp->sp_spl = p_lock(&sp->sp_lock, SP_SPL);
		xcb->xcb_cmd = PCB_FLUSH;
		s = splhi();
		mIntr(sp->sp_dev->sdv_desc->ssm_slicaddr, PRINT_BIN,
		      (u_char)PRVEC(PCB_FLUSH_V));
		splx(s);
		while (xcb->xcb_status == PCB_BUSY)
			continue;
		v_sema(&sp->sp_bufsema);
		v_lock(&sp->sp_lock, sp->sp_spl);
		return(EINTR);
	}
	while (n = MIN(SPRBSIZE, (unsigned)uio->uio_resid)) {
		if ((error = uiomove(rawbuf, (int)n, UIO_WRITE, uio)))
			return(error);
		cp = rawbuf;
		/*
		 * get exclusive use of buffer before obtaining
		 * lock
		 */
		p_sema(&sp->sp_bufsema, PZERO-1);
		sp->sp_spl = p_lock(&sp->sp_lock, SP_SPL);
		while (n--)
			sp_canon(sp, *cp++);
		if (sp->sp_state & SP_ERROR) {
			sp->sp_state &= ~SP_ERROR;
			v_lock(&sp->sp_lock, sp->sp_spl);
			v_sema(&sp->sp_bufsema);
			return(ENXIO);
		}
		v_lock(&sp->sp_lock, sp->sp_spl);
		v_sema(&sp->sp_bufsema);
	}
	return(0);
}

/*
 * sp_output - does buffer management and starts I/O
 *
 * Call sp_start and then wait for transfer to complete.
 * (assumes SSM handles ONLINE/OFFLINE changes which occur
 * during a transfer transparently -- i.e. the DONE interrupt
 * will not occur until the transfer is actually complete,
 * even if the printer goes offline during the transfer)
 *
 * (must be called with sp_info locked)
 */
static
sp_output(sp, closeflag)
	register struct sp_info *sp;
{
	sp->sp_xcount = sp->sp_count;
	sp->sp_count = 0;
	sp->sp_xptr = sp->sp_curp = sp->sp_buffer;

	while (sp->sp_xcount && !(sp->sp_state & SP_ERROR)) {
		if (closeflag)
			sp->sp_state |= SP_DATAOUT;
		sp_start(sp);
		while (sp->sp_state & SP_BUSY) {
			/* loop in case woken by SIGSTOP/SIGCONT */
			p_sema_v_lock(&sp->sp_wait, PZERO+1, &sp->sp_lock,
					sp->sp_spl);
			sp->sp_spl = p_lock(&sp->sp_lock, SP_SPL);
		}
		if (closeflag)
			break;
	}
}

/*
 * sp_start - Send characters to printer. 
 *
 * Send transmit command to SSM.
 * (must be called with sp_info structure locked)
 */
static
sp_start(sp)
	register struct sp_info *sp;
{
	register struct print_xcb *xcb;
	spl_t s;

	if (sp->sp_xcount == 0)
		return;

	sp->sp_state |= SP_BUSY;

	xcb = (struct print_xcb *)
		PRINT_BASE_CB(sp->sp_dev->sdv_desc->ssm_prnt_cbs, 
		sp->sp_devno) + PCB_XMIT_CB;
	xcb->xcb_cmd = PCB_XMIT | PCB_IENABLE;
	xcb->xcb_status = PCB_BUSY;
	xcb->xcb_count = sp->sp_xcount;
	xcb->xcb_addr = (u_long)(sp->sp_xptr);
	s = splhi();
	mIntr(sp->sp_dev->sdv_desc->ssm_slicaddr, PRINT_BIN,
			(u_char)PRVEC(PCB_XMIT_V));
	splx(s);
}

/*
 * spintr - SSM interrupt service routine
 */
spintr(vector)
	int vector;
{
	int unit = (vector - sp_base_vector) / NVEC_FROM_SSM;
	int type = (vector - sp_base_vector) % NVEC_FROM_SSM;
	struct sp_info *sp;
	struct print_xcb *xcb;
	spl_t s;

	if (unit < 0 || unit > spmaxdev) {
		printf("sp: stray interrupt, unit out of range %d\n",unit);
		return;
	}
	if ((sp = sp_info[unit]) == NULL) {
		printf("sp: stray interrupt, dead unit %d\n", unit);
		return;
	}
	s = p_lock(&sp->sp_lock, SP_SPL);
	if (type == PCB_READY) {
		/*
		 * printer now ready so wakeup anybody waiting
		 * to transmit so they can restart
		 */
		sp->sp_state |= SP_READY;
		sp->sp_state &= ~SP_BUSY;
		if (sp->sp_state & SP_DATAOUT)
			sp_start(sp);
		else
			vall_sema(&sp->sp_wait);
		v_lock(&sp->sp_lock, s);
		return;
	}
	/*
	 *  This is a transmit complete interrupt
	 */
	xcb = (struct print_xcb *)
			PRINT_BASE_CB(sp->sp_dev->sdv_desc->ssm_prnt_cbs, 
			sp->sp_devno) + PCB_XMIT_CB;
	
	/*
	 * update transfer count and address
	 */
	sp->sp_xcount = xcb->xcb_count;
	sp->sp_xptr = (char *)xcb->xcb_addr;
	if (xcb->xcb_status == PCB_ERR && xcb->xcb_pstatus != PCM_READY) {
		/*
		 * Printer became "not ready" (offline, out-of-paper..)
		 * while transmitting --- don't wakeup process until 
		 * it becomes ready again.
		 */
		sp->sp_state &= ~SP_READY;
		if (!(sp->sp_state & SP_DATAOUT)) {
			v_lock(&sp->sp_lock, s);
			return;
		}
	} else {
		if (xcb->xcb_status == PCB_ERR)
			sp->sp_state |= SP_ERROR;
		sp->sp_state &= ~SP_DATAOUT;
	}
	sp->sp_state &= ~SP_BUSY;
	vall_sema(&sp->sp_wait);
	v_lock(&sp->sp_lock, s);
}

/*
 * sp_ready - get current status of printer
 *		
 * Determine if printer is online, has paper, and has no
 * errors. Return 1 if ready, 0 if not.
 * (must be called with sp_info locked) 
 */
static
sp_ready(sp)
	register struct sp_info *sp;
{
	register struct print_mcb *mcb;
	spl_t s;

	mcb = (struct print_mcb *)
			PRINT_BASE_CB(sp->sp_dev->sdv_desc->ssm_prnt_cbs,
			sp->sp_devno) + PCB_MSG_CB;
	mcb->mcb_cmd = PCB_GSTAT;
	mcb->mcb_status = PCB_BUSY;
	mcb->mcb_iface = 0;
	s = splhi();
	mIntr(sp->sp_dev->sdv_desc->ssm_slicaddr, PRINT_BIN,
			(u_char)PRVEC(PCB_MSG_V));
	splx(s);
	while (mcb->mcb_status == PCB_BUSY)
		continue;
	if (mcb->mcb_status == PCB_ERR || mcb->mcb_iface != PCM_READY)
		return(0);
	return(1);
}

/*
 * sp_timer - when printer is not ready, runs every 2*HZ to detect
 *	      change of state
 */
static
sp_timer(sp)
	struct sp_info *sp;
{
	spl_t s;

	s = p_lock(&sp->sp_lock, SP_SPL);
	if (sp_ready(sp)) {
		sp->sp_state |= SP_READY;
		vall_sema(&sp->sp_wait);
	} else
		timeout(sp_timer, (caddr_t)sp, 2*HZ);
	v_lock(&sp->sp_lock, s);
}

/*
 * macro used by sp_canon for inserting chars in buffer and 
 * starting transfer when needed.
 */
#define SPUTC(c) { \
	*sp->sp_curp++ = (c); \
	sp->sp_count++; \
	if (sp->sp_curp == endbuf) \
		sp_output(sp, 0); \
}

/*
 * sp_canon - canonical output
 *
 * Process characters if configured for mapping, insert into printer
 * buffer, and output when buffer fills.
 * (must be called with sp_info locked)
 */
static
sp_canon(sp, c) 
	register struct sp_info *sp;
	register int c;
{
	register int logcol;
	register char *endbuf;

	endbuf = sp->sp_buffer + SP_BUFSIZE;
	if (sp->sp_ops == SPRAW) {
		SPUTC(c);
		return;
	}

	logcol = sp->sp_lcol;
	if (sp->sp_ops == SPCAPS) {
		int c2;

		if (c >= 'a' && c <= 'z')
			c += 'A' - 'a';
		else {
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
				sp_canon(sp, c2);
			 	c = '-';	
				break;
			}	
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
			if (sp->sp_line == 0 && sp->sp_col == 0)
				break;
			/* 
			 * falls into ... 
			 */

		case '\n':
			SPUTC(c);
			if (c == '\f')
				sp->sp_line = 0;
			else
				sp->sp_line++;
			sp->sp_col = 0;
			/* 
			 * falls into ... 
			 */

		case '\r':	
			logcol = 0;
			break;

		default: 		/* must be a printable character */
			/*
			 * Have we backspaced over the beginning of
			 * line?
			 */
			if (logcol < sp->sp_col) {
				SPUTC('\r');
				sp->sp_col = 0;
			}

			if (logcol < sp->sp_width) {
				while (logcol > sp->sp_col) {
					SPUTC(' ');
					sp->sp_col++;
				}
				SPUTC(c);
				sp->sp_col++;
			}
			logcol++;
		}

	sp->sp_lcol = logcol;
}

/*
 * init_ssm_prnt_dev - initialize SSM printer port 
 *
 * Initializes an SSM printer port by sending a PCB_INIT 
 * message to the SSM.
 * Assumes that mIntr() retries messages until they succeed.
 */
init_ssm_prnt_dev(devp, interface)
	struct ssm_dev *devp;
	u_char interface;
{
	register struct print_icb *iptr;
	spl_t s;

	iptr = (struct print_icb *) (PRINT_BASE_CB(devp->sdv_desc->ssm_prnt_cbs, devp->sdv_unit) + 
			             PCB_MSG_CB);
	iptr->icb_cmd = PCB_INIT;
	iptr->icb_dest = SL_GROUP | TMPOS_GROUP;
	iptr->icb_basevec = devp->sdv_sw_intvec;
	iptr->icb_interface = interface;
	iptr->icb_scmd = SL_MINTR | devp->sdv_bin;
	iptr->icb_status = PCB_BUSY;

	s = splhi();
	mIntr(devp->sdv_desc->ssm_slicaddr, PRINT_BIN, (u_char)PRVEC(PCB_MSG_V));
	splx(s);
	while (iptr->icb_status == PCB_BUSY)
		continue;
}

