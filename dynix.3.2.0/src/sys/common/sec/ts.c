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
static	char	rcsid[] = "$Header: ts.c 2.30 1991/05/15 15:57:52 $";
#endif

/*
 * ts.c - SCSI tape device driver
 */

/* $Log: ts.c,v $
 *
 *
 */

#define	TS_DEBUG	1
#define	DISCONNECT	1

#include "../h/param.h"		/* sizes */
#include "../h/mutex.h"		/* gates and such */
#include "../h/cmap.h"
#include "../h/user.h"		/* user info and errno.h */
#include "../h/proc.h"
#include "../h/file.h"		/* flag defines */
#include "../h/conf.h"		/* Configuration info */
#include "../h/buf.h"		/* struct buf, header and such */
#include "../h/uio.h"		/* struct uio */
#include "../h/systm.h"		/* more sizes, may not be needed */
#include "../h/ioctl.h"		/* io copin's... */
#include "../h/mtio.h"		/* ioctl cmds from mt/others.. */
#include "../h/vm.h"
#include "../h/cmn_err.h"

#include "../balance/engine.h"	/* for debug and engine structures */
#include "../balance/slic.h"	/* slic access */

#include "../machine/ioconf.h"	/* IO Configuration Definitions */
#include "../machine/pte.h"	/* page tables info */
#include "../machine/intctl.h"	/* spl declarations */

#include "../sec/sec.h"		/* scsi common data structures */
#include "../sec/sec_ctl.h"	/* scsi drivers' common stuff */
#include "../sec/scsi.h"	/* scsi command definitions */
#include "../sec/ts.h"		/* driver local structures */

/*
 * Externs and global data structures.
 */
extern	gate_t	tsgate;			/* Real gate for locks... */
extern  struct	ts_bconf tsbconf[];	/* Binary configuration info (bc)*/
struct	ts_info	**tsifd;		/* Unit interrupt mapping base ptr */

extern int	tssensebuf_sz;		/* Sense buffer info - bc */
extern int	tsmaxndevs;		/* Device numbering info - bc */
int		tsmaxunit;		/* Largest accessible device unit */
int		ts_baselevel;		/* Base interrupt level */
extern caddr_t	topmem;			/* highest addressable phys location */
int		tsbigmem = 0;		/* we have > 128M */

#define	KVIRTTOPHYS(addr)	(PTETOPHYS(Sysmap[btop(addr)]) + ((int)(addr) & (NBPG-1)))

struct sec_dev_prog *TS_GET_Q_TAIL();

#define	fsbtodb(c)	(c)

int	tsprobe(), tsboot(), tsintr(), tsstrat();    /* Forward reference */

struct sec_driver ts_driver = {
/*	name  base	flags			 probe	  boot	  intr */
	"ts", 0x20,	SED_TYPICAL|SED_IS_SCSI, tsprobe, tsboot, tsintr
};

/*
 * Tables for error messages
 */

struct ts_errors {
	char *data;
	int  prntflag;
	} ts_errors[] = {{NULL, 0}, 
			{"Recoverable error", 0},
			{"Tape not ready", 0},
			{"Media error", 1},
			{"Hardware error", 1},
			{"Illegal request", 1},
			{NULL, 0},
			{"Media is protected", 0},
			{NULL, 0},
			{"Vendor unique error", 1},
			{"Aborted command", 1},
			{"Aborted command", 1},
			{"Unknown error", 1},
			{"Volume overflow", 0},
			{"Unknown error", 1},
			{"Unknown error", 1},
			{"Vendor Unique error", 1}};

char *ts_cmd[] = {"test unit ready",
		  "rewind",
		  "retension",
		  "request sense",
		  "", "", "", "",
		  "read",
		  "",
		  "write",
		  "", "", "", "", "",
		  "write file marks",
		  "space",
		  "", "", "",
		  "mode select",
		  "", "", "",
		  "erase",
		  "", ""};


/*
 * Driver development helpers.
 */
#ifdef TS_DEBUG
int	ts_debug = 0;	/* 0=off 1=little 2=more 3=lots >3=all */
#endif TS_DEBUG

/*
 * The following data structures are only used for probing
 */
struct sec_dev_prog ts_devprog;		/* one device program */
struct sec_req_sense	ts_statb;	/* one request sense program */
u_char	*ts_sense;			/* sense data on probe device error */
#ifndef	DISCONNECT
	struct sec_smode tsmode;
#endif	DISCONNECT
#define TSMBSZ	31			/* minimum buffer rounding size */

extern	int	tsspintime;

/*
 * tsprobe(ts) - probe procedure 
 *	struct sec_dev *ts;
 *
 * This procedure polls a device with the test unit ready command
 * to determine if the device is present. A device is present if
 * the return status is not a timeout or a serious hardware error.
 *
 * This may need to be changed or bypassed for devices which 
 * need formating "on-the-fly" because a non-formated drive 
 * will not pass this probe.
 *
 * Note: This procedure tracks the header file sec.h
 * which must track the firmware for queue size information.
 * Also the status address MUST be below 4 Meg because the SCSI h/w
 * can't DMA above that.
 *
 * NOTE: must use calloc to get a single request sense buffer
 *	 which isn't going across a 64kb boundary for the firmware.
 */
tsprobe(sed)
	struct sec_probe *sed;
{
	register struct	sec_powerup *iq = sed->secp_desc->sec_powerup;
	register int	i;
	register int	spin;
	static	 int	ts_oneshot = 1;
	int	 stat=0;	/* 
				 *  NOTE: assume address below 0x400000 , 
				 *	mapping not on yet 
				 */

	ASSERT_DEBUG(((int)&stat < 0x400000),"tsprobe: configuration problem - address of \"stat\" greater then 0x400000");

#ifdef TS_DEBUG
	if (ts_debug>3)
		printf("probe: init_q 0x%x, slicid 0x%x stat=%x, device=%d\n", 
		(int)iq, sed->secp_desc->sec_slicaddr, &stat, sed->secp_chan);
#endif TS_DEBUG 
	if (ts_oneshot) {
		ts_oneshot = 0;
		callocrnd(((tssensebuf_sz+TSMBSZ)&~TSMBSZ));	
				/* Round up to test for autoconf bug */
		ts_sense = (u_char *)calloc(((tssensebuf_sz+TSMBSZ)&~TSMBSZ));
	}

	/*
	 * set the device program up (lun=secp_chan&7).
	 */
	ts_devprog.dp_status1 = 0;
	ts_devprog.dp_count = 0;
	ts_devprog.dp_un.dp_data = NULL;
	ts_devprog.dp_next = NULL;
	ts_devprog.dp_cmd_len = 6;
	ts_devprog.dp_data_len = 0;
	ts_devprog.dp_cmd[0] = SCSI_TEST;
	ts_devprog.dp_cmd[1] = (u_char)(sed->secp_unit << SCSI_LUNSHFT); 
	ts_devprog.dp_cmd[2] = 0;
	ts_devprog.dp_cmd[3] = 0;
	ts_devprog.dp_cmd[4] = 0;
	ts_devprog.dp_cmd[5] = 0;

	i = iq->pu_requestq.pq_head;
	iq->pu_requestq.pq_un.pq_progs[i] = &ts_devprog;		
							/* set device prog */
	iq->pu_requestq.pq_head = (i+1) % SEC_POWERUP_QUEUE_SIZE;	
							/* mark in progress */

	SEC_startio(	SINST_STARTIO, 			/* command */
			&stat,				/* return status loc */
			TS_ANYBIN, 			/* bin number on SEC */
			sed->secp_chan,			/* Disk channel # */
			&iq->pu_cib, 			/* device input q loc */
			(u_char)sed->secp_desc->sec_slicaddr	
							/* SEC slic id number */
		);

	/*
	 * ok, scsi command in progress so spin and wait for it to
	 * complete. 
	 * XXX Note: Async timeout issue affect the sanity check below.
	 */
	if (stat == SINST_INSDONE) {
		spin = calc_delay((unsigned int)tsspintime);
		while (iq->pu_doneq.pq_tail == iq->pu_doneq.pq_head) {
			ASSERT(spin-->0,"tsprobe: Async timed out");
                        /*
                         *+ tsprobe timed out while waiting for a
                         *+ command completion status from the
                         *+ SCED.  The SCSI command being executed
                         *+ was Test Unit Ready.
                         */
		}
		iq->pu_doneq.pq_tail = (iq->pu_doneq.pq_tail+1) % 
				SEC_POWERUP_QUEUE_SIZE;	/* tell board "done" */
	} else {
		SEC_startio(	SINST_RESTARTIO,	/* command */
				&stat,			/* return status loc */
				TS_ANYBIN, 		/* bin number on SEC */
				sed->secp_chan,		/* Disk channel # */
				&iq->pu_cib, 		/* device input q loc */
				(u_char)sed->secp_desc->sec_slicaddr	
							/* SEC slic id number */
			);
		iq->pu_requestq.pq_tail = iq->pu_requestq.pq_head = NULL;
		return(SECP_NOTFOUND);		/* probably bus timeout */
	}

	if (ts_devprog.dp_status1 != SEC_ERR_NONE) {

		if (ts_devprog.dp_status1 != SSENSE_CHECK) {
							/* Timeout */
			SEC_startio(	SINST_RESTARTIO,
					&stat,			
					TS_ANYBIN, 		
					sed->secp_chan,	
					&iq->pu_cib, 
					(u_char)sed->secp_desc->sec_slicaddr	
				);
			if (ts_devprog.dp_status1 == SEC_BUSY)
				goto isalive;
			else
				return(SECP_NOTFOUND);
		}

		ts_statb.rs_dev_prog.dp_status1 = 0;
		ts_statb.rs_dev_prog.dp_count = 0;
		ts_statb.rs_dev_prog.dp_un.dp_data = ts_sense;
		ts_statb.rs_dev_prog.dp_data_len = tssensebuf_sz;
		ts_statb.rs_dev_prog.dp_next = NULL;
		ts_statb.rs_dev_prog.dp_cmd_len = 6;
		ts_statb.rs_dev_prog.dp_cmd[0] = SCSI_RSENSE;
		ts_statb.rs_dev_prog.dp_cmd[1] = (u_char)(sed->secp_unit
						<< SCSI_LUNSHFT); 
		ts_statb.rs_dev_prog.dp_cmd[2] = 0;
		ts_statb.rs_dev_prog.dp_cmd[3] = 0;
		ts_statb.rs_dev_prog.dp_cmd[4] = (u_char)tssensebuf_sz;
		ts_statb.rs_dev_prog.dp_cmd[5] = 0;
		ts_statb.rs_status = 0;

		SEC_startio(	SINST_REQUESTSENSE,
				(int *)&ts_statb,	/* cast for lint */
				TS_ANYBIN,
				sed->secp_chan,
				&iq->pu_cib,
				(u_char)sed->secp_desc->sec_slicaddr	
							/* SEC slic id number */
			);
		if (ts_statb.rs_status == SINST_INSDONE) {
			spin = calc_delay((unsigned int)tsspintime);
			while (iq->pu_doneq.pq_tail == iq->pu_doneq.pq_head) {
				ASSERT(spin-->0,"tsprobe: Async timed out on request sense");
                                /*
                                 *+ tsprobe timed out while waiting for a
                                 *+ command completion status from the
                                 *+ SCED.  The SCSI command being executed
                                 *+ was Request Sense.
                                 */
			}
		} else {
			panic("tsprobe: request sense hang!\n");
			/*
			 *+ tsprobe failed its SCSI SINST_REQUESTSENSE
			 *+ command.
			 */
		}
		ts_devprog.dp_status1 = SEC_ERR_NONE;

		SEC_startio(	SINST_RESTARTIO,	/* command */
				&stat,			/* return status loc */
				TS_ANYBIN, 		/* bin number on SEC */
				sed->secp_chan,		/* Disk channel # */
				&iq->pu_cib, 		/* device input q loc */
				(u_char)sed->secp_desc->sec_slicaddr	
							/* SEC slic id number */
			);
		iq->pu_doneq.pq_tail = (iq->pu_doneq.pq_tail+1) % 
			SEC_POWERUP_QUEUE_SIZE;	/* tell board "done" */

		switch (ts_sense[2] & 0xf)	{
		default:
			return(SECP_NOTFOUND);		/* hard error */
		case 0:
		case 6:
		case 2:
			break;
		}
	}

isalive:

	if (tsinquiry(sed) == SECP_NOTFOUND)
		return(SECP_NOTFOUND);

#ifndef DISCONNECT
	tsmode.sm_status = 0;
	tsmode.sm_un.sm_scsi.ssm_timeout = 0;
	tsmode.sm_un.sm_scsi.ssm_flags = 2;
#ifdef	TS_DEBUG
	if (ts_debug >= 1) {
		printf("ts_probe: disabling disconnect\n");
	}
#endif	TS_DEBUG
	SEC_startio(	SINST_SETMODE,
			&tsmode.sm_status,
			TS_ANYBIN, 			/* bin number on SEC */
			sed->secp_chan,			/* Disk channel # */
			&iq->pu_cib, 			/* device input q loc */
			(u_char)sed->secp_desc->sec_slicaddr	
							/* SEC slic id number */
	);
#endif	DISCONNECT
	return(SECP_FOUND|SECP_ONELUN);
}

/*
 * tsboot - initialize all channels of this device driver.
 *
 * Called once after all probing has been done.
 *
 * This procedure initializes and allocates all device driver data
 * structures based off of the configuration information passed in from
 * autoconfig() and from the device drivers binary configuration tables.
 * The boot procedure also maps interrupt levels to unit number by
 * placing the channels communications structure pointer into a
 * major/minor number mapped dynamically allocated array (tsifd[]).
 */
tsboot(ndevs, sed_array)
int ndevs;
struct sec_dev sed_array[];
{
	register struct sec_dev		*sed;
	register struct	ts_info		*ifd;
	register struct	ts_bconf	*bconf;	
					/* Binary configuration info (bc)*/
	int dev;
	int niats;
	extern u_char *SEC_rqinit();

	tsifd = (struct ts_info **)calloc(((ndevs) * (sizeof(struct ts_info *))));
	ts_baselevel = sed_array[0].sd_vector;

	/*
	 * If necessary, scale down the number of devices
	 * being booted to meet minor device number boundaries
	 * and the size of the binary configuration table,
	 * which must contain a corresponding entry per device.
	 */
	if (ndevs > TS_UNITMAX + 1)
		CPRINTF("tsboot: %d device limit exceeded - extras ignored\n",
			ndevs = TS_UNITMAX + 1);

	if (ndevs > tsmaxndevs) {
		CPRINTF("tsboot: %d device binary configuration limit exceeded",
			ndevs = tsmaxndevs);
		CPRINTF(" (from conf_ts.c)\n");
		CPRINTF("\tadditional configured tape drives ignored\n");
	}

	/*
	 * Keep track of the largest accessible
	 * unit as we boot (initially none).
	 */
	tsmaxunit = -1;

	/*
	 * for each configured device
	 */
	for (dev = 0; dev < ndevs; ++dev) {
		sed = &sed_array[dev];
#ifdef DEBUG
		if (ts_debug) {
			printf("tsboot: dev=%d info=%x cib=%x ta=%d un=%d bin=%d slic=%d vec=%d, alive is %s\n",
				dev,
				tsifd,
				sed->sd_cib,
				sed->sd_target,
				sed->sd_unit,
				sed->sd_bin,
				sed->sd_desc->sec_slicaddr,
				sed->sd_vector,
				(sed->sd_alive ? "Found" : "Not Found")
			);
		}
#endif DEBUG
		if (!sed->sd_alive)
			continue;

		bconf = &tsbconf[dev];

		/*
		 * Found the device, continue configuration process.
		 *
		 * Initialize the device structure and
		 * copy pertinent info into it.
		 */

		/*
		 * Info structure.
		 */
		tsifd[dev] = ifd = (struct ts_info *)calloc(sizeof(struct ts_info));
		ifd->ts_unit = dev;
		ifd->ts_desc = sed;
		ifd->ts_lun = sed->sd_unit << SCSI_LUNSHFT;
		ifd->ts_rwbits = bconf->bc_rwbits;
		ifd->ts_cflags = bconf->bc_cflags;
		ifd->ts_bufsz = roundup(bconf->bc_bufsz*1024, CLBYTES);

		/*
		 * allocate ts_info structure, and iats.
		 */

		niats = howmany(ifd->ts_bufsz, DEV_BSIZE);
		ifd->ts_lobuf = (struct ts_iobuf *)
					calloc(sizeof(struct ts_iobuf));
		ifd->ts_lobuf->ts_iats = (struct sec_iat *)
					calloc(niats * sizeof(struct sec_iat));
		ifd->ts_hibuf = (struct ts_iobuf *)
					calloc(sizeof(struct ts_iobuf));
		ifd->ts_hibuf->ts_iats = (struct sec_iat *)
					calloc(niats * sizeof(struct sec_iat));

		/*
		 * If addressable locations over 128M exist, the I/O
		 * buffers should be allocated at boot time to insure
		 * that they will be in low core.
		 */
		
		if (topmem > (caddr_t) MAX_SCED_ADDR_MEM) {
			callocrnd(CLBYTES);

			/*
			 * Deconfigure device if not enough memory.
			 * Should be very rare.
			 */
			if (calloc(0) >
				(caddr_t)(MAX_SCED_ADDR_MEM-2*ifd->ts_bufsz)) {
				CPRINTF(
			     "ts%d: Not enough memory addressable below 128M\n",
					dev);
				return;
			}

			ifd->ts_lobuf->buffer = calloc(ifd->ts_bufsz);
			ifd->ts_hibuf->buffer = calloc(ifd->ts_bufsz);
			tsbigmem = 1;

			/*
			 * Check curmem again, in case there was a hole.
			 * Can't give memory back, there is no "cfree()".
			 * This should be even more rare.
			 */
			if (calloc(0) > (caddr_t)MAX_SCED_ADDR_MEM) {
				CPRINTF(
			     "ts%d: Not enough memory addressable below 128M\n",
					dev);
				return;
			}
		}

		/*
		 * Init locks, semas ...
		 */
		init_lock(&ifd->ts_lock, tsgate);
		init_sema(&ifd->ts_usrsync, 1, 0, tsgate);
		init_sema(&ifd->ts_iosync, 0, 0, tsgate);
		init_sema(&ifd->ts_lobuf->io_wait, 0, 0, tsgate);
		init_sema(&ifd->ts_hibuf->io_wait, 0, 0, tsgate);
		bufinit(&ifd->ts_rbufh, tsgate);

		/*
		 * initialize device queues
		 * Won't fill in the device program addresses at this time.
		 */
		ifd->ts_reqq.sq_progq = sed->sd_requestq;
		ifd->ts_reqq.sq_size = sed->sd_req_size;
		ifd->ts_reqq.sq_progq->pq_head = 0;
		ifd->ts_reqq.sq_progq->pq_tail = 0;

		ifd->ts_doneq.sq_progq = sed->sd_doneq;
		ifd->ts_doneq.sq_size = sed->sd_doneq_size;
		ifd->ts_doneq.sq_progq->pq_head = 0;
		ifd->ts_doneq.sq_progq->pq_tail = 0;

		ifd->ts_genio.dp_un.dp_data = NULL;
		ifd->ts_genio.dp_data_len = 0;
		ifd->ts_genio.dp_next = NULL;
		ifd->ts_genio.dp_cmd_len = 6;

		/*
		 * Request sense information buffer,
		 * one per channel.
		 * Note: Assumes channel can only generate one fault
		 * at a time.
		 */
		callocrnd(tssensebuf_sz);
		ifd->ts_sensebuf = (u_char *) calloc(tssensebuf_sz);

		ifd->ts_sensereq.rs_dev_prog.dp_un.dp_data = 
			SEC_rqinit(ifd->ts_sensebuf, tssensebuf_sz);
		ifd->ts_sensereq.rs_dev_prog.dp_data_len = tssensebuf_sz;
		ifd->ts_sensereq.rs_dev_prog.dp_next = NULL;
		ifd->ts_sensereq.rs_dev_prog.dp_cmd_len = 6;

		ifd->ts_fflags |= TSF_FSTOPEN;

		/*
		 * Boot succeeded on this device.
		 * Update the max accessible unit.
		 */
		tsmaxunit = dev;
	}
#ifdef DEBUG
	printf("ts_debug@0x%x, set to %d\n", &ts_debug, ts_debug);
#endif DEBUG
}

/*
 *	tsopen - open a channel
 *
 *	Check for validity of device.
 *	Set max block size to the entire tape until
 *	an interrupt sets differently.
 *
 *	Tmp assumptions: The raw bufheader can mutex the ts_flags
 *		variable.
 */

/*ARGSUSED*/
tsopen(dev, flags)
	dev_t	dev;
	int	flags;
{
	register struct ts_info	*ifd;
	register int unit = TS_UNIT(dev);

#ifdef DEBUG
	if (ts_debug)
		printf("O");
#endif

	if (unit > tsmaxunit			/* Valid unit number? */
	|| (ifd = tsifd[unit]) == NULL		/* Device available? */
	|| (!ifd->ts_desc->sd_alive)) 		/* Passed probing? */
		return(ENXIO);

	ifd->ts_spl = p_lock(&ifd->ts_lock, TSSPL);
	/* was it closed with wait for rewind? */
	if (ifd->ts_openf == CLOSEDREW) {		
		p_sema_v_lock(&ifd->ts_iosync, PRIBIO, &ifd->ts_lock, 
			ifd->ts_spl);
		/* regain lock for status structure */
		ifd->ts_spl = p_lock(&ifd->ts_lock, TSSPL);
		ifd->ts_cur_mode = GENERAL;
		ifd->ts_fileno = 0;
		ifd->ts_openf = CLOSED;
	}

	if (ifd->ts_openf != CLOSED) {		/* Is it open already? */
		v_lock(&ifd->ts_lock, ifd->ts_spl);
		return(EBUSY);
	}
	ifd->ts_openf = OPEN;

	/*
	 * test unit status
	 */
	if (ts_test_unit(ifd)) {
		ifd->ts_openf = CLOSED;
		v_lock(&ifd->ts_lock, ifd->ts_spl);
		return(EIO);
	}

	if (ifd->ts_fflags & TSF_FSTOPEN) {
		(void) ts_mode_sel(ifd);
		ifd->ts_fflags &= ~TSF_FSTOPEN;
		ifd->ts_fflags | = TSF_ATTEN;
		ifd->ts_fileno = 0;
	}
	if (ifd->ts_fflags & TSF_ATTEN)
		ifd->ts_cur_mode = GENERAL;
	ifd->ts_fflags &= ~(TSF_LSTIOR | TSF_LSTIOW | TSF_FAIL);
	ifd->ts_saved_devp = NULL;
	ifd->ts_blkno = 0;

	/*
	 * Must release lock here because ts_bufinit may block while
	 * allocating memory.
	 */
	v_lock(&ifd->ts_lock, ifd->ts_spl);
	if (ts_bufinit(ifd->ts_lobuf, ifd->ts_bufsz, ((int)ifd->ts_cflags
	& TSC_OPENFAIL))) {
		ifd->ts_openf = CLOSED;
		return(ENOMEM);
	}
	if (ts_bufinit(ifd->ts_hibuf, ifd->ts_bufsz, ((int)ifd->ts_cflags 
	& TSC_OPENFAIL))) {
		ts_buffree(ifd->ts_lobuf, ifd->ts_bufsz);
		ifd->ts_openf = CLOSED;
		return(ENOMEM);
	}
	return(0);				/* Good status */
}


/*
 * tsclose - close the device either with a rewind or without.
 */
tsclose(dev, flag)
	dev_t	dev;
	int	flag;
{
	register int unit = TS_UNIT(dev);
	register struct ts_info	*ifd = tsifd[unit];

#ifdef TS_DEBUG
	if (ts_debug)
		printf("C");
#endif TS_DEBUG

	ifd->ts_spl = p_lock(&ifd->ts_lock, TSSPL);
	if (ts_bflush(ifd))
		u.u_error = EIO;
	if (flag == FWRITE || 
		((flag & FWRITE) && (ifd->ts_fflags & TSF_LSTIOW))) {

		(void) ts_write_fm(ifd, 1);
		if (ifd->ts_fflags & TSF_FAIL) {
			u.u_error = EIO;
			v_lock(&ifd->ts_lock, ifd->ts_spl);
			uprintf("ts%d: tsclose: error writing file mark on close\n",
				unit);
                        /*
                         *+ The SCSI command to write the file mark
                         *+ for the ts driver failed.  This probably occurred,
                         *+ along with an error, while the kernel was
                         *+ attempting to write to the medium.  The
                         *+ medium might have been protected against
                         *+ writes or against an attempt to write while not
                         *+ at the beginning of tape or the logical end of
                         *+ data.
                         */
			ifd->ts_spl = p_lock(&ifd->ts_lock, TSSPL);
		}
	}
	if (TS_REWIND(dev)) {
		(void) ts_rewind(ifd,0);
		ifd->ts_openf = CLOSEDREW;
	} else {
		if (ifd->ts_fflags & TSF_LSTIOR) 
			(void) ts_space(ifd, 1, TS_SPFM);
		ifd->ts_openf = CLOSED;
	}
			
	/*
	 * Release lock early, because ts_buffree can block
	 */
	v_lock(&ifd->ts_lock, ifd->ts_spl);
	ts_buffree(ifd->ts_lobuf, ifd->ts_bufsz);
	ts_buffree(ifd->ts_hibuf, ifd->ts_bufsz);

}

/*
 * ts_bufinit - initialize i/o buffer structures
 */

ts_bufinit(ts, buf_size, openfail)
	register struct ts_iobuf *ts;
	register int buf_size;
	int openfail;
{
	register struct sec_iat *iatp;
	register caddr_t bp;

	ts->io_req.dp_next = NULL;
	ts->io_req.dp_cmd_len = 6;
	ts->state = FREE;
	ts->err_flag = 0;
	ts->nblks = 0;
	ts->start_blk = (daddr_t)0;
	if (!tsbigmem)
		ts->buffer = wmemall(buf_size, (openfail) ? 0 : 1);
	if (ts->buffer == 0)
		return(-1);
	for (iatp = ts->ts_iats, bp = ts->buffer; bp < &ts->buffer[buf_size];
			iatp++, bp += DEV_BSIZE) {
		iatp->iat_data = (u_char *)KVIRTTOPHYS(bp);
		iatp->iat_count = DEV_BSIZE;
	}
	ts->io_req.dp_un.dp_iat = SEC_IATIFY (KVIRTTOPHYS(ts->ts_iats));
	return(0);

}

ts_buffree(ts, buf_size)
	register struct ts_iobuf *ts;
	register int buf_size;
{
	if (!tsbigmem)
		wmemfree(ts->buffer, buf_size);
}

/*
 * tsminphys - correct for too large a request.
 */
tsminphys(bp)
register struct buf *bp;
{
	register struct ts_info *ifd = tsifd[TS_UNIT(bp->b_dev)];

	if (bp->b_bcount > ifd->ts_bufsz)
		bp->b_bcount = ifd->ts_bufsz;
}


/*
 * tswrite	-	Standard raw write procedure.
 */
tswrite(dev, uio)
	dev_t	dev;
	struct uio *uio;
{
	register unit = TS_UNIT(dev);
	register struct ts_info *ifd = tsifd[unit];

#ifdef TS_DEBUG
	ASSERT(ifd != 0, "tswrite ifd zero");
	ASSERT(unit <= tsmaxunit, "tswrite dev");
#endif TS_DEBUG

	 return(physio(tsstrat, (struct buf *)0, dev, B_WRITE, tsminphys, uio)); 
}


/*
 * tsread	-	Standard raw read procedure.
 */
tsread(dev, uio)
	dev_t	dev;
	struct uio *uio;
{
	register unit = TS_UNIT(dev);
	register struct ts_info *ifd = tsifd[unit];
	
#ifdef TS_DEBUG
	ASSERT(ifd != 0, "tswrite ifd zero");
	ASSERT(unit <= tsmaxunit, "tsread dev");
#endif TS_DEBUG
	return(physio(tsstrat, (struct buf *)0, dev, B_READ, tsminphys, uio));
}


/*
 * tsstrat - SCSI strategy routine.
 *	check the block sizes and limits.
 *
 * tmp assumptions:
 *	Only one processor can access the ts_info structure for the
 *	state variable on each devices channel. Interrupts will block
 *	and spin on this lock for each channel. 
 *
 *	Concurrent interrupts on different channels is assumed possible.
 */
tsstrat(bp)
	register struct	buf	*bp;
{
	register int unit = TS_UNIT(bp->b_dev);
	register struct	ts_info	*ifd = tsifd[unit];

	bp->b_resid = 0;		/* must be 0 when done! */
#ifdef	TS_DEBUG
	if (ts_debug >= 3) {
		printf("ts_strat: blkno %x count %x\n", 
			bp->b_blkno, bp->b_bcount);
	}
#endif	TS_DEBUG

	p_sema(&ifd->ts_usrsync, PRIBIO);
	ifd->ts_spl = p_lock(&ifd->ts_lock, TSSPL);

	if (ifd->ts_openf != OPEN) {
#ifdef	TS_DEBUG
		if (ts_debug >= 2) {
			printf("refusing job because ts_openf %x\n",
				ifd->ts_openf);
		}
#endif	TS_DEBUG
		bp->b_resid = bp->b_bcount;
		bp->b_flags |= B_ERROR;
		iodone(bp);
		v_sema(&ifd->ts_usrsync);
		v_lock(&ifd->ts_lock, ifd->ts_spl);
		return;
	}

	if (bp->b_iotype == B_FILIO) {
		bp->b_resid = bp->b_bcount;
		bp->b_flags |= B_ERROR;
		bp->b_error = ENXIO;
	} else {
		if (bp->b_flags & B_READ)
			ts_rtread(ifd, bp);
		else
			ts_rtwrite(ifd, bp);
	}

#ifdef	TS_DEBUG
	if (ts_debug >= 3) {
		printf("tsstrat: I/O completion \n");
	}
#endif	TS_DEBUG
	iodone(bp);
	v_sema(&ifd->ts_usrsync);
	v_lock(&ifd->ts_lock, ifd->ts_spl);
}

ts_bflush(softp)
register struct ts_info *softp;
{
#ifdef	TS_DEBUG
	if (ts_debug >= 2) {
		printf("ts_flush: mode %x \n", softp->ts_cur_mode);
	}
#endif	TS_DEBUG
	switch (softp->ts_cur_mode) {
	case GENERAL:
		break;
	case READ:
		(void) ts_read_wait(softp->ts_lobuf, softp);
		(void) ts_read_wait(softp->ts_hibuf, softp);
		softp->ts_lobuf->state = FREE;
		softp->ts_hibuf->state = FREE;
		break;

	case WRITE:
		if (softp->ts_lobuf->state == VALID) {
			bzero(&softp->ts_lobuf->buffer[softp->ts_curbyte],
				dbtob(softp->ts_lobuf->nblks) -
				softp->ts_curbyte);
			(void) ts_q_write(softp->ts_lobuf, softp);
		}

		(void) ts_write_wait(softp->ts_hibuf, softp);
		(void) ts_write_wait(softp->ts_lobuf, softp);
		softp->ts_lobuf->state = FREE;
		softp->ts_hibuf->state = FREE;
		if (softp->ts_fflags & TSF_FAIL) {
			v_lock(&softp->ts_lock, softp->ts_spl);
			uprintf("ts%d: error writing buffer to tape\n",
				softp->ts_unit);
                        /*
                         *+ The ts driver received a bad command status
                         *+ while attempting to flush its buffered output
                         *+ to tape.  The probable causes are: 
			 *+ a) medium protected against writes,
			 *+ b) medium not written at beginning
                         *+    of tape or logical end of data, 
			 *+ c) invalid medium type for this drive, 
			 *+ d) attempt to write past
                         *+ the physical end of the medium.
                         */
			softp->ts_spl = p_lock(&softp->ts_lock, TSSPL);
			return(-1);
		}
		break;
	}
	return(0);
}

ts_rtread(softp, bp)
register struct ts_info *softp;
register struct buf *bp;
{
	register caddr_t cp;
	register struct ts_iobuf *tp;
	register int n;

	switch (softp->ts_cur_mode) {
	case WRITE:
		v_lock(&softp->ts_lock, softp->ts_spl);
		uprintf("ts%d: illegal read after write\n", softp->ts_unit);
                /*
                 *+ The ts driver does not support reading
                 *+ immediately following a write.  There is
                 *+ no data to read until the tape is rewound.
                 */
		softp->ts_spl = p_lock(&softp->ts_lock, TSSPL);
		bp->b_resid = bp->b_bcount;
		bp->b_error |= B_ERROR;
		return;

	case GENERAL:
		if ((softp->ts_fflags & TSF_ATTEN) && 
			(softp->ts_cflags & TSC_AUTORET))
		{
			(void) ts_retension(softp);
		}
		softp->ts_fflags &= ~TSF_ATTEN;
		softp->ts_cur_mode = READ;

	case READ:
		softp->ts_fflags |= TSF_LSTIOR;
		bp->b_resid = bp->b_bcount;
		cp = bp->b_un.b_addr;

		while ((tp = softp->ts_lobuf)->state == FREE) {
			/*
			 * Prime readahead.  If an error
			 * or pending EOF has occurred the
			 * state will not be FREE.
			 */
			(void) ts_q_read(tp, softp);
			softp->ts_lobuf = softp->ts_hibuf;
			softp->ts_hibuf = tp;
			softp->ts_curbyte = 0;
		}

		while (bp->b_resid) {
			/*
			 * Wait for available read data and
			 * then copy to the caller's buffer.
			 */
			(void) ts_read_wait(tp, softp);

			n = min((u_int)bp->b_resid, 
				(u_int)(dbtob(tp->nblks) - softp->ts_curbyte));

			if (n == 0) {
				/* Device failure or EOF */
				if (tp->err_flag)
					bp->b_flags |= B_ERROR;
				else if (bp->b_resid == bp->b_bcount) {
					/*
					 * Clear EOF indicators and
					 * free up the buffers when
					 * caller gets zero bytes back.
					 */
					softp->ts_fflags &=
						~(TSF_EOF | TSF_LSTIOR);
					softp->ts_blkno = 0;
					tp->state = FREE;       /* lobuf */
					softp->ts_hibuf->state = FREE;
				}       /* Else leave EOF set for next read */
				return;
			}

			v_lock(&softp->ts_lock, softp->ts_spl);
			if (copyout(&tp->buffer[softp->ts_curbyte],
				    cp, (u_int)n)) {
				softp->ts_spl = p_lock(&softp->ts_lock, TSSPL);
				softp->ts_fflags |= TSF_FAIL;
				bp->b_flags |= B_ERROR;
				return;
			}
			softp->ts_spl = p_lock(&softp->ts_lock, TSSPL);
			cp += n;
			bp->b_resid -= n;
			if ((softp->ts_curbyte += n) == softp->ts_bufsz) {
				/*
   				 * Start next readahead.  Note that
				 * exhausted buffers of short read await
				 * EOF or error processing instead.
				 */
				(void) ts_q_read(tp, softp);
				softp->ts_lobuf = softp->ts_hibuf;
				softp->ts_hibuf = tp;
				tp = softp->ts_lobuf;
				softp->ts_curbyte = 0;
			}
		}
	}
}

ts_rtwrite(softp, bp)
register struct ts_info *softp;
register struct buf *bp;
{
	register caddr_t cp;
	register struct ts_iobuf *tp;
	register int n;

	switch (softp->ts_cur_mode) {

	case READ:
		v_lock(&softp->ts_lock, softp->ts_spl);
		uprintf("ts%d: illegal write after read\n", softp->ts_unit);
                /*
                 *+ The ts driver does not support writing
                 *+ immediately after reading.  Writes are
                 *+ supported only from the beginning of the
                 *+ medium (in which case all former data is
                 *+ erased) and at the logical end of data (in which
                 *+ case the new data is appended).
                 */
		softp->ts_spl = p_lock(&softp->ts_lock, TSSPL);
		bp->b_resid = bp->b_bcount;
		bp->b_error |= B_ERROR;
		return;

	case GENERAL:
		if ((softp->ts_fflags & TSF_ATTEN) &&
			(softp->ts_cflags & TSC_AUTORET)) {
			(void) ts_erase(softp);
		}
		softp->ts_fflags &= ~TSF_ATTEN;
		softp->ts_cur_mode = WRITE;

	case WRITE:
		softp->ts_fflags |= TSF_LSTIOW;
		bp->b_resid = bp->b_bcount;
		cp = bp->b_un.b_addr;

		while ((bp->b_resid != 0) || 
			(softp->ts_curbyte == softp->ts_bufsz)) {
			
			tp = softp->ts_lobuf;
			(void) ts_write_wait(tp, softp);

			/*
			 * In the sequence that follows, note that
			 * when EOM is set in ts_fflags, but FAIL
			 * is not, the early warning for end-of-media
			 * has been encountered.  The user is notified
			 * by terminating a write with less than the
			 * requested amount of data written, upon which
			 * the EOF flag is set indicating notification
			 * has been given.  Subsequent attempts to
			 * write without resetting media result in an
			 * error being reported.
			 */

			if (softp->ts_fflags & TSF_FAIL) {
				bp->b_flags |= B_ERROR;
				softp->ts_openf = ERR;
				return;
			} else if (softp->ts_fflags & TSF_EOM) {
				if (softp->ts_fflags & TSF_EOF) {
					bp->b_error = ENOSPC;
					bp->b_flags |= B_ERROR;
					return;
				} else if (bp->b_resid)
					softp->ts_fflags |= TSF_EOF;
				return;
			}

			if (tp->state == FREE) {
				tp->nblks = 0;
				tp->start_blk = softp->ts_blkno;
				tp->state = VALID;
				tp->err_flag = 0;
				softp->ts_curbyte = 0;
				continue;
			}

			n = min((u_int)bp->b_resid, 
				(u_int)(softp->ts_bufsz - softp->ts_curbyte));

			if (n == 0) {
				if (softp->ts_curbyte == softp->ts_bufsz) {
					(void) ts_q_write(tp, softp);
					softp->ts_lobuf = softp->ts_hibuf;
					softp->ts_hibuf = tp;
					continue;
				}
				printf("ts%d: unexpected zero byte write\n",
					softp->ts_unit);
                                /*
                                 *+ This is an internal ts driver error.
                                 *+ An attempt was made to start a transfer, but
                                 *+ no bytes were remaining.
                                 */
			}

			v_lock(&softp->ts_lock, softp->ts_spl);
			if (copyin(cp, &tp->buffer[softp->ts_curbyte], (u_int)n))
			{
				softp->ts_spl = p_lock(&softp->ts_lock, TSSPL);
				softp->ts_fflags |= TSF_FAIL;
				continue;
			}
			softp->ts_spl = p_lock(&softp->ts_lock, TSSPL);
			cp += n;
			bp->b_resid -= n;
			softp->ts_curbyte += n;
			tp->nblks = btodb(softp->ts_curbyte + (dbtob(1) - 1));
		}
	}
}


/*
 * These are to gather performance information
 */

#define	NSLOTS	4
int ts_stats[NSLOTS + 1];
int ts_strcount;

tsintr(level)
int level;
{
	register struct ts_info *softp;
	register struct sec_dev_prog *devp;
	register struct ts_iobuf *ts;
	struct sec_dev_prog *tdevp;
	int residue;
	char *errmsg;
	int command;
	struct sec_progq *sq;
	u_char *rs;
	int sensekey;
	int len;
	u_char *cp;
	bool_t early_warning = 0;

#ifdef	TS_DEBUG
	if (ts_debug >= 3) {
		printf("tsintr:\n");
	}
#endif	TS_DEBUG

	level -= ts_baselevel;
	softp = tsifd[level];

	softp->ts_spl = p_lock(&softp->ts_lock, TSSPL);

	while (!TS_TEST_Q_EMPTY(&softp->ts_doneq)) {
		
		devp = TS_GET_Q_TAIL(&softp->ts_doneq);
		TS_INCR_Q_TAIL(&softp->ts_doneq);
#ifdef	TS_DEBUG
		if (ts_debug >= 3) {
			printf("tsintr: job status %x\n", devp->dp_status1);
		}
#endif	TS_DEBUG

		switch (devp->dp_status1) {

		case (SEC_BUSY | SEC_CHECK):
			softp->ts_fflags |= TSF_FAIL;
			errmsg = "Busy and check status";
			break;

		case SEC_BUSY:
			softp->ts_fflags |= TSF_FAIL;
			errmsg = "Device busy";
			break;

		case SEC_FW_ERR:
			softp->ts_fflags |= TSF_FAIL;
			errmsg = "SCED firmware error";
			break;

		case SEC_CHECK:
			softp->ts_saved_devp = devp;
			(void) ts_request_sense(softp);
			v_lock(&softp->ts_lock, softp->ts_spl);
			return;

		case SEC_ERR_NONE:
			errmsg = NULL;
			break;

		default:
			softp->ts_fflags |= TSF_FAIL;
			printf("ts%d: 0x%x\n", level, devp->dp_status1);
                        /*
                         *+ The ts driver received an unknown command
                         *+ termination status from the SCED.  This can
                         *+ indicate that the SCED firmware is in an 
			 *+ inconsistent state.
                         */
			errmsg = "Unknown program error";
			break;
		}

		residue = -1;
		if (devp->dp_cmd[0] == SCSI_RSENSE) {

			rs = softp->ts_sensebuf;
			devp = softp->ts_saved_devp;

			if ((rs[0] & SENSE_ECLASS) == SENSE_ECLASS7) {
				if (rs[0] & SENSE_VALID) {
					residue = (u_char)rs[3]<<24;
					residue |= (u_char)rs[4]<<16;
					residue |= (u_char)rs[5]<<8;
					residue |= (u_char)rs[6];
				}
	
				sensekey = rs[2] & SENSE_KEY;
				switch (sensekey) {
	
				case SSENSE_NOSENSE:
				case SSENSE_RECOVERABLE:
					if (rs[2] & SENSE_EOM) {
						if (softp->ts_fflags & TSF_LSTIOW) {
							softp->ts_fflags |= 
								TSF_EOM;
							early_warning = 1;
						} else {
							softp->ts_fflags |= 
								TSF_FAIL |
								TSF_EOM | 
								TSF_EOF;
						}
					} else if (rs[2] & SENSE_FM) {
						softp->ts_fflags |=
							TSF_FAIL | TSF_EOF;
					}
					break;
	
				case SSENSE_BLANK_CHK:
					softp->ts_fflags |= 
						TSF_FAIL | TSF_EOM | TSF_EOF;
					break;
	
				case SSENSE_UNIT_ATN:
					softp->ts_fflags |= 
							TSF_ATTEN | TSF_FAIL;
					break;
	
				case SSENSE_NOT_READY:
				case SSENSE_ILL_REQ:
				case SSENSE_DATA_PROT:
				case SSENSE_ABORT:
				case SSENSE_VOL_OVER:
				case SSENSE_MEDIA_ERR:
				case SSENSE_HARD_ERR:
				default:
					if (rs[2] & SENSE_EOM)
						softp->ts_fflags |= TSF_FAIL |
							TSF_EOM | TSF_EOF;
					else if (rs[2] & SENSE_FM)
						softp->ts_fflags |=
							TSF_FAIL | TSF_EOF;
					else
						softp->ts_fflags |= TSF_FAIL;
				}
				len = 8 + rs[7];
			} else {
				softp->ts_fflags |= TSF_FAIL;
				sensekey = 0x10;
				len = 4;
			}

			errmsg = ts_errors[sensekey].data;
			command = devp->dp_cmd[0];
			if (((softp->ts_cflags & TSC_RWS_SENSE) &&
				(errmsg != NULL) &&
				((command == SCSI_READ) || 
				(command == SCSI_WRITE) ||
				(command == SCSI_SPACE))) ||
			(softp->ts_cflags & TSC_PRSENSE) ||
			ts_errors[sensekey].prntflag) {

				if (errmsg != NULL) {
					printf("ts%d: %s on command %s\n", 
						level, errmsg, ts_cmd[command]);
					/*
					 *+ A SCSI command issued by the ts 
					 *+ driver had an error termination.
					 *+ A summary of the SCSI command being
					 *+ executed and the reason for
					 *+ the SCSI termination has been 
					 *+ displayed. For more information, 
					 *+ look up the specific termination 
					 *+ message and/or command.
					 */
				}
				if (len >= 11) {
					CPRINTF("recoverable errors: %d\n",
						((rs[9] << 8) | rs[10]));
				}
				CPRINTF("sense buf:");
				for (cp = rs; cp < &rs[len]; cp++)
					CPRINTF("%x ", *cp);
				CPRINTF("\n");
				CPRINTF("cmd buf:");
				for (cp = devp->dp_cmd;
					cp <= &devp->dp_cmd[5]; cp++)
					CPRINTF("%x ", *cp);
				CPRINTF("\n");
			}
		}

		softp->ts_resid = residue;

		if (softp->ts_fflags & TSF_FAIL) {
			TS_INCR_Q_TAIL(&softp->ts_reqq);
			if (!TS_TEST_Q_EMPTY(&softp->ts_reqq)) {
				tdevp = TS_GET_Q_TAIL(&softp->ts_reqq);
				TS_INCR_Q_TAIL(&softp->ts_reqq);

				ts = softp->ts_lobuf;
				if (tdevp != &ts->io_req)
					ts = softp->ts_hibuf;

				ts->state = IODONE;
				ts->nblks = 0;
				ts->err_flag = -1;
				v_sema(&ts->io_wait);
			}
		}
		if (devp->dp_status1 != SEC_ERR_NONE) {
			if (early_warning && residue > 0) {
				/* Adjust and continue the current request */

				ASSERT_DEBUG(btodb(devp->dp_data_len) >= residue,
					"ts_intr: residue exceeds data len");
				
				devp->dp_un.dp_iat +=
					btodb(devp->dp_data_len) - residue;
				devp->dp_data_len = dbtob(residue);
				devp->dp_cmd[2] = (u_char)residue >> 16;
				devp->dp_cmd[3] = (u_char)residue >> 8;
				devp->dp_cmd[4] = (u_char)residue;
				devp->dp_status1 = 0;
				ts_restartio(softp, SINST_RESTARTCURRENTIO);
				v_lock(&softp->ts_lock, softp->ts_spl);
				return;
			} else
				/* Start next request */
				ts_restartio(softp, SINST_RESTARTIO);
		}

		command = devp->dp_cmd[0];
		if ((softp->ts_fflags & TSF_FAIL) && errmsg != NULL) {
			printf("ts%d: %s on command %s\n",
				level, errmsg, ts_cmd[command]);
			/*
			 *+ A SCSI command issued by the ts 
			 *+ driver had an error termination.
			 *+ A summary of the SCSI command being
			 *+ executed and the reason for
			 *+ the SCSI termination has been 
			 *+ displayed. For more information, 
			 *+ look up the specific termination 
			 *+ message and/or command.
			 */
		}

		switch (command) {
		case SCSI_SPACE:
			if ((softp->ts_fflags & TSF_FAIL) == 0) {
				softp->ts_nspace -= softp->ts_spvalue;
				if (softp->ts_nspace != 0) {
					ts_do_space(softp);
					v_lock(&softp->ts_lock, softp->ts_spl);
					return;
				}
			}

		case SCSI_MODES:
		case SCSI_ERASE:
		case SCSI_WFM:
		case SCSI_REWIND:
		case SCSI_STARTOP:
		default:
			v_sema(&softp->ts_iosync);
			break;

		case SCSI_TEST:
			if (softp->ts_fflags & TSF_ATTEN)
				softp->ts_fflags &= ~TSF_FAIL;
			v_sema(&softp->ts_iosync);
			break;

		case SCSI_READ:
		case SCSI_WRITE:

			ts = softp->ts_lobuf;
			if (devp != &ts->io_req)
				ts = softp->ts_hibuf;
			
			if (softp->ts_fflags & TSF_FAIL) {
				if (residue == -1)
					ts->nblks = 0;
				else {
					ts->nblks -= residue;

					/*
					 * This is to work around a
					 * in the Emulex firmware
					 */

					if (ts->nblks < 0)
						ts->nblks = 0;
				}
				softp->ts_blkno = ts->start_blk + ts->nblks;
				if (softp->ts_fflags & TSF_EOF) {
					ts->err_flag = 0;
					softp->ts_fflags &= ~TSF_FAIL;
				} else
					ts->err_flag = 1;
			}

			sq = softp->ts_reqq.sq_progq;
			if (sq->pq_tail != sq->pq_head)
				ts_strcount++;
			else {
				if (ts_strcount > NSLOTS)
					ts_stats[NSLOTS]++;
				else
					ts_stats[ts_strcount]++;
				ts_strcount = 0;
			}

			ts->state = IODONE;
			v_sema(&ts->io_wait);
		}
	}
	v_lock(&softp->ts_lock, softp->ts_spl);
}

/*
 * tsioctl - asynchronous io control requests of the driver.
 */
tsioctl(dev, cmd, data)
	int	cmd;
	dev_t	dev;
	caddr_t data;
{
	register int unit = TS_UNIT(dev);
	register struct ts_info *ifd = tsifd[unit];
	register struct mtop *mtop;
	struct	mtget	*mtget;
	int	 error, opcount;

#ifdef DEBUG
	if (ts_debug)
		printf("b");
#endif DEBUG

	ASSERT(ifd != 0, "tsioctl: ifd == 0");
        /*
         *+ The ts driver received an ioctl request
         *+ for a drive that doesn't exist.  tsioctl
         *+ should be called only when a device is
         *+ open.
         */

	ifd->ts_spl = p_lock(&ifd->ts_lock, TSSPL);
	switch (cmd) {

	case MTIOCTOP:	/* tape operation */
		mtop = (struct mtop *)data;
		opcount = mtop->mt_count;

		if ((opcount < 0) || (opcount > 0x7ff)) {
			v_lock(&ifd->ts_lock, ifd->ts_spl);
			return(EINVAL);
		}

		error = 0;
		switch (ifd->ts_cur_mode) {
		case READ:
			if (mtop->mt_op == MTWEOF) {
				v_lock(&ifd->ts_lock, ifd->ts_spl);
				uprintf(
				  "ts%d: tscioctl: can't write eof after read\n",
				   unit);
                                /*
                                 *+ When in read mode, the ts driver does 
				 *+ not su pport writing an EOF mark.
                                 */
				return(EINVAL);
			}
			break;

		case WRITE:
			switch (mtop->mt_op) {
			case MTFSF:
			case MTFSR:
			case MTSEOD:
				v_lock(&ifd->ts_lock, ifd->ts_spl);
				uprintf(
				   "ts%d: tsioctl: bad operation after write\n",
				    unit);
                                /*
                                 *+ When in write mode, the ts driver does 
				 *+ not support the attempted operation.
                                 */
				return(EINVAL);
			default:
				break;
			}
			break;

		case GENERAL:
			if ((ifd->ts_fflags & TSF_ATTEN) &&
				(ifd->ts_cflags & TSC_AUTORET) &&
				(mtop->mt_op !=  MTERASE) && 
				(mtop->mt_op != MTRET) &&
				(mtop->mt_op != MTNORET)){
				(void) ts_retension(ifd);
			}
			ifd->ts_fflags &= ~TSF_ATTEN;
			break;
		}
		if (ts_bflush(ifd)) {
			ifd->ts_openf = ERR;
			v_lock(&ifd->ts_lock, ifd->ts_spl);
			return(EIO);
		}
			
			
		ifd->ts_fflags &= ~(TSF_LSTIOR | TSF_LSTIOW);
		switch (mtop->mt_op) {
		case MTNOP:
			break;
		case MTFSF: 
			error = ts_space(ifd, opcount, TS_SPFM);
			break;
		case MTWEOF:
			error = ts_write_fm(ifd, opcount);
			break;
		case MTFSR: 
			v_lock(&ifd->ts_lock, ifd->ts_spl);
			return(EINVAL);
		case MTBSR:
			v_lock(&ifd->ts_lock, ifd->ts_spl);
			return(EINVAL);
		case MTSEOD:
			error = ts_space(ifd, 0, TS_SPEOD);
			break;
		case MTREW:
			error = ts_rewind(ifd,1);
			break;
		case MTERASE:
			error = ts_erase(ifd);
			break;
		case MTOFFL: 
			v_lock(&ifd->ts_lock, ifd->ts_spl);
			return(EINVAL);
		case MTBSF:
			v_lock(&ifd->ts_lock, ifd->ts_spl);
			return(EINVAL); 
		case MTRET:
			error = ts_retension(ifd);
			break;
		case MTNORET:
			ifd->ts_fflags &= ~TSF_ATTEN;
			error = 0;
			break;
		default:
			v_lock(&ifd->ts_lock, ifd->ts_spl);
			return (EINVAL);
		}

		v_lock(&ifd->ts_lock, ifd->ts_spl);
		if (error)
			return(EIO);
		else 
			return (0);

	case MTIOCGET:
		mtget = (struct mtget *)data;
		mtget->mt_dsreg = ifd->ts_status & 0xffff;
		mtget->mt_erreg = ifd->ts_sensebuf[2] & SENSE_KEY;
		mtget->mt_resid = ifd->ts_resid;
		mtget->mt_fileno = ifd->ts_fileno;
		mtget->mt_blkno = ifd->ts_blkno;
		mtget->mt_type = MT_ISTS;
		v_lock(&ifd->ts_lock, ifd->ts_spl);
		return (0);

	default:
		v_lock(&ifd->ts_lock, ifd->ts_spl);
		return (EINVAL);
	}
}

ts_q_write(ts, softp)
register struct ts_iobuf *ts;
register struct ts_info *softp;
{
#ifdef	TS_DEBUG
	if (ts_debug >= 2) {
		printf("ts_q_write: startblk %x nblks %x fflag %x state %x\n",
			ts->start_blk, ts->nblks, softp->ts_fflags, ts->state);
	}
#endif	TS_DEBUG

	if (ts->state != VALID) {
		return(-1);
	}

	if (softp->ts_fflags & TSF_FAIL) {
		ts->state = FREE;
		return(0);
	}
	softp->ts_blkno += ts->nblks;

	ts->state = WIP;
	ts->err_flag = 0;
	ts->io_req.dp_un.dp_iat = SEC_IATIFY (KVIRTTOPHYS(ts->ts_iats));
	ts->io_req.dp_data_len = dbtob(ts->nblks);
	ts->io_req.dp_cmd[0] = SCSI_WRITE;
	ts->io_req.dp_cmd[1] = softp->ts_lun | softp->ts_rwbits;
	ts->io_req.dp_cmd[2] = (u_char)(ts->nblks >> 16);
	ts->io_req.dp_cmd[3] = (u_char)(ts->nblks >> 8);
	ts->io_req.dp_cmd[4] = (u_char)(ts->nblks);
	ts->io_req.dp_status1 = 0;

	TIODONE(&ts->io_wait) = 0;
	return(ts_startio(softp, &ts->io_req));
}


ts_q_read(ts, softp)
register struct ts_iobuf *ts;
register struct ts_info *softp;
{
#ifdef	TS_DEBUG
	if (ts_debug >= 2) {
		printf("ts_q_read: state %x forcefail %x blkno %x\n",
			ts->state, softp->ts_fflags, softp->ts_blkno);
	}
#endif	TS_DEBUG
	if ((ts->state != VALID) && (ts->state != FREE)) {
		return(-1);
	}

	if (softp->ts_fflags & (TSF_EOM | TSF_EOF | TSF_FAIL)) {
		ts->nblks = 0;
		ts->start_blk = softp->ts_blkno;
		ts->state = VALID;
		ts->err_flag = (softp->ts_fflags & TSF_EOF) ? 0 : 1;
		return(0);
	}
	ts->nblks = btodb(softp->ts_bufsz);
	ts->start_blk = softp->ts_blkno;
	softp->ts_blkno += ts->nblks;
#ifdef	TS_DEBUG
	if (ts_debug >= 2) {
		printf("ts_q_read: startblk %x, nblks %x\n",
			ts->start_blk, ts->nblks);
	}
#endif	TS_DEBUG

	ts->state = RIP;
	ts->err_flag = 0;
	ts->io_req.dp_un.dp_iat = SEC_IATIFY (KVIRTTOPHYS(ts->ts_iats));
	ts->io_req.dp_data_len = dbtob(ts->nblks);
	ts->io_req.dp_cmd[0] = SCSI_READ;
	ts->io_req.dp_cmd[1] = softp->ts_lun | softp->ts_rwbits;
	ts->io_req.dp_cmd[2] = (u_char)(ts->nblks >> 16);
	ts->io_req.dp_cmd[3] = (u_char)(ts->nblks >> 8);
	ts->io_req.dp_cmd[4] = (u_char)(ts->nblks);
	ts->io_req.dp_status1 = 0;

	TIODONE(&ts->io_wait) = 0;
	return(ts_startio(softp, &ts->io_req));
}

ts_read_wait(tp, softp)
register struct ts_iobuf *tp;
register struct ts_info *softp;
{
#ifdef	TS_DEBUG
	if (ts_debug >= 3) {
		printf("ts_read_wait: startblk %x, nblks %x state %x\n", 
			tp->start_blk, tp->nblks, tp->state);
	}
#endif	TS_DEBUG

	switch (tp->state) {

	case VALID:
	case FREE:
		return(0);

	case WIP:
		printf("ts%d: ts_read_wait; state is WIP\n", softp->ts_unit);
                /*
                 *+ The internal state of the ts driver is confused.
                 *+ It is trying to wait for a read, but a
                 *+ write is in progress.
                 */
		return(-1);

	case RIP:
#ifdef	TS_DEBUG
		if (ts_debug >= 2) {
			printf("ts_read_wait: block \n");
		}
#endif	TS_DEBUG
		p_sema_v_lock(&tp->io_wait, PRIBIO, &softp->ts_lock, 
			softp->ts_spl);
		softp->ts_spl = p_lock(&softp->ts_lock, TSSPL);
#ifdef	TS_DEBUG
		if (ts_debug >= 2) {
			printf("ts_read_wait: unblock \n");
		}
#endif	TS_DEBUG
		break;

	case IODONE:
#ifdef	TS_DEBUG
		if (ts_debug >= 2) {
			printf("ts_read_wait: IODONE \n");
		}
#endif	TS_DEBUG
		break;

	default:
		printf("ts_read_wait: unknown state!!\n");
                /*
                 *+ The ts driver is in a confused state.
                 */
	}

	if (tp->state != IODONE) {
		printf("ts_read_wait: bad state\n");
                /*
                 *+ The ts driver is in an unexpected state
                 *+ while waiting for a read to complete.
                 */
		return(-1);
	}

	tp->state = VALID;
	return(0);
}

ts_write_wait(tp, softp)
register struct ts_iobuf *tp;
register struct ts_info *softp;

{
#ifdef	TS_DEBUG
	if (ts_debug >= 3) {
		printf("ts_write_wait: startblk %x, size %x\n", tp->start_blk, tp->nblks);
	}
#endif	TS_DEBUG
	switch (tp->state) {

	case VALID:
	case FREE:
		return(0);

	case RIP:
		printf("ts%d: ts_write_wait: RIP or VALID state\n",
			softp->ts_unit);
                /*
                 *+ The ts driver is in an invalid state.
                 *+ It is trying to wait for a write to
                 *+ complete, but a read is in progress.
                 */
		return(-1);

	case WIP:
#ifdef	TS_DEBUG
		if (ts_debug >= 2) {
			printf("ts_write_wait: blocking\n");
		}
#endif	TS_DEBUG
		p_sema_v_lock(&tp->io_wait, PRIBIO, &softp->ts_lock, 
			softp->ts_spl);
		softp->ts_spl = p_lock(&softp->ts_lock, TSSPL);
#ifdef	TS_DEBUG
		if (ts_debug >= 2) {
			printf("ts_write_wait: unblocking\n");
		}
#endif	TS_DEBUG
		break;

	case IODONE:
#ifdef	TS_DEBUG
		if (ts_debug >= 2) {
			printf("ts_write_wait: iodone\n");
		}
#endif	TS_DEBUG
		break;

	default:
		printf("ts_write_wait: invalid state\n");
                /*
                 *+ The ts driver is in an invalid state.
                 */
		return(-1);
	}

	if (tp->state != IODONE) {
		printf("ts_write_wait: bad state\n");
                /*
                 *+ The ts driver is in an invalid state.
                 */
		return(-1);
	}

	tp->state = FREE;
	return(0);
}

ts_space(softp, opcnt, type)
register struct ts_info *softp;
register int opcnt;
register int type;
{

#ifdef	TS_DEBUG
	if (ts_debug >= 1) {
		printf("ts_space: opcnt %x, type %x\n", opcnt, type);
	}
#endif	TS_DEBUG

	if ((type == TS_SPFM) && (softp->ts_fflags & (TSF_EOF | TSF_EOM))) {
		if (softp->ts_fflags & TSF_EOM)
			return(0);
		else {
			opcnt--;
			softp->ts_fflags &= ~(TSF_EOF | TSF_FAIL);
			softp->ts_fileno += 1;
		}
	}

	if ((opcnt <= 0) && (type == TS_SPFM))
		return(0);

	softp->ts_fflags &= ~(TSF_FAIL);
	softp->ts_nspace = opcnt;
	softp->ts_sptype = type;

	TIODONE(&softp->ts_iosync) = 0;
	ts_do_space(softp);
	p_sema_v_lock(&softp->ts_iosync, PRIBIO, &softp->ts_lock,
		softp->ts_spl);

	softp->ts_spl = p_lock(&softp->ts_lock, TSSPL);
	if (softp->ts_fflags & TSF_FAIL)
		return(-1);
	if (type == TS_SPEOD) {
		softp->ts_cur_mode = GENERAL;
		softp->ts_fflags |= TSF_EOF;
		softp->ts_fileno = -1;
	} else {
		softp->ts_cur_mode = READ;
		softp->ts_fileno += softp->ts_spvalue;
	}
	return(0);
}

ts_do_space(softp)
register struct ts_info *softp;
{

	register struct sec_dev_prog *devp;

	if (softp->ts_nspace < -128)
		softp->ts_spvalue = -128;
	else if (softp->ts_nspace >  127)
		softp->ts_spvalue = 127;
	else
		softp->ts_spvalue = softp->ts_nspace;

#ifdef	TS_DEBUG
	if (ts_debug >= 1) {
		printf("ts_do_space: spvalue %x\n", softp->ts_spvalue);
	}
#endif	TS_DEBUG

	devp = &softp->ts_genio;

	devp->dp_status1 = 0;
	devp->dp_count = 0;
	devp->dp_un.dp_data = NULL;
	devp->dp_data_len = 0;
	devp->dp_cmd[0] = SCSI_SPACE;
	devp->dp_cmd[1] = softp->ts_lun | softp->ts_sptype;
	devp->dp_cmd[2] = 0;
	devp->dp_cmd[3] = 0;
	devp->dp_cmd[4] = (u_char)softp->ts_spvalue;
	devp->dp_cmd[5] = 0;

	(void) ts_startio(softp, devp);
}

ts_erase(softp)
register struct ts_info *softp;
{
	register struct sec_dev_prog *devp;

#ifdef	TS_DEBUG
	if (ts_debug >= 1) {
		printf("ts_erase\n");
	}
#endif	TS_DEBUG

	softp->ts_fflags &= ~(TSF_FAIL | TSF_EOM | TSF_EOF);
	devp = &softp->ts_genio;
	TIODONE(&softp->ts_iosync) = 0;
	devp->dp_status1 = 0;
	devp->dp_count = 0;
	devp->dp_un.dp_data = NULL;
	devp->dp_data_len = 0;
	devp->dp_cmd[0] = SCSI_ERASE;
	devp->dp_cmd[1] = softp->ts_lun | softp->ts_rwbits;
	devp->dp_cmd[2] = 0;
	devp->dp_cmd[3] = 0;
	devp->dp_cmd[4] = 0;
	devp->dp_cmd[5] = 0;

	(void) ts_startio(softp, devp);

	p_sema_v_lock(&softp->ts_iosync, PRIBIO, &softp->ts_lock,
		softp->ts_spl);

	softp->ts_spl = p_lock(&softp->ts_lock, TSSPL);
	if (softp->ts_fflags & TSF_FAIL ) 
		return(-1);
	softp->ts_cur_mode = GENERAL;
	return(0);
}

ts_rewind(softp,wait)
register struct ts_info *softp;
int wait;
{
	register struct sec_dev_prog *devp;

#ifdef	TS_DEBUG
	if (ts_debug >= 1) {
		printf("ts_rewind\n");
	}
#endif	TS_DEBUG

	softp->ts_fflags &= ~(TSF_FAIL | TSF_EOM | TSF_EOF);
	devp = &softp->ts_genio;
	TIODONE(&softp->ts_iosync) = 0;
	devp->dp_status1 = 0;
	devp->dp_count = 0;
	devp->dp_un.dp_data = NULL;
	devp->dp_data_len = 0;
	devp->dp_cmd[0] = SCSI_REWIND;
	devp->dp_cmd[1] = softp->ts_lun;
	devp->dp_cmd[2] = 0;
	devp->dp_cmd[3] = 0;
	devp->dp_cmd[4] = 0;
	devp->dp_cmd[5] = 0;

	(void) ts_startio(softp, devp);

	if (wait == 0) {
		return(0);
	} else {
		p_sema_v_lock(&softp->ts_iosync, PRIBIO, &softp->ts_lock,
			softp->ts_spl);
	}

	softp->ts_spl = p_lock(&softp->ts_lock, TSSPL);
	if (softp->ts_fflags & TSF_FAIL)
		return(-1);
	softp->ts_cur_mode = GENERAL;
	softp->ts_fileno = 0;
	return(0);
}

ts_retension(softp)
register struct ts_info *softp;
{
	register struct sec_dev_prog *devp;

#ifdef	TS_DEBUG
	if (ts_debug >= 1) {
		printf("ts_retension\n");
	}
#endif	TS_DEBUG

	softp->ts_fflags &= ~(TSF_FAIL | TSF_EOM | TSF_EOF);
	devp = &softp->ts_genio;
	TIODONE(&softp->ts_iosync) = 0;
	devp->dp_status1 = 0;
	devp->dp_count = 0;
	devp->dp_un.dp_data = NULL;
	devp->dp_data_len = 0;
	devp->dp_cmd[0] = SCSI_STARTOP;
	devp->dp_cmd[1] = softp->ts_lun;
	devp->dp_cmd[2] = 0;
	devp->dp_cmd[3] = 0;
	devp->dp_cmd[4] = 3;
	devp->dp_cmd[5] = 0;

	(void) ts_startio(softp, devp);

	p_sema_v_lock(&softp->ts_iosync, PRIBIO, &softp->ts_lock,
		softp->ts_spl);

	softp->ts_spl = p_lock(&softp->ts_lock, TSSPL);
	if (softp->ts_fflags & TSF_FAIL)
		return(-1);
	softp->ts_cur_mode = GENERAL;
	return(0);
}

ts_test_unit(softp)
register struct ts_info *softp;
{
	register struct sec_dev_prog *devp;
	int	saved_atten;

#ifdef	TS_DEBUG
	if (ts_debug >= 1) {
		printf("ts_test_unit\n");
	}
#endif	TS_DEBUG

	saved_atten = softp->ts_fflags & TSF_ATTEN;
	softp->ts_fflags &= ~(TSF_FAIL | TSF_ATTEN);
	devp = &softp->ts_genio;
	TIODONE(&softp->ts_iosync) = 0;
	devp->dp_status1 = 0;
	devp->dp_count = 0;
	devp->dp_un.dp_data = NULL;
	devp->dp_data_len = 0;
	devp->dp_cmd[0] = SCSI_TEST;
	devp->dp_cmd[1] = softp->ts_lun;
	devp->dp_cmd[2] = 0;
	devp->dp_cmd[3] = 0;
	devp->dp_cmd[4] = 0;
	devp->dp_cmd[5] = 0;

	(void) ts_startio(softp, devp);

	p_sema_v_lock(&softp->ts_iosync, PRIBIO, &softp->ts_lock,
		softp->ts_spl);

	softp->ts_spl = p_lock(&softp->ts_lock, TSSPL);
	softp->ts_fflags |= saved_atten;
	if (softp->ts_fflags & TSF_ATTEN) {
		softp->ts_fflags & = ~(TSF_EOF | TSF_EOM);
	}
	if (softp->ts_fflags & TSF_FAIL)
		return(-1);
	return(0);
}

ts_write_fm(softp, opcnt)
register struct ts_info *softp;
register int opcnt;
{
	register struct sec_dev_prog *devp;

#ifdef	TS_DEBUG
	if (ts_debug >= 1) {
		printf("ts_write_fm opcnt %x\n", opcnt);
	}
#endif	TS_DEBUG

	softp->ts_fflags &= ~TSF_FAIL;
	devp = &softp->ts_genio;
	TIODONE(&softp->ts_iosync) = 0;
	devp->dp_status1 = 0;
	devp->dp_count = 0;
	devp->dp_un.dp_data = NULL;
	devp->dp_data_len = 0;
	devp->dp_cmd[0] = SCSI_WFM;
	devp->dp_cmd[1] = softp->ts_lun;
	devp->dp_cmd[2] = (u_char)(opcnt >> 16);
	devp->dp_cmd[3] = (u_char)(opcnt >> 8);
	devp->dp_cmd[4] = (u_char)(opcnt);
	devp->dp_cmd[5] = 0;

	(void) ts_startio(softp, devp);

	p_sema_v_lock(&softp->ts_iosync, PRIBIO, &softp->ts_lock,
		softp->ts_spl);

	softp->ts_spl = p_lock(&softp->ts_lock, TSSPL);
	if (softp->ts_fflags & TSF_FAIL)
		return(-1);
	softp->ts_cur_mode = WRITE;
	return(0);
}

u_char ts_density = 0;

ts_mode_sel(softp)
register struct ts_info *softp;
{
	register struct sec_dev_prog *devp;

#ifdef	TS_DEBUG
	if (ts_debug >= 1) {
		printf("ts_mode_sel\n");
	}
#endif	TS_DEBUG

retry:
	softp->ts_fflags &= ~TSF_FAIL;
	bzero((char *)softp->ts_sensebuf, (u_int)tssensebuf_sz);
	softp->ts_sensebuf[2] = 0x10;
	softp->ts_sensebuf[3] = 0x8;
	softp->ts_sensebuf[4] = ts_density;
	softp->ts_sensebuf[10] = 0x2;
	devp = &softp->ts_genio;
	TIODONE(&softp->ts_iosync) = 0;
	devp->dp_status1 = 0;
	devp->dp_count = 0;
	devp->dp_un.dp_data = softp->ts_sensebuf;
	devp->dp_data_len = 0xc;
	devp->dp_cmd[0] = SCSI_MODES;
	devp->dp_cmd[1] = softp->ts_lun;
	devp->dp_cmd[2] = 0;
	devp->dp_cmd[3] = 0;
	devp->dp_cmd[4] = 0xc;
	devp->dp_cmd[5] = 0;

	(void) ts_startio(softp, devp);

	p_sema_v_lock(&softp->ts_iosync, PRIBIO, &softp->ts_lock,
		softp->ts_spl);

	softp->ts_spl = p_lock(&softp->ts_lock, TSSPL);
	if (softp->ts_fflags & TSF_FAIL) {
		if (ts_density == 5)
			return(-1);
		else {
			ts_density = 5;
			goto retry;
		}
	}
	return(0);
}

ts_request_sense(softp)
register struct ts_info *softp;

{
	register struct sec_dev_prog *devp;

#ifdef	TS_DEBUG
	if (ts_debug >= 3) {
		printf("ts_request_sense\n");
	}
#endif	TS_DEBUG

	bzero((char *)softp->ts_sensebuf, (u_int)tssensebuf_sz);
	devp = &softp->ts_sensereq.rs_dev_prog;

	devp->dp_status1 = 0;
	devp->dp_count = 0;
	devp->dp_data_len = tssensebuf_sz;
	devp->dp_cmd[0] = SCSI_RSENSE;
	devp->dp_cmd[1] = softp->ts_lun;
	devp->dp_cmd[2] = 0;
	devp->dp_cmd[3] = 0;
	devp->dp_cmd[4] = (u_char)tssensebuf_sz;
	devp->dp_cmd[5] = 0;

	sec_startio(SINST_REQUESTSENSE,
		&softp->ts_sensereq.rs_status,
		softp->ts_desc);

	return(0);
}

ts_startio(softp, devp)
register struct ts_info *softp;
register struct sec_dev_prog *devp;
{
	struct sec_pq *sq;
	struct sec_progq *pq;

#ifdef TS_DEBUG
	if (ts_debug >= 2) {
		printf("ts_startio\n");
	}
#endif	TS_DEBUG

	sq = &softp->ts_reqq;
	pq = sq->sq_progq;
	pq->pq_un.pq_progs[pq->pq_head] = devp;
	pq->pq_head = (pq->pq_head + 1) % sq->sq_size;
	sec_startio(SINST_STARTIO,
		&softp->ts_status,
		softp->ts_desc);
	if (softp->ts_status != SINST_INSDONE) {
		printf("ts%d: ts_startio: bad status %x\n",
			softp->ts_unit, softp->ts_status);
                /*
                 *+ The ts driver received a bad SCED
                 *+ completion status from a command.
                 *+ There might be a problem with the SCED.
                 */
	}

	return(0);
}

ts_restartio(softp, how)
register struct ts_info *softp;
int how;
{
#ifdef	TS_DEBUG
	if (ts_debug >= 2) {
		printf("ts_restartio: %s\n",
		(how == SINST_RESTARTIO) ? "next" : "current");
	}
#endif	TS_DEBUG

	sec_startio(how,
		&softp->ts_status,
		softp->ts_desc);
	if (softp->ts_status == (SINST_INSDONE | SEC_ERR_NO_MORE_IO)) {
	} else if (softp->ts_status != SINST_INSDONE) {
		printf("ts%d: ts_restartio: bad status %x\n",
			softp->ts_unit, softp->ts_status);
                /*
                 *+ The ts driver received a bad SCED completion
                 *+ status.  This can indicate a SCED problem.
                 */
        }
}



/*
 * Program queue management routines.  These may be candidates for
 * implementation as macros.  Or, they may be moved in line and
 * implemented more efficiently later.
 *
 * TS_TEST_Q_EMPTY  - return true if program queue is empty
 */

TS_TEST_Q_EMPTY(sq)
struct sec_pq	*sq;
{
	struct sec_progq *pq;

	pq = sq->sq_progq;

	return(pq->pq_head == pq->pq_tail);
}

/*
 * TS_GET_Q_TAIL  - return pointer to first completed device
 *			program.
 */

struct sec_dev_prog *
TS_GET_Q_TAIL(sq)
struct sec_pq	*sq;
{
	struct sec_progq *pq;

	pq = sq->sq_progq;

	return(pq->pq_un.pq_progs[pq->pq_tail]);
}

/*
 * TS_INCR_Q_TAIL  - increment the tail of the queue, thus deleting a
 *			device program to the completion queue.
 */

TS_INCR_Q_TAIL(sq)
struct sec_pq	*sq;
{
	struct sec_progq *pq;

	pq = sq->sq_progq;

	pq->pq_tail = (pq->pq_tail + 1) % sq->sq_size;
}


/*
 * do an INQUIRY command to the device and confirm that it is a
 * SCSI tape device, ie, that the device code is `1' (meaning
 * sequential access device), and the `removable media' bit is set.
 */

tsinquiry(sed)
	struct sec_probe *sed;
{
	register int	i;
	register struct	sec_powerup *iq = sed->secp_desc->sec_powerup;
	struct tsinq *tsinqp;
	int	spin;
	int	stat=0;			/* NOTE: assume address below 0x400000 , mapping not on yet */

	ASSERT_DEBUG(((int)&stat < 0x400000),"tsinquiry: configuration problem - address of \"stat\" greater then 0x400000");
#ifdef TSDEBUG
	if(ts_debug>3)
		printf("tsreadc: init_q 0x%x, slicid 0x%x stat=0x%x, device=%d\n", (int)iq, sed->secp_desc->sec_slicaddr, &stat, sed->secp_chan);
	printf("sed->secp_chan = %d\n", sed->secp_chan);
#endif TSDEBUG 

	tsinqp = (struct tsinq *)calloc(sizeof(struct tsinq));
	/*
	 * Fill out the device program for 
	 * a single command (test unit ready)
	 */
	ts_devprog.dp_un.dp_data = (u_char *)tsinqp;
	ts_devprog.dp_next = NULL;
	ts_devprog.dp_count = NULL; 		/* Ether ONLY */
	ts_devprog.dp_data_len = sizeof(struct tsinq);
	ts_devprog.dp_cmd_len = 6;
	ts_devprog.dp_status1 = 0;
	ts_devprog.dp_status2 = 0;

	/*
	 * setup the logical unit number in the SCSI cmd test unit ready.
	 */

	bzero((caddr_t)ts_devprog.dp_cmd, 10);
	ts_devprog.dp_cmd[0] = SCSI_INQUIRY;
	ts_devprog.dp_cmd[1] |= (unsigned char)(sed->secp_unit << SCSI_LUNSHFT);
	ts_devprog.dp_cmd[4] = sizeof(struct tsinq);

	i = iq->pu_requestq.pq_head;
	iq->pu_requestq.pq_un.pq_progs[i] = &ts_devprog;		/* set device prog */
	iq->pu_requestq.pq_head = (i+1) % SEC_POWERUP_QUEUE_SIZE;	/* mark in progress */

	SEC_startio(	SINST_STARTIO, 			/* command */
			&stat,				/* return status loc */
			TS_ANYBIN, 			/* bin number on SEC */
			sed->secp_chan,			/* Disk channel # */
			&iq->pu_cib, 			/* device input q loc */
			(u_char)sed->secp_desc->sec_slicaddr	/* SEC slic id number */
		);

	/*
	 * ok, SCSI command in progress so spin and wait for it to
	 * complete. 
	 * XXX Note: Async timeout issue affect the sanity check below.
	 * There should never be an async timeout from the firmware
	 * so hanging here is ok.
	 */
	spin = calc_delay((unsigned int)tsspintime);			/* sanity counter */
	if (stat == SINST_INSDONE) {
		while(iq->pu_doneq.pq_tail == iq->pu_doneq.pq_head) {
			ASSERT(spin-->0,"tsinq: Async timed out");
                        /*
                         *+ tsprobe timed out while waiting for a
                         *+ command completion status from the
                         *+ SCED.  The SCSI command being executed
                         *+ was Test Unit Ready.
                         */
		}

		iq->pu_doneq.pq_tail = (iq->pu_doneq.pq_tail+1) % SEC_POWERUP_QUEUE_SIZE;	/* tell board "done" */
	}else{
		iq->pu_requestq.pq_tail = iq->pu_requestq.pq_head = NULL;
		printf("ts: probe issued a totally bad command (params)\n");
		/*
		 *+ tsinquiry() SCSI INQUIRY command returned a bad status.
		 */
		return(SECP_NOTFOUND);
	}

	if(ts_devprog.dp_status1 != SEC_ERR_NONE) {
#ifdef SDDEBUG
		printf("ts: err status=0x%x\n", ts_devprog.dp_status1);
#endif SDDEBUG
		ts_devprog.dp_status1 = 0;
		SEC_startio(	SINST_RESTARTIO,		/* command */
				&stat,				/* return status loc */
				TS_ANYBIN, 			/* bin number on SEC */
				sed->secp_chan,			/* Disk channel # */
				&iq->pu_cib, 			/* device input q loc */
				(u_char)sed->secp_desc->sec_slicaddr	/* SEC slic id number */
			);
		return(SECP_NOTFOUND);
	}

	if (tsinqp->tsq_devtype == TS_DEVTYPE && 
	    tsinqp->tsq_rmv == TS_RMV)
		return(SECP_FOUND);
	else
		return(SECP_NOTFOUND);
}
