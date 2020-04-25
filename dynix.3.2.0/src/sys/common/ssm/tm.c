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
static char rcsid[]="$Header: tm.c 1.9 1991/05/14 23:07:25 $";
#endif

/*
 * tm.c
 *	SCSI tape driver for the System's Services Module (SSM)
 */

/* $Log: tm.c,v $
 *
 */

#include "../h/param.h"			/* Sizes */
#include "../h/mutex.h"			/* Gates, semaphores, and such */
#include "../h/user.h"			/* User info and errno.h */
#include "../h/file.h"			/* Flag defines */
#include "../machine/pte.h" 		/* Page tables info */
#include "../h/vmmac.h"			/* VM related conversion functions */
#include "../h/buf.h"			/* struct buf, header and such */
#include "../h/uio.h"			/* struct uio */
#include "../h/ioctl.h"			/* I/O copin's... */
#include "../h/mtio.h"			/* Ioctl cmds from mt/others.. */
#include "../h/scsi.h"			/* Standard SCSI definitions */
#include "../h/cmn_err.h"
#include "../machine/intctl.h"		/* Spl declarations */
#include "../ssm/ioconf.h"		/* IO Configuration Definitions */
#include "../balance/cfg.h"
#include "../ssm/ssm.h"			/* SCSI common data structures */
#include "../ssm/ssm_misc.h"		/* SCSI drivers' common stuff */
#include "../ssm/ssm_scsi.h"		/* SCSI command definitions */
#include "../ssm/tm.h"			/* Driver local structures */

extern caddr_t calloc();
extern caddr_t wmemall();
extern bool_t cp_sema();
extern unsigned max(), min();
static int tm_baselevel;
static struct tm_info *tm_info = NULL;
static tmprobe(), tmboot(), tmintr();
struct ssm_driver tm_driver = {
      /* driver prefix, configuration flags, probe(),  boot(),  interrupt() */
	"tm", 	        SDR_TYPICAL_SCSI,    tmprobe, tmboot, tmintr
};

/*
 * Tables for error messages.
 * Null or empty entries are not 
 * used or supported in this driver.
 */
static struct tm_errors {
	char *se_data;
	int  se_prntflag;
} tm_errors[] = {
	{ "No sense", 0 }, 
	{ "Recoverable error", 0 },
	{ "Tape not ready", 0 },
	{ "Media error", 1 },
	{ "Hardware error", 1 },
	{ "Illegal request", 1 },
	{ "Unit attention", 0 },
	{ "Media is protected", 0 },
	{ "Blank check", 0 },
	{ "Vendor unique error", 1 },
	{ "Aborted command", 1 },
	{ "Aborted command", 1 },
	{ "Unknown error", 1 },
	{ "Volume overflow", 0 },
	{ "Unknown error", 1 },
	{ "Unknown error", 1 },
	{ "Vendor unique error", 1 }
};

static char *tm_cmd[] = {
	"test unit ready",
	"rewind",
	"retension",
	"request sense",
	"", "", "", "",
	"read",
	"",
	"write",
	"inquiry", 
	"", "", "", "",
	"write file marks",
	"space",
	"", "", "",
	"mode select",
	"", "", "",
	"erase",
	"", ""
};

/*
 * Forward references.
 */
int tmminphys();		
static int tm_pop_cb();
static void tm_q_cb(), tm_push_cb();
static tmstrat();

/*
 * Note:  Once booted, many of the state transitions
 * within this driver are based upon info structure
 * semaphores that a process waits upon, as opposed 
 * to flags. 
 *
 * System call entries are serialized upon entry to 
 * the driver with the tm_usersync semaphore to 
 * prevent interference with one another.  An info
 * structure lock must still be used to protect 
 * critical regions between those paths and the ISR.
 *
 * The SSM assures that CBs are terminated in the
 * order started.
 *
 * Semaphores are associated with an index into
 * a devices I/O buffer/CB arrays to form simple
 * queue descriptors, the semaphore serving as an 
 * element counter and resource wait queue.  The
 * queues are manipulated with simple utility
 * routines: tm_q_cb(), tm_push_cb(), and 
 * tm_pop_cb(). 
 *
 * I/O buffer and CB allocation is based upon the 
 * tm_free queue. 
 *
 * CBs that have been started but have not terminated 
 * are appended to the tm_active queue.  Upon 
 * termination, the ISR removes them from there, 
 * appends them to another queue and v_sema's its 
 * semaphore.  READs use the tm_dav queue and its 
 * semaphore, WRITEs use the tm_free queue and its 
 * semaphore, and others use the tm_gen_cmd queue 
 * and its semaphore.
 *
 * Some diagnostic messages are printed using uprintf
 * instead of printf.  This occurs in situations where
 * the behaviour of this drive differs from standard
 * tape drives or where the error could be catostrophic
 * if the user did not see it, such as failed writes
 * to the media.
 */

/*
 * tmprobe()
 *	Probe for a SCSI streaming tape drive on an SSM.
 *	If the device is found return SCP_FOUND, 
 *	otherwise return SCP_NOTFOUND.
 */
static 
tmprobe(sp)
	register struct ssm_probe *sp;
{
	register struct scsi_cb *cb;
	struct scb_init sinit;
	struct scinq *inquiry;
	int i, cmd_completion;

	ASSERT(sp, "tmprobe: Unexpected NULL probe descriptor.");
	/*
	 *+ The probe function of the tm driver received a false
	 *+ descriptor address from the configuration code.  The
	 *+ configuration code may be broken.
	 */
	/* 
  	 * Fill out the scb_init structure and
	 * invoke a general SSM/SCSI function to
 	 * initialize the SSM for this device, 
	 * allocate CB's for probing and get a
	 * device i.d. number from the SSM.
 	 */
	sinit.si_mode = SCB_PROBE;
	sinit.si_ssm_slic = sp->sp_desc->ssm_slicaddr;
	sinit.si_scsi = (u_char)sp->sp_busno;
	sinit.si_target = (u_char)sp->sp_target;
	sinit.si_lunit = (u_char)sp->sp_unit;
	init_ssm_scsi_dev(&sinit);

	if (sinit.si_id < 0) {
		return (SCP_NOTFOUND);	/* An initialization error occurred */
	}

	cb = sinit.si_cbs;	/* CB to use for SCSI probe commands */
	ASSERT(cb, "tmprobe: CB not allocated");
	/*
	 *+ The tm driver could not allocate the SCSI resources
  	 *+ needed to probe for its device.  Either another
	 *+ driver has taken more than its share or more memory should
	 *+ be added to your system.
	 */

	/* 
	 * Compute and store SSM SLIC vector 
	 * for this device in the CB.  It's 
	 * expected to be there later.
	 */
	cb->sw.cb_unit_index = SCVEC(sinit.si_id, 0);  
	
	/* 
	 * To determine its type issue SCSI inquiry 
	 * command to the device, polling for for its 
	 * termination, and determine if the returned 
	 * inquiry data indicates it is a a sequential 
	 * access device with removable media.
	 * 
	 * If a check condition occurs, try sending a few 
	 * SCSI test-unit-ready commands which should clear 
	 * it if the device was simply reporting powerup.
	 */
	cmd_completion = scsi_probe_cmd(SCSI_INQUIRY, cb, sp);
	inquiry = (struct scinq *) cb->sh.cb_addr;
	ASSERT(inquiry, "tmprobe: Inquirybuf not allocated");
	/*
         *+ The tm driver could not allocate the buffer space
	 *+ needed to probe for its device.  Either another
	 *+ driver has taken more than its share or more memory should
	 *+ be added to your system.
	 */

	if (cmd_completion) { 
		if (SCSI_CHECK_CONDITION(cb->sh.cb_status)) {
			for (i = 4; i && scsi_probe_cmd(SCSI_TEST, cb, sp) 
			     && SCSI_CHECK_CONDITION(cb->sh.cb_status); i--)
				continue;
			cmd_completion = scsi_probe_cmd(SCSI_INQUIRY, cb, sp);
		}
		if (cmd_completion && SCSI_GOOD(cb->sh.cb_status)
		&&  inquiry->sc_devtype == INQ_SEQ 
		&&  inquiry->sc_qualif == INQ_REMOVABLE) 
			return (SCP_FOUND);
		else 
			return (SCP_NOTFOUND);
	} else 
		return (SCP_NOTFOUND);
}
 
/*
 * tmboot()
 *	Initialize a group of SCSI tape drives.  
 *	Record the base interrupt vector for the group. 
 * 	Assume they are sequential and use the same SLIC 
 * 	bin.  Allocate and initialize the driver information 
 *	structures, descriptors for the iobuffers, and 
 *	dma mapping entries.  Perform any device specific 
 *	initialization to get things started.
 */
static
tmboot(ndevs, dev_array)
	int ndevs;
	struct ssm_dev dev_array[];
{
	register struct ssm_dev *devp = dev_array;
	struct tm_info *infop;
	struct tm_bconf *bconfp = tm_bconf;
	struct scsi_cb *cb;
	struct tm_iobuf *iobp;
	u_long *maps;
	int i, buf_size, num_maps;
	struct scb_init sinit;
	unsigned ibuf_size;

#ifdef DEBUG
	ASSERT(ndevs > 0, "tmboot: # devices to boot is less than one");
	ASSERT(dev_array, "tmboot: NULL device array address");
	ASSERT(tm_bconf, "tmboot: NULL binary configuration data address");
#endif DEBUG

	/*
 	 * Ignore device descriptions in excess of 
	 * the driver maximum or for which there is 
	 * not a binary configuration table entry.
	 */
	if (ndevs > TM_UNITMAX + 1) {
		CPRINTF("tmboot: %d minor device limit exceeded - ",
 			ndevs = TM_UNITMAX + 1);
		CPRINTF("extras ignored\n");
	}
	if (ndevs > tm_max_ndevs) {
		CPRINTF("tmboot: %d device binary configuration limit ",
			 ndevs = tm_max_ndevs);
		CPRINTF("exceeded (from conf_tm.c)\n");
		CPRINTF("\tadditional configured tape drives ignored\n");
	} else
		tm_max_ndevs = ndevs;	/* Save the actual number configured. */

	/*
	 * Store device group's base interrupt vector,  
	 * then allocate an array of info structure.  
	 */
	tm_baselevel = devp->sdv_sw_intvec;
	tm_info = infop = (struct tm_info *) 
		calloc(ndevs * sizeof(struct tm_info));
	ASSERT(tm_info, "tmboot: Info structure allocation failed");
	/*
         *+ There is not enough memory for the tm driver to
  	 *+ allocate its per device information structures.
	 *+ Either another device has taken more than its
	 *+ share of memory or more memory needs to be added to your
	 *+ machine.
	 */

	/*
	 * Determine the size of request-sense buffer 
	 * that the each device needs.  The buffer is 
	 * used once for each device when inquiring and 
	 * selecting its mode first time it is opened.
	 */
	ibuf_size = max((unsigned)TM_RSENSE_LEN, (unsigned)TM_MSEL_PARMLEN + 1);
	if (ibuf_size < sizeof(struct scinq))
		ibuf_size = sizeof(struct scinq);

	/* 
	 * Locate the binary configuration information 
	 * for each drive and attempt to boot it.
	 */
	for ( ; ndevs; ndevs--, infop++, devp++, bconfp++) {
		if (!devp->sdv_alive)
			continue;
		/* 
		 * Verify that there are enough dma mapping 
		 * entries available for this drive; they're 
		 * allocated by the SSM config code.  
		 * De-configure the device if there are not 
		 * enough to support the iobuffers for this drive.
		 */
		buf_size = roundup(bconfp->bc_bufsz * 1024, CLBYTES),
		num_maps = buf_size / CLBYTES;	
		if (devp->sdv_maps_avail < num_maps  * NCBPERSCSI) {
			CPRINTF("tm%d: not enough maps available ", 
				tm_max_ndevs - ndevs);
			CPRINTF("(%d available, %d needed) ... deconfigured\n",
				devp->sdv_maps_avail, num_maps * NCBPERSCSI);
			devp->sdv_alive = 0;
			continue;
		}
		ASSERT(devp->sdv_maps, "tmboot: DMA map address is NULL");
		/*
		 *+ The system's configuration code failed to allocate
	  	 *+ any DMA mapping entries for the tm driver.  Either
		 *+ the system is out of memory for DMA maps or
		 *+ this driver or other drivers are requesting too many
		 *+ maps from a global pool.
		 */

		/* 
  		 * Fill out the scb_init structure and
		 * invoke a general SSM/SCSI function to
 		 * initialize the SSM for this device, 
		 * allocate CB's and get a device i.d. 
		 * number from the SSM.
 		 */
		sinit.si_mode = SCB_BOOT;
		sinit.si_ssm_slic = devp->sdv_desc->ssm_slicaddr;
		sinit.si_scsi = (u_char)devp->sdv_busno;
		sinit.si_target = (u_char)devp->sdv_target;
		sinit.si_lunit = (u_char)devp->sdv_unit;
		sinit.si_host_bin = devp->sdv_bin;
		sinit.si_host_basevec = devp->sdv_sw_intvec;

		init_ssm_scsi_dev(&sinit);
		if (sinit.si_id < 0) {
			CPRINTF("tm%d: initialization error - exceeded", 
				tm_max_ndevs - ndevs);
			CPRINTF(" SCSI bus devices limit ... deconfigured\n");
			devp->sdv_alive = 0;
			continue;
		} 

		/*
		 * Initialize the information 
		 * structure for this drive.  
		 */
		infop->tm_devno = SCSI_DEVNO(devp->sdv_target, devp->sdv_unit);
		infop->tm_rwbits = bconfp->bc_rwbits,
		infop->tm_cflags = bconfp->bc_cflags,
		infop->tm_fflags = TMF_FIRSTOPEN;
		infop->tm_bufsize = buf_size;
		infop->tm_dev = devp;
		infop->tm_ssm = devp->sdv_desc;
		infop->tm_cbs = cb = sinit.si_cbs;  /* Base CB for device. */

		ASSERT(infop->tm_cbs, "tmboot: NULL base CB encountered");
		/*
		 *+ The SSM device initialization failed to allocate
		 *+ any SCSI resources for the tm driver.  Either
		 *+ a driver has allocated excessive amounts of memory
		 *+ or more memory should be added to your system.
		 */

		/* 
		 * Initialize each I/O buffer description and 
		 * corresponding CB associated with this device. 
		 * Allocate a set of dma maps for it, also.
		 */
		for (i = 0, iobp = infop->tm_iobuf, 
		     maps = (u_long *)devp->sdv_maps; 
		     i < NCBPERSCSI; i++, cb++, iobp++, maps += num_maps) {
			/*
			 * Allocate a request-sense buffer. 
			 * It will also be used once for inquiring
			 * about the device and selecting its mode 
			 * the first time it is opened.
			 */
			cb->sh.cb_sense = (u_long) ssm_alloc(
				ibuf_size, SSM_ALIGN_XFER, SSM_BAD_BOUND);
			cb->sh.cb_slen = TM_RSENSE_LEN;

			/* 
			 * Compute and store SSM SLIC vector 
			 * for this CB, in the CB.  It's 
			 * expected to be there later.
			 */
			cb->sw.cb_unit_index = SCVEC(sinit.si_id, i);  

			/* Locate a map table for the iobuffer */
			iobp->ci_nmaps = num_maps;
			cb->sw.cb_iovstart = iobp->ci_maps = maps;
		}

		/* 
		 * Complete initialization of the device's 
		 * info structure, including its locks and 
		 * semaphores.  
		 */
		infop->tm_part_write = -1;	/* None available */
		init_sema(&infop->tm_usrsync, 1, 0, tm_gate);
		init_sema(&infop->tm_free.sq_sync, NCBPERSCSI, 0, tm_gate);
		infop->tm_free.sq_count = NCBPERSCSI;
		init_sema(&infop->tm_active.sq_sync, 0, 0, tm_gate);
		infop->tm_active.sq_count = 0;
		init_sema(&infop->tm_dav.sq_sync, 0, 0, tm_gate);
		infop->tm_dav.sq_count = 0;
		init_sema(&infop->tm_gen_cmd.sq_sync, 0, 0, tm_gate);
		infop->tm_gen_cmd.sq_count = 0;
		init_lock(&infop->tm_lock, tm_gate);
	}
}
 
/*
 * tmopen()
 *	Attempt to open the specified device if 
 *	it corresponds to a unit which is alive 
 *	and available for exclusive use.  Allocate 
 *	the I/O buffers used to keep the drive 
 *	streaming and fill in the dma maps for them.
 */
tmopen(dev)
	dev_t dev;
{
	int unit = TM_UNIT(dev) ;
	struct tm_info *infop = tm_info + unit;

	/* 
	 * Verify that the minor device number.
	 * is within the valid range of tape
	 * devices and that it corresponds to
	 * an active device.
	 * 
 	 * The minor device number contains unused 
	 * bits, NOREWIND flag, and unit number.
	 */
	if (minor(dev) > TM_MAXMINOR 
	|| unit >= tm_max_ndevs
	|| tm_info == NULL
	|| infop == NULL
	|| !infop->tm_dev) 
		return (ENXIO);		/* Device is not available */

	p_sema(&infop->tm_usrsync, PRIBIO);
	infop->tm_spl = p_lock(&infop->tm_lock, TM_SPL);

	/* 
	 * This device can only be opened for 
	 * exclusive access.  Verify that it 
	 * is closed. 
	 */
	if (infop->tm_openf == TMO_RWNDCLS)
		/*
 		 * The previous close started a rewind
		 * operation.  Wait for its termination
		 * and then free the CB it used.
	 	 */
		tm_q_cb(&infop->tm_free, 
			tm_pop_cb(&infop->tm_gen_cmd, infop, TM_UNLOCK));
	else if (infop->tm_openf != TMO_CLOSED) {
		v_lock(&infop->tm_lock, infop->tm_spl);
		v_sema(&infop->tm_usrsync);
		return (EBUSY);
	}

	infop->tm_openf = TMO_OPEN;

#ifdef DEBUG
	ASSERT(infop->tm_free.sq_count == NCBPERSCSI,
		"tmopen: Not all CBs are currently inactive");
#endif DEBUG

	/*
	 * Wait, if necessary, for the device to 
	 * come online.  If it errors out then mark 
	 * the device as closed and return.
	 */ 
	if (!gen_scsi_cmd(SCSI_TEST, infop, 0, 0, TM_WAIT)) {
		infop->tm_openf = TMO_CLOSED;
		v_lock(&infop->tm_lock, infop->tm_spl);
		v_sema(&infop->tm_usrsync);
		return (EIO);
	} 

	if (infop->tm_fflags & TMF_FIRSTOPEN) {
		/*
	 	 * The first time this device is opened after 
	 	 * booting, its mode of operation must be set 
	 	 * (type of I/O, device block size, density).
	 	 * Atmempt to have the device use its default 
	 	 * density first, if that fails, then re-select 
	 	 * with an explicit density.
		 * The length of the mode selection data is
		 * based upon the drive type, which is set
		 * by gen_scsi_cmd after the inquiry command.
	 	 */
		if (!gen_scsi_cmd(SCSI_INQUIRY, infop, 0, 0, TM_WAIT)
		||  (!gen_scsi_cmd(SCSI_MODES, infop, 0,
				   MSEL_DEN_DEFAULT, TM_WAIT) 
		     && !gen_scsi_cmd(SCSI_MODES, infop, 0,
				MSEL_DEN_QIC24, TM_WAIT))) {
			infop->tm_openf = TMO_CLOSED;
			v_lock(&infop->tm_lock, infop->tm_spl);
			v_sema(&infop->tm_usrsync);
			return (EIO);
		}
		infop->tm_fflags &= ~TMF_FIRSTOPEN;
		infop->tm_fflags |= TMF_ATTEN;
		infop->tm_curmode = TMM_GENERAL;
		infop->tm_fileno = 0;

	} else if (infop->tm_fflags & TMF_ATTEN) 
		infop->tm_curmode = TMM_GENERAL;

	infop->tm_blkno = 0;

	v_lock(&infop->tm_lock, infop->tm_spl);

	/* 
	 * Complete the initialization of this device.  
	 * Clear the state flags, then attempt to allocate 
	 * and initialize the iobuffers.  
	 */
	infop->tm_fflags &= 
		~(TMF_LASTIOW | TMF_LASTIOR | TMF_FAIL | TMF_EOF | TMF_EOM);
	if (tm_iobuf_init(infop)) {
		v_sema(&infop->tm_usrsync);
		return (0);
	} else {
		infop->tm_openf = TMO_CLOSED;
		v_sema(&infop->tm_usrsync);
		return (ENOMEM);
	}
}
 
/* tmclose()
 *	Perform required cleanup and mark the 
 *	device as no longer open, notifing 
 *	user level of any errors. 
 */
tmclose(dev, flags)
	dev_t dev;
	int flags;
{
	register struct tm_info *infop = tm_info + TM_UNIT(dev);
	
	p_sema(&infop->tm_usrsync, PRIBIO);
	infop->tm_spl = p_lock(&infop->tm_lock, TM_SPL);

	/* 
	 * Wait for outstanding I/O to terminate.
	 * Notify user level any errors. 
	 */
	if (!tm_io_flush(infop)) 
		u.u_error = EIO;

	/*
	 * If the file was open for writing or was 
	 * written on as its last operation, write 
	 * a filemark to make the end-of-file.
	 */
	if ((flags == FWRITE 
	     ||   flags & FWRITE && infop->tm_fflags & TMF_LASTIOW) 
	&&  !gen_scsi_cmd(SCSI_WFM, infop, 1, 0, TM_WAIT)) {
		u.u_error = EIO;
		uprintf("tm%d: error writing file mark on close\n",
			TM_UNIT(dev));
		/*
		 *+ The tm driver's SCSI write file mark command
		 *+ failed.  This probably occurred, along with
		 *+ an error, while the kernel was attempting to
		 *+ write to the medium.  The medium might have
		 *+ been protected against writes or against an
		 *+ attempt to write while not at the beginning
		 *+ of the tape or at the logical end of data.
		 */
	}

#ifdef DEBUG
	ASSERT(infop->tm_free.sq_count == NCBPERSCSI,
		"tmclose: Not all CBs are currently inactive");
#endif DEBUG

	/*
	 * Free the i/o buffers, finish cleaning up, 
	 * and mark the device as no longer open.
	 */
	if (TM_REWIND(dev)) {
		/*
		 * Start the rewind, but return to the caller 
		 * without awaiting completion.  It is important 
		 * to free the iobuffers prior to starting the 
		 * rewind command, since rewind will otherwise 
		 * lock a CB, hence an iobuffer, until completion.
		 */
		infop->tm_curmode = TMM_GENERAL;
		tm_iobuf_free(infop);
		(void) gen_scsi_cmd(SCSI_REWIND, infop, 0, 0, TM_NOWAIT);  
		infop->tm_openf = TMO_RWNDCLS;
		infop->tm_fileno = 0;
	} else {
		if (infop->tm_fflags & TMF_LASTIOR) {
			/* Skip to the next file. */
			(void) gen_scsi_cmd(SCSI_SPACE, infop, 1, 
				SCSI_SPACE_FILEMARKS, TM_WAIT);
		}
		tm_iobuf_free(infop);
		infop->tm_openf = TMO_CLOSED;
	}
	v_lock(&infop->tm_lock, infop->tm_spl);
	v_sema(&infop->tm_usrsync);
}
 
/* 
 * tmwrite()
 *	Invoke physio() to lock down user buffer 
 *	in memory and invoke the driver strategy 
 *	routine to perform the output.
 */
tmwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	return (physio(tmstrat, (struct buf *)NULL, dev, B_WRITE, 
			tmminphys, uio));
}
 
/* 
 * tmread()
 *	Invoke physio to lock down user buffer 
 *	in memory and invoke the driver strategy 
 *	routine to perform the input.
 */
tmread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	return (physio(tmstrat, (struct buf *)NULL, dev, B_READ, 
			tmminphys, uio));
}
 
/*
 * tmstrategy()
 * 	READ or WRITE to the device via the raw 
 *	device interface using a buf-structure.
 * 
 *	If device is not open or I/O is being 
 *	attempted via the standard block interface 
 *	reject the attempt to perform I/O and 
 *	generate user level error.
 */
static
tmstrat(bp)
	struct buf *bp;
{
	register struct tm_info *infop = tm_info + TM_UNIT(bp->b_dev);

	ASSERT(bp, "tmstrat: Unexpected NULL buf-struct pointer");
	/*
	 *+ physio passed an invalid address describing a transfer request
	 *+ to the tm driver's strategy function.
	 */
	bp->b_resid = 0;
	
	p_sema(&infop->tm_usrsync, PRIBIO);

	/* 
	 * Reject invalid operations.  Otherwise, invoke 
	 * the appropriate internal I/O interface to 
	 * execute the operation.
	 */
	if (infop->tm_openf != TMO_OPEN) {
		bp->b_resid = bp->b_bcount,
		bp->b_flags |= B_ERROR;
		bp->b_error = EBADF;
	} else if (bp->b_iotype == B_FILIO) {
		bp->b_resid = bp->b_bcount,
		bp->b_flags |= B_ERROR;
		bp->b_error = ENXIO;
	} else if (bp->b_flags & B_READ)
		tm_raw_read(infop, bp);
	else 
		tm_raw_write(infop, bp);

	iodone(bp);			/* Indicate I/O termination.  */
	v_sema(&infop->tm_usrsync);
}
 
/*
 * tmintr()
 *	Interrupt service routine for the SCSI tape 
 *	driver.  Determine which CB of which device 
 *	interrupted and process the interrupt if the 
 *	CB has terminated.  Make appropriate state
 *	transitions, perform error reporting, and 
 *	awaken processes waiting that CB's termination.
 */
static
tmintr(level)
	int level;
{
	register struct scsi_cb *cb;
	char *errmsg = NULL;
	long residue = -1;
	u_int command;
	int index, next_cb;
	int unit = (level -= tm_baselevel) / NCBPERSCSI;
	int cb_num = level & NCBPERSCSI - 1;
	struct tm_iobuf *iobp;
	register struct tm_info *infop = tm_info + unit;
	spl_t s;

	/*
	 * Verify that the interrupt is not bogus.
	 * The vector must correspond to a valid
	 * device unit number, that device must
	 * have CBs active, and the CB at the
	 * head of the active CB queue is the one
	 * to which this interrupt vector must
	 * correspond, since the SSM terminates
	 * CBs in the order started for a device.  
	 */
	if (unit < 0 || unit >= tm_max_ndevs) {
		CPRINTF("tmintr: Invalid interrupt vector; ignored\n");
		return;
	}

	infop->tm_spl = p_lock(&infop->tm_lock, TM_SPL);

	if (infop->tm_active.sq_count <= 0) {
		CPRINTF("tm%d: inactive device; interrupt ignored\n", unit);
		v_lock(&infop->tm_lock, infop->tm_spl);
		return;
	}
	if (infop->tm_active.sq_head != cb_num) {
		CPRINTF("tm%d: unexpected CB termination order; ", unit);
		CPRINTF("interrupt ignored\n");
		v_lock(&infop->tm_lock, infop->tm_spl);
		return;
	}

	cb = infop->tm_cbs + cb_num;
	iobp = infop->tm_iobuf + cb_num;
	command = (u_int)cb->sh.cb_scmd[0];

	switch (cb->sh.cb_compcode) {	/* Validate the CB termination */
	case SCB_BUSY:
		/* Not really done yet; ignore this interrupt */
		CPRINTF("tm%d: interrupt ignored; CB is busy\n", unit); 
		v_lock(&infop->tm_lock, infop->tm_spl);
		return;
	case SCB_BAD_CB:
		infop->tm_fflags |= TMF_FAIL;
		errmsg = "CB is not acceptable";
		break;
	case SCB_NO_TARGET:
		infop->tm_fflags |= TMF_FAIL;
		errmsg = "Target adapter does not respond";
		break;
	case SCB_SCSI_ERR:
		infop->tm_fflags |= TMF_FAIL;
		errmsg = "A SCSI protocol error occurred";
		break;
	case SCB_OK:
		/* 
		 * Good CB termination; Now check its
		 * SCSI termination status.
		 */
		if (tm_check_condition(infop, cb, iobp, residue)) {
			tm_report_sense_info(infop, cb, iobp);
			errmsg = iobp->ci_msg;
			residue = iobp->ci_resid;
		} else if (!SCSI_GOOD(cb->sh.cb_status)) {
			infop->tm_fflags |= TMF_FAIL;
			errmsg = "Unknown Program Error";
		}
		break;
	default:
		panic("tmintr - Bad SCSI command block completion status");
		/*
		 *+ A SCSI command completed with an illegal completion
		 *+ status.
		 */
		/*NOTREACHED*/
	}

	/* 
	 * Report errors that occurred.
	 */
	if (errmsg)
		CPRINTF("tm%d: %s on command %s.\n", unit, errmsg, 
			tm_cmd[command]);

	/*
	 * The cb_num calculated earlier is the head of the 
	 * infop->tm_active queue, since the SSM 
	 * supposedly terminates CBs for a device in the 
	 * order they were started.  Delete that CB from 
	 * the queue.  An earlier check ensures the CB is
 	 * the queue head and available and passing TM_LOCKED
	 * to tm_pop_cb ensures that the info structure lock
	 * is not released (and reaquired).
	 * 
	 */
	index = tm_pop_cb(&infop->tm_active, infop, TM_LOCKED);

	/*
	 * A process is sleeping on a semaphore awaiting 
	 * command termination; determine which one and
	 * take action to wake it up.
	 */
	switch ((u_int)cb->sh.cb_scmd[0]) {
	case SCSI_READ:
		/* 
		 * If a failure occurred, process
		 * it and inhibit further readahead.
		 */
		if (infop->tm_fflags & TMF_FAIL) {
			if (infop->tm_fflags & TMF_EOF) {
				/* Not an error; compute how much data 
				 * was read before EOF. Correct underflow 
				 * that occurs on some devices.  Clear
			 	 * the failure flag.
				 */
				if (residue < 0 
				||  (iobp->ci_nblks -= residue) < 0)
					iobp->ci_nblks = 0;
				iobp->ci_err_flag = 0;
				infop->tm_fflags &= ~TMF_FAIL;
			} else {
				/* Note the error for the sleeping process */
				iobp->ci_err_flag = 1;
			}
		} else if (infop->tm_free.sq_count > 0
		  &&	   infop->tm_openf == TMO_OPEN) {
			/* 
			 * Start a readahead, since the this 
			 * read had valid termination, there
			 * are more CBs available, and possibly
			 * more read requests to come.  Note:
			 * the checks above will shut readahead
			 * down after reaching EOF or while
			 * flushing I/O.
			 */
#ifdef DEBUG
		  	ASSERT(!(infop->tm_fflags & TMF_EOF),
				"tmintr: Readahead when EOF set");
#endif DEBUG

			next_cb = tm_pop_cb(&infop->tm_free, infop, 
				TM_LOCKED);
#ifdef DEBUG
			ASSERT(next_cb >= 0 && next_cb < NCBPERSCSI, 
				"tmintr: bad CB index");
#endif DEBUG
			/*
			 * The following assertion applies as long as
			 * NCBPERSCSI is defined as 2.
			 */ 
#ifdef DEBUG
			ASSERT(next_cb != index,
			       "tmintr: next CB index == terminated CB index");
#endif DEBUG
			tm_start_read(next_cb, infop);
		}

		/*
		 * Awaken any process waiting for 
		 * READ data availability.
		 */
		tm_q_cb(&infop->tm_dav, index);
		break;
	case SCSI_WRITE:
		if ((infop->tm_fflags & (TMF_FAIL | TMF_EOM)) == TMF_EOM 
		&&  residue > 0) {
			/*
			 * Early warning notification.  Adjust
			 * the CB and attempt to re-write 
			 * the blocks not written this time.
			 */
#ifdef DEBUG
			ASSERT(iobp->ci_nblks >= residue,
				"tm_intr: residue > original transfer count");
			ASSERT(cb->sh.cb_count == dbtob(residue),
				"tm_intr:cb_count not correctly updated by FW");
/*
 * A bug in the SSM firware causes the 
 * following assertion to fail.  Work 
 * around it for now.
 *			ASSERT(cb->sh.cb_iovec == iobp->ci_maps + 
 *				dbtob(iobp->ci_nblks - residue) / CLBYTES,
 *				"tm_intr:cb_iovec not correctly updated by FW");
 */
#endif /* DEBUG */
			cb->sh.cb_clen = SCSI_CMD6SZ;
			cb->sh.cb_scmd[2] = (u_char)(residue >> 16);
			cb->sh.cb_scmd[3] = (u_char)(residue >> 8);
			cb->sh.cb_scmd[4] = (u_char)residue;
			cb->sh.cb_addr += dbtob(iobp->ci_nblks - residue);
 			cb->sh.cb_iovec = iobp->ci_maps + 
 				dbtob(iobp->ci_nblks - residue) / CLBYTES;
			cb->sh.cb_compcode = SCB_BUSY;
			tm_push_cb(&infop->tm_active, index);
			s = splhi();
			mIntr(infop->tm_ssm->ssm_slicaddr, SCSI_BIN,
				cb->sw.cb_unit_index);
			splx(s);
		} else {
			/* Terminate the current CB */
			tm_q_cb(&infop->tm_free, index);
			if (infop->tm_fflags & TMF_FAIL) {
				/*
			 	 * Terminate other CB's that are 
				 * queued but that the SSM has 
				 * not been notified.
			 	 */
				while (infop->tm_active.sq_count) {
					index = tm_pop_cb(&infop->tm_active, 
						infop, TM_LOCKED);
					tm_q_cb(&infop->tm_free, index);
				}
			} else if (infop->tm_active.sq_count > 0) {
				/* Start the next queued request */
				s = splhi();
				mIntr(infop->tm_ssm->ssm_slicaddr, SCSI_BIN, 
					infop->tm_cbs[infop->tm_active.sq_head].
						sw.cb_unit_index);
				splx(s);
			}
		}
		break;
	case SCSI_TEST:
		if (infop->tm_fflags & TMF_ATTEN) 
			infop->tm_fflags &= ~TMF_FAIL; 
		/* Fall through to the default case */
	default:
		/* Awaken potential waiting process */
		tm_q_cb(&infop->tm_gen_cmd, index); 
		break;
	}
	v_lock(&infop->tm_lock, infop->tm_spl);
}
 
/*
 * tmioctl() 
 *	Driver asynchronous i/o control requests.
 *	Verify that the command is a tape operation 
 *	and verify that it is compatible with the 
 * 	current mode of operation, before performing 
 *	the operation and reporting its termination 
 *	status.  
 */
tmioctl(dev, cmd, data)
	dev_t	dev;
	int	cmd;
	caddr_t data;
{
	register struct tm_info *infop = tm_info + TM_UNIT(dev);
	struct mtop *mtop = (struct mtop *)data;
	struct mtget	*mtget;
	int success = 1;
	int opcount;

	/* 
	 * Validate the ioctl cmd requested, the range 
	 * of the operation count, and that the data 
	 * address is non-NULL.
	 */
	switch (cmd) {
	case MTIOCGET:
		mtget = (struct mtget *)data;
		mtget->mt_dsreg = 0;
		mtget->mt_erreg = 0;
		mtget->mt_resid = infop->tm_resid;
		mtget->mt_fileno = infop->tm_fileno;
		mtget->mt_blkno = infop->tm_blkno;
		mtget->mt_type = MT_ISTS;
		return (0);
	case MTIOCTOP:
		break;
	default:
		return (EINVAL);
	}
	if (mtop == NULL || (opcount = mtop->mt_count) < 0 || opcount > 0x7ff) 
		return (EINVAL);

	p_sema(&infop->tm_usrsync, PRIBIO);
	infop->tm_spl = p_lock(&infop->tm_lock, TM_SPL);

	/* Verify the operation is allowable in the current mode. */
	switch (infop->tm_curmode) {
	case TMM_READ:
		if (mtop->mt_op == MTWEOF) {
			uprintf("tm%d: can't write eof after read\n",
				TM_UNIT(dev));
			/*
			 *+ The tm driver does not support
			 *+ writing a filemark while in read
			 *+ mode.
			 */
			v_lock(&infop->tm_lock, infop->tm_spl);
			v_sema(&infop->tm_usrsync);
			return (EINVAL);
		}
		break;
	case TMM_WRITE:
		switch (mtop->mt_op) {
		case MTFSF:
		case MTFSR:
			uprintf("tm%d: bad operation after write\n",
				TM_UNIT(dev));
			/*
			 *+ The attempted operation is not supported by
		  	 *+ the tm driver when in write mode.
			 */
			v_lock(&infop->tm_lock, infop->tm_spl);
			v_sema(&infop->tm_usrsync);
			return (EINVAL);
		}
		break;
	case TMM_GENERAL:
		if (infop->tm_fflags & TMF_ATTEN 
		&&  infop->tm_cflags & TMC_AUTORET)
			switch (mtop->mt_op) { 
			case MTERASE:
			case MTRET:
			case MTNORET:
				break;
			default:
				/* Retention the media first. */
				(void) gen_scsi_cmd(SCSI_LOAD_UNLOAD, infop, 0,
					SCSI_LOAD_MEDIA | SCSI_RETEN_MEDIA, 
					TM_WAIT);
				break;
			}
		infop->tm_fflags &= ~TMF_ATTEN;
		break;
	}

	/* 
	 * Wait for all outstanding I/O to terminate 
	 * before closing.  Propagate errors to the 
	 * user level.
	 */
	if (!tm_io_flush(infop)) {
		infop->tm_openf = TMO_ERR;
		v_lock(&infop->tm_lock, infop->tm_spl);
		v_sema(&infop->tm_usrsync);
		return (EIO);
	}
			
	/* Perform the specific tape operation. */
	switch (mtop->mt_op) {
	case MTNOP:
		break;
	case MTFSF: 
		success = gen_scsi_cmd(SCSI_SPACE, infop, opcount, 
				SCSI_SPACE_FILEMARKS, TM_WAIT);
		if (success) {
			infop->tm_fileno += opcount; 
			infop->tm_blkno = 0;
		}
		break;
	case MTWEOF:
		success = gen_scsi_cmd(SCSI_WFM, infop, opcount, 0, TM_WAIT);
		if (success) {
			infop->tm_fileno += 1;
			infop->tm_blkno = 0;
		}
		break;
	case MTSEOD:
		success = gen_scsi_cmd(SCSI_SPACE, infop, 0, 
			SCSI_SPACE_ENDOFDATA, TM_WAIT);
		if (success)
			infop->tm_fileno = -1;
		break;
	case MTREW:
		success = gen_scsi_cmd(SCSI_REWIND, infop, 0, 0, TM_WAIT);
		if (success) {
			infop->tm_fileno = 0;
			infop->tm_blkno = 0;
		}
		break;
	case MTERASE:
		success = gen_scsi_cmd(SCSI_ERASE, infop, 0, 0, TM_WAIT);
		break;
	case MTRET:
		success = gen_scsi_cmd(SCSI_LOAD_UNLOAD, infop, 0,
			SCSI_LOAD_MEDIA | SCSI_RETEN_MEDIA, TM_WAIT);
		break;
	case MTNORET:
		infop->tm_fflags &= ~TMF_ATTEN;
		break;
	default:
		v_lock(&infop->tm_lock, infop->tm_spl);
		v_sema(&infop->tm_usrsync);
		return (EINVAL);
	}
	v_lock(&infop->tm_lock, infop->tm_spl);
	v_sema(&infop->tm_usrsync);
	return ((success) ? 0 : EIO);
}
 
/*
 * scsi_probe_cmd()
 *	Fill out a CB for the specified SCSI command,
 *	start it and poll for termination.
 *	Return one upon success, zero for failure.
 */		
static int
scsi_probe_cmd(cmd, cb, sp)
	u_char cmd;
	register struct scsi_cb *cb;
	register struct ssm_probe *sp;
{
	static struct scrsense *sensebuf = NULL;
	static struct scinq *inquiry = NULL;

#ifdef DEBUG	
	ASSERT(cmd == SCSI_TEST || cmd == SCSI_INQUIRY, 
		"tmprobe: Unexpected SCSI command issued.");
	ASSERT(cb, "tmprobe: Unexpected NULL CB.");
	ASSERT(sp, "tmprobe: Unexpected NULL probe descriptor.");
#endif DEBUG

	/*
	 * Allocate a reusable SCSI request-sense buffer 
	 * for probing, if not already done.
	 */
	if (!sensebuf)
		sensebuf = (struct scrsense *)ssm_alloc(sizeof(struct scrsense),
			SSM_ALIGN_XFER, SSM_BAD_BOUND);

#ifdef DEBUG
	ASSERT(sensebuf, "tmprobe: Request-sense buffer not allocated.\n");
#endif DEBUG
		
	bzero((char *)cb, SCB_SHSIZ);
	cb->sh.cb_sense = (u_long) sensebuf;	/* Its virtaddr == physaddr */
	cb->sh.cb_slen =  sizeof(struct scrsense);
	cb->sh.cb_cmd = SCB_READ;
	cb->sh.cb_clen = SCSI_CMD6SZ; 
	cb->sh.cb_scmd[1] = sp->sp_unit << SCSI_LUNSHFT;

	if  ((cb->sh.cb_scmd[0] = cmd) == SCSI_INQUIRY) { 
		/*
		 * Fill in additional CB fields. 
		 *
	 	 * Allocate a reusable SCSI inquiry buffer 
	 	 * for probing, if not already done.
	 	 */
		if (!inquiry)
			inquiry = (struct scinq *) ssm_alloc(
				sizeof(struct scinq), SSM_ALIGN_XFER, 
				SSM_BAD_BOUND);
#ifdef DEBUG
		ASSERT(inquiry, "tmprobe: Inquiry buffer not allocated.\n");
#endif DEBUG

		cb->sh.cb_addr = (u_long)inquiry; /* Its virtaddr == physaddr */
		cb->sh.cb_count = sizeof(struct scinq);
		cb->sh.cb_scmd[4] = sizeof(struct scinq);
	}

	/*
	 * Start the SSM processing the CB 
	 * and poll for completion.
	 */
	cb->sh.cb_compcode = SCB_BUSY;
	mIntr(sp->sp_desc->ssm_slicaddr, SCSI_BIN, cb->sw.cb_unit_index);
	while (cb->sh.cb_compcode == SCB_BUSY) 
		continue;

	/*
	 * Return a value appropriate for the
	 * command completion.
	 */
	switch (cb->sh.cb_compcode) {
	case SCB_BAD_CB:
		CPRINTF("tmprobe - Bad SCSI command block on SSM@SLIC ");
		CPRINTF("%d SCSI%d\n", sp->sp_desc->ssm_slicaddr, sp->sp_busno);
		return (0);
	case SCB_SCSI_ERR:
		CPRINTF("tmprobe - Protocol error on SSM@SLIC %d SCSI%d\n",
			sp->sp_desc->ssm_slicaddr, sp->sp_busno);
		return (0);
	case SCB_NO_TARGET:
		/* 
		 * Selection phase timed out; no such 
		 * target adapter. Note: if target adapter 
		 * present and logical unit is not, a 
		 * SCSI_CHECK_CONDITION would result, 
		 * with cb_compcode == SCB_OK.
		 */
		return (0);
	case SCB_OK:
		return (1);
	default:
		/* Shouldn't happen... */
		CPRINTF("tmprobe - Bad SCSI command block completion status ");
		CPRINTF("on SSM@SLIC %d, SCSI%d\n", sp->sp_desc->ssm_slicaddr,
			sp->sp_busno);
		panic("bad SCSI");
		/*
		 *+ The tm driver received an unknown completion
		 *+ code for a SCSI command.
		 */
	}
	/*NOTREACHED*/
}
 
/* 
 * tmminphys()
 *	Adjust the transfer size in the buf-structure 
 *	addressed by bp to fit within device dependent 
 *	limits; in this case the maximum is the size 
 * 	of the i/o buffer allocated to the device.
 */
tmminphys(bp)		
	register struct buf *bp;
{
	register struct tm_info *infop = tm_info + TM_UNIT(bp->b_dev);
	
	ASSERT(bp, "tmminphys: Unexpected NULL buf-struct pointer");
	/*
	 *+ An invalid buf structure address was handed to the
	 *+ tm driver by the system's physio function.
	 */

	if (bp->b_bcount > infop->tm_bufsize) 
		bp->b_bcount = infop->tm_bufsize;
}
 
/* tm_start_read()
 * 	Build a SCSI READ command in infop->tm_cbs[index]
 * 	Attempt to read as much data as the CB's 
 *	I/O buffer can hold.  Start the CB and return.
 *	If there has been a failure or end-of-data 
 *	note it in the CB and put it on the termination 
 * 	queue instead.
 *
 *	Assume that the info structure is locked 
 *	upon entry.
 */
static
tm_start_read(index, infop)
	register int index;
	register struct tm_info *infop;
{
	register struct tm_iobuf *iobp;
	register struct scsi_cb *cb;
	spl_t s;

#ifdef DEBUG
	ASSERT(infop, "tm_start_read: NULL info-structure pointer");
	ASSERT(index >= 0 && index < NCBPERSCSI, 
		"tm_start_read: bad CB index");
#endif DEBUG
	
	iobp = infop->tm_iobuf + index;
	cb = infop->tm_cbs + index;
	iobp->ci_curbyte = 0;

	/*
	 * Don't really activate a read if 
	 * Read failure conditions present
	 * or attempting to flush I/O activity.
	 */
	if (infop->tm_fflags & (TMF_EOM | TMF_EOF | TMF_FAIL)
	||  infop->tm_openf != TMO_OPEN) {
		iobp->ci_nblks = 0;
		iobp->ci_err_flag = (infop->tm_fflags & TMF_EOF) ? 0 : 1;
		tm_q_cb(&infop->tm_dav, index);
	} else {
		/*
		 * Fill in the CB to perform a read and start 
		 * by notifying the SSM.  
		 */
		iobp->ci_nblks = btodb(infop->tm_bufsize);
		iobp->ci_err_flag = 0;
		infop->tm_blkno += iobp->ci_nblks;

		bzero(SWBZERO(cb), SWBZERO_SIZE);
		cb->sh.cb_cmd = SCB_READ | SCB_IENABLE;
		cb->sh.cb_clen = SCSI_CMD6SZ;
		cb->sh.cb_scmd[0] = SCSI_READ;
		cb->sh.cb_scmd[1] = 
			infop->tm_devno << SCSI_LUNSHFT | infop->tm_rwbits;
		cb->sh.cb_scmd[2] = (u_char)(iobp->ci_nblks >> 16);
		cb->sh.cb_scmd[3] = (u_char)(iobp->ci_nblks >> 8);
		cb->sh.cb_scmd[4] = (u_char)iobp->ci_nblks;
		cb->sh.cb_addr = (u_long)iobp->ci_buffer;
		cb->sh.cb_iovec = cb->sw.cb_iovstart;
		cb->sh.cb_count = infop->tm_bufsize;
		cb->sh.cb_compcode = SCB_BUSY;

		tm_q_cb(&infop->tm_active, index);
		s = splhi();
		mIntr(infop->tm_ssm->ssm_slicaddr, SCSI_BIN, 
			cb->sw.cb_unit_index);
		splx(s);
	}
}
 
/*
 * tm_raw_read()
 *	If the device's current mode is for reading 
 *	then mark set the last-io-read fflag and 
 *	start up as many reads as there are buffers
 *	and command blocks for.  Then wait for the 
 *	reads to complete and copy the data to the 
 *	buf structure. If the device needs to be
 *	retentioned, do so before starting the 
 *	reads.  If the device's current mode is for 
 *	writing, generate an error to the user level.
 */
tm_raw_read(infop, bp)
	register struct tm_info *infop;
	struct buf *bp;
{
	register struct tm_iobuf *iobp;
	caddr_t cp;
	long avail;
	unsigned n;
	int status, index;

#ifdef DEBUG
	ASSERT(infop, "tm_raw_read: NULL info-structure pointer");
	ASSERT(bp, "tm_raw_read: NULL buf-structure pointer");
#endif DEBUG

	infop->tm_spl = p_lock(&infop->tm_lock, TM_SPL);

	/*
 	 * Proceed based on the device's current mode.
	 */
	switch (infop->tm_curmode) {
	case TMM_WRITE:
		/* Illegal read after write; generate error to the user. */
		uprintf("tm%d: illegal read after write\n",
			infop - tm_info);
		/*
		 *+ The tm driver does not support reading
	  	 *+ immediately following a write.  There
		 *+ is no data to read until the tape is
		 *+ rewound.
		 */
		bp->b_resid = bp->b_bcount,
		bp->b_flags |= B_ERROR;
		infop->tm_resid = bp->b_resid;
		v_lock(&infop->tm_lock, infop->tm_spl);
		return;
	case TMM_GENERAL:
		/* Perform retentioning if the device needs it */
		if (infop->tm_fflags & TMF_ATTEN 
		&&  infop->tm_cflags & TMC_AUTORET) 
			(void) gen_scsi_cmd(SCSI_LOAD_UNLOAD, infop, 0, 
				SCSI_LOAD_MEDIA | SCSI_RETEN_MEDIA, TM_WAIT);
		infop->tm_fflags &= ~TMF_ATTEN;
		infop->tm_curmode = TMM_READ;
		/* Fall through to the TMM_READ case... */
	case TMM_READ:
		/* 
		 * Note: indicate that READ has occurred
		 * to force space-to-eof upon closure
		 * if not at the end-of-file mark.
	 	 */
		infop->tm_fflags |= TMF_LASTIOR;
		bp->b_resid = bp->b_bcount;
		cp = bp->b_un.b_addr;

		while (bp->b_resid) {
			ASSERT(bp->b_resid > 0, 
				"tm_raw_read: bp->b_resid < 0");
			/*
			 *+ The tm driver detected a buf structure from
			 *+ physio that contains an invalid, negative residual
			 *+ count or that the driver has over-decremented.
			 */

			if (infop->tm_free.sq_count == NCBPERSCSI) {
				/* 
				 * Prime the read/readahead, since there 
				 * is none currently active.
				 */
				index = tm_pop_cb(&infop->tm_free, 
							infop, TM_LOCKED);
#ifdef DEBUG
				ASSERT(index >= 0 && index < NCBPERSCSI, 
					"tm_raw_read: bad CB index.\n");
#endif DEBUG
				tm_start_read(index, infop);
			}

			/*
			 * Await data availability from a terminated
			 * READ command on the infop->tm_dav queue 
			 * and process the available status/data.  
			 */
			index = tm_pop_cb(&infop->tm_dav, infop, TM_UNLOCK);

#ifdef DEBUG
			ASSERT(index >= 0 && index < NCBPERSCSI, 
				"tm_raw_read: bad CB index");
#endif DEBUG

			iobp = infop->tm_iobuf + index;


			/* 
			 * If a device failure occurred don't attempt 
			 * reading the data; propagate the error to user 
			 * level and change the open flags so that
			 * tmstrat() does not attempt further I/O
			 * until the device is closed and re-opened.
			 *
			 * Note: an end-of-file is conveyed by a
			 * zero transfer count, not a failure.
			 * 
		 	 */
			if (iobp->ci_err_flag) {
				bp->b_error |= B_ERROR;
				infop->tm_openf = TMO_ERR;
				tm_q_cb(&infop->tm_free, index);
				v_lock(&infop->tm_lock, infop->tm_spl);
				return;
			} 

			infop->tm_blkno += iobp->ci_nblks;

			/*
			 * Determine the number of bytes available
			 * to be copied to the buf-structure.
			 * If the number of bytes to copyout is zero, 
			 * an end-of-file occurred and no more data
			 * is available from the file.
			 */
			avail = dbtob(iobp->ci_nblks) - iobp->ci_curbyte;

#ifdef DEBUG
			ASSERT(avail >= 0, 
				"tm_raw_read: READ bytes available < 0");
			ASSERT(avail <= infop->tm_bufsize, 
				"tm_raw_read: READ bytes available > buffer");
#endif DEBUG

			if (!(n = min((unsigned)bp->b_resid,(unsigned)avail))) {
				/* 
				 * End-of-file has been encountered.
				 * Return a zero read count on this or 
				 * the next read, then allow reads to
				 * go into the next file.
				 */
				if (bp->b_resid == bp->b_bcount)
					infop->tm_fflags &= 
						~(TMF_EOF | TMF_LASTIOR);
				infop->tm_resid = bp->b_resid;
				tm_q_cb(&infop->tm_free, index);
				v_lock(&infop->tm_lock, infop->tm_spl);
				return;
			} 

			/*
			 * Attempt to copy n-bytes of buffered data from 
			 * the CB to the buf-struct data area.  Upon error 
			 * make the data from the CB available again, and 
			 * propagate the error to the user level.
			 * 
			 * Unlock and re-lock the info structure around 
			 * the copyout, since it may sleep.
			 */
			v_lock(&infop->tm_lock, infop->tm_spl);
			status = copyout(iobp->ci_buffer + iobp->ci_curbyte, 
					cp,  n);
			infop->tm_spl = p_lock(&infop->tm_lock, TM_SPL);

			if (status) {
				bp->b_flags |= B_ERROR;
				tm_push_cb(&infop->tm_dav, index);
				v_lock(&infop->tm_lock, infop->tm_spl);
				return;	
			}

			/* 
			 * Update position of next available data in 
			 * the CB buffer, the amount of data to read,
			 * and where to put it in the buf structure. 
			 *
			 * If all the buffered data has been read then 
			 * start the CB reading ahead again.  Otherwise
			 * put it back on the infop->tm_dav queue.
			 */
			bp->b_resid -= n;
			cp += n;
			iobp->ci_curbyte += n;
			infop->tm_iocount +=n;	/* For statistics */
			infop->tm_resid = bp->b_resid;

#ifdef DEBUG
			ASSERT(iobp->ci_curbyte <= dbtob(iobp->ci_nblks),
				"tm_raw_read: invalid position in buffer");
#endif DEBUG

			if (iobp->ci_curbyte == dbtob(iobp->ci_nblks))
				tm_q_cb(&infop->tm_free, index);
			else
				tm_push_cb(&infop->tm_dav, index);
		}
		break;
	}
	v_lock(&infop->tm_lock, infop->tm_spl);
}

/* tm_start_write()
 * 	Build a SCSI WRITE command in infop->tm_cbs[index]
 *	Start the CB and return.  
 *
 *	Assume that the info structure is locked 
 *	upon entry.
 */
static
tm_start_write(index, infop)
	register int index;
	register struct tm_info *infop;
{
	register struct scsi_cb *cb;
	register struct tm_iobuf *iobp;
	spl_t s;

#ifdef DEBUG
	ASSERT(infop, "tm_start_write: NULL info-structure pointer");
	ASSERT(index >= 0 && index < NCBPERSCSI, 
		"tm_start_write: bad CB index");
#endif DEBUG

	iobp = infop->tm_iobuf + index;
	cb = infop->tm_cbs + index;

	/*
	 * Fill in the CB to perform a read and 
	 * notify the SSM.
	 */
	bzero(SWBZERO(cb), SWBZERO_SIZE);
	cb->sh.cb_cmd = SCB_WRITE | SCB_IENABLE;
	cb->sh.cb_clen = SCSI_CMD6SZ;
	cb->sh.cb_scmd[0] = SCSI_WRITE;
	cb->sh.cb_scmd[1] = 
		infop->tm_devno << SCSI_LUNSHFT | infop->tm_rwbits;
	cb->sh.cb_scmd[2] = (u_char)(iobp->ci_nblks >> 16);
	cb->sh.cb_scmd[3] = (u_char)(iobp->ci_nblks >> 8);
	cb->sh.cb_scmd[4] = (u_char)iobp->ci_nblks;
	cb->sh.cb_addr = (u_long)iobp->ci_buffer;
	cb->sh.cb_iovec = cb->sw.cb_iovstart;
	cb->sh.cb_count = dbtob(iobp->ci_nblks);
	cb->sh.cb_compcode = SCB_BUSY;

	tm_q_cb(&infop->tm_active, index);
	/*
	 * Allow the SSM to only be notified
	 * of one active CB at a time so that
	 * EOM early warning doesn't corrupt
	 * data ordering.  The ISR will activate 
	 * the next one when the first one terminates.
	 */
	if (infop->tm_active.sq_count == 1) {
		s = splhi();
		mIntr(infop->tm_ssm->ssm_slicaddr, SCSI_BIN, cb->sw.cb_unit_index);
		splx(s);
	}
}
 
/*
 * tm_raw_write()
 *	If the device's current mode is for writing set 
 *	the last-io-write fflag. Locate a CB/buffer to 
 *	write the data into. If it fills, start a SCSI 
 *	write on that buffer and then attempt to find 
 *	another CB/buffer to continue buffering the data.
 *	Otherwise, leave the index of the partially
 *	filled CB/buffer in infop->tm_part_write so it
 *	can be used by the next invocation.
 *
 *	If the device needs to be erased/retensioned, 
 *	do so before buffering the data.  If the device's 
 *	current mode is for reading, generate an error 
 *	to the user level.
 */
tm_raw_write(infop, bp)
	register struct tm_info *infop;
	struct buf *bp;
{
	register struct tm_iobuf *iobp;
	caddr_t cp;
	long avail;
	unsigned n;
	int index, status;

#ifdef DEBUG
	ASSERT(infop, "tm_raw_write: NULL info-structure pointer");
	ASSERT(bp, "tm_raw_write: NULL buf-structure pointer");
#endif DEBUG

	infop->tm_spl = p_lock(&infop->tm_lock, TM_SPL);

	/*
	 * Action is based on current mode.
	 */
	switch (infop->tm_curmode) {
	case TMM_READ:
		/* Illegal write after read; generate error to the user. */
		uprintf("tm%d: illegal write after read\n",
			infop - tm_info);
		/*
		 *+ The tm driver does not support writing
		 *+ immediately after reading.  Writes can
		 *+ occur only at the beginning of the medium
		 *+ (in which case all former data is erased)
		 *+ and at the logical end of data (in which case the
		 *+ new data is appended).
		 */
		bp->b_resid = bp->b_bcount,
		bp->b_flags |= B_ERROR;
		v_lock(&infop->tm_lock, infop->tm_spl);
		return;
	case TMM_GENERAL:
		/* 
		 * If the device is requesting attention after 
		 * a media change or reset and is configured for 
		 * auto-retentioning, erase the tape before writing.
		 */
		if (infop->tm_fflags & TMF_ATTEN 
		&&  infop->tm_cflags & TMC_AUTORET) 
			(void) gen_scsi_cmd(SCSI_ERASE, infop, 0, 0, TM_WAIT);
		infop->tm_fflags &= ~TMF_ATTEN;
		infop->tm_curmode = TMM_WRITE;
		/* Fall through to the TMM_WRITE case... */
	case TMM_WRITE:
		/* 
		 * Buffer the data to be written latter.
		 * If the buffer fills, start writing it
		 * out and continue when another is available.
		 * Set TMF_LASTIOW to indicate that a filemark
		 * needs to be written after the data.
		 */
		infop->tm_fflags |= TMF_LASTIOW;
		bp->b_resid = bp->b_bcount;
		cp = bp->b_un.b_addr;

		while (bp->b_resid) {
			ASSERT(bp->b_resid > 0, 
				"tm_raw_write: bp->b_resid < 0");
			/*
		 	 *+ The tm driver detected a buf structure from
			 *+ physio that contains an invalid, negative residual
			 *+ count or that the driver has over-decremented.
			 */
			/* 
			 * Locate a CB/buffer to buffer the output data 
			 * into.  If there is a partial one from a previous 
			 * write use it.  Otherwise wait for one to use 
			 * from the infop->tm_free queue.  Since there may 
			 * be delayed writes active, check for device 
			 * failure after locating the CB.
			 */
			if ((index = infop->tm_part_write) >= 0) {
				infop->tm_part_write = -1;
#ifdef DEBUG
				ASSERT(index >= 0 && index < NCBPERSCSI, 
					"tm_raw_write: bad partial CB index");
#endif DEBUG
				iobp = infop->tm_iobuf + index;
			} else {
				index = tm_pop_cb(&infop->tm_free, infop,
				 	TM_UNLOCK);
#ifdef DEBUG
				ASSERT(index >= 0 && index < NCBPERSCSI, 
					"tm_raw_write: bad CB index");
#endif DEBUG
				iobp = infop->tm_iobuf + index;
				iobp->ci_curbyte = 0;
			}

			/* 
			 * In the sequence that follows, note that
			 * when EOM is set in tm_fflags, but FAIL
			 * is not, the early warning for end-of-media 
			 * has been encountered.  The user is notified
			 * by terminating a write with less than the
			 * requested amount of data written, upon which
			 * the EOF flag is set indicating notification
			 * has been given.  Subsequent attempts to 
			 * write without resetting media (which clears
			 * these flags) result in an error being reported.
			 * Note: using EOF instead of setting FAIL after
			 * giving notice allows buffered data to be
			 * flushed and filemarks to be written.
			 */ 
			if ((infop->tm_fflags & (TMF_FAIL|TMF_EOM)) == TMF_EOM){
				if (infop->tm_fflags & TMF_EOF) {
					/* 
					 * Attempt to write beyond
					 * permitted boundary.
					 */
					bp->b_error = ENOSPC;
					bp->b_flags |= B_ERROR;
				} else {
					/* 
					 * Not all data has been 
					 * written. Terminate I/O
					 * to notify of EOM and note
					 * that its been reported.
					 */
					infop->tm_fflags |= TMF_EOF;
				}
				if (iobp->ci_curbyte)
					infop->tm_part_write = index;
				else
					tm_q_cb(&infop->tm_free, index);
				v_lock(&infop->tm_lock, infop->tm_spl);
				return;
			}
			/* 
			 * If a device failure occurred don't attempt 
			 * writing the data; propagate the error to user 
			 * level and change the open flags so that
			 * tmstrat() does not attempt further I/O
			 * until the device is closed and re-opened.
		 	 */
			if (infop->tm_fflags & TMF_FAIL) {
				bp->b_error |= B_ERROR;
				infop->tm_openf = TMO_ERR;
				tm_q_cb(&infop->tm_free, index);
				v_lock(&infop->tm_lock, infop->tm_spl);
				return;
			} 

			/*
			 * Attempt to move n-bytes of data from the buf-
			 * struct data area to the i/o buffer.  If an 
			 * error occurs then mark the device failure in 
			 * fflags, put the current CB on the infop->tm_free 
			 * queue, and terminate the write with an error to 
			 * the user level.
			 * 
			 * Unlock and re-lock the info structure around the 
			 * copyout, since it may sleep.
			 */
			avail = infop->tm_bufsize - iobp->ci_curbyte;
#ifdef DEBUG
			ASSERT(avail >= 0, 
				"tm_raw_write: buffer bytes available < 0");
#endif DEBUG
			n = min((unsigned)bp->b_resid, (unsigned)avail);

			v_lock(&infop->tm_lock, infop->tm_spl);
			status = copyin(cp, iobp->ci_buffer + 
					iobp->ci_curbyte, n);
			infop->tm_spl = p_lock(&infop->tm_lock, TM_SPL);

			if (status) {
				/*
				 * Propagate the error to user level.  If 
				 * CB/buffer is partially filled save its 
				 * index where a retry can access it - its 
				 * data is not corrupt.  Otherwise free it.
				 */
				bp->b_flags |= B_ERROR;
				if (iobp->ci_curbyte)
					infop->tm_part_write = index;
				else
					tm_q_cb(&infop->tm_free, index);
				v_lock(&infop->tm_lock, infop->tm_spl);
				return;
			}
			
			/*
			 * Update the buffer location for the next data 
			 * to be copied in.  If the buffer is full, start 
			 * writing it out.  Otherwise save the index of 
			 * the partial CB/buffer where the next iteration/
			 * invocation can access it.
			 */
			cp += n;
			bp->b_resid -= n;
			infop->tm_iocount +=n;	/* For statistics */
			iobp->ci_curbyte += n;
			iobp->ci_nblks = btodb(
				roundup(iobp->ci_curbyte, dbtob(1)));
			if (iobp->ci_curbyte == infop->tm_bufsize) 
			       tm_start_write(index, infop);
			else
				infop->tm_part_write = index;
		}
		break;
	}
	v_lock(&infop->tm_lock, infop->tm_spl);
}

/*
 * tm_io_flush()
 *	Wait for outstanding I/O to terminate.
 *	Return 1 for success, zero for failure.
 */
static
tm_io_flush(infop)
	register struct tm_info *infop;
{
	register struct tm_iobuf *iobp;
	int i, index;
	struct scrsense *sensebuf;
	u_char *cp;
	u_int count;
	char saved_openf;

#ifdef DEBUG
	ASSERT(infop, "tm_io_flush: NULL info-structure pointer");
#endif DEBUG

	/* 
	 * Force the write and read and
	 * interfaces to shut down activity
	 */
	saved_openf = infop->tm_openf;	
	infop->tm_openf = TMO_FLUSH;

	/* Determine the mode of operation and what to wait for. */
	switch (infop->tm_curmode) {
	case TMM_READ:
		/* 
		 * Delay until outstanding READ commands 
		 * complete. As they terminate they will
		 * appear on the infop->tm_dav queue; 
		 * return them to the infop->tm_free queue.  
		 * They are all terminated when the free 
		 * queue count is NCBPERSCSI.
		 */
		while (infop->tm_free.sq_count < NCBPERSCSI) {
			index = tm_pop_cb(&infop->tm_dav, infop, TM_UNLOCK);
#ifdef DEBUG
			ASSERT(index >= 0 && index < NCBPERSCSI, 
				"tm_io_flush: bad CB index");
#endif DEBUG
			tm_q_cb(&infop->tm_free, index);
		}

		if (infop->tm_cflags & (TMC_PRSENSE | TMC_RSENSE)) {
			/* 
			 * Display error summary information 
			 * about the data read.
			 */
#ifdef DEBUG
			ASSERT(infop->tm_free.sq_count
				== NCBPERSCSI, "tm_io_flush: empty free-q");
#endif DEBUG
			index = infop->tm_free.sq_head;
			(void) gen_scsi_cmd(SCSI_RSENSE, infop, 0, 0, TM_WAIT);
			sensebuf = (struct scrsense *) 
				infop->tm_cbs[index].sh.cb_sense;
			cp = ((u_char *) sensebuf) + 
				((sensebuf->rs_addlen == 3) ? 9 : 12);
			count = (u_int) *cp << 8 | (u_int) *(cp + 1);
			if (count) {
				CPRINTF("tm%d: read ", infop - tm_info);
				CPRINTF("%d blocks with %d recoverable errors\n",
					dbtob(infop->tm_iocount), count);
			}
			infop->tm_iocount = 0;
		}
		break;
	case TMM_WRITE:
		/* 
		 * If a CB is partially filled with valid
		 * unwritten data, pad the partial block 
		 * in the buffer with zeroes and write it.
		 */
		if ((index = infop->tm_part_write) >= 0) {
			infop->tm_part_write = -1;
			iobp = infop->tm_iobuf + index;
			bzero(iobp->ci_buffer + iobp->ci_curbyte,
			        dbtob(iobp->ci_nblks) - iobp->ci_curbyte);
			tm_start_write(index, infop);
		}

		/* 
		 * Wait for pending writes to complete.
		 * They are all done when they have been
		 * moved/appended to the infop->tm_free
		 * queue.  Therefore, just aquire CB/buffers 
		 * from that queue until all are aquired,
		 * then put them back.
		 */
		for (i = NCBPERSCSI; i--; ) {
			index = tm_pop_cb(&infop->tm_free, infop, TM_UNLOCK);
#ifdef DEBUG
			ASSERT(index >= 0 && index < NCBPERSCSI, 
				"tm_io_flush: bad CB index");
#endif DEBUG
		}
		for (index = 0; index < NCBPERSCSI; index++) 
			tm_q_cb(&infop->tm_free, index);

		if (infop->tm_cflags & (TMC_PRSENSE | TMC_WSENSE)) {
			/*
			 * Display error summary information 
			 * about the data written.  Locate the
			 * CB that will be used for fetching 
			 * the sense data first.
			 */
#ifdef DEBUG
			ASSERT(infop->tm_free.sq_count
				== NCBPERSCSI, "tm_io_flush: empty free-q");
#endif DEBUG
			index = infop->tm_free.sq_head;
			(void) gen_scsi_cmd(SCSI_RSENSE, infop, 0, 0, TM_WAIT);
			sensebuf = (struct scrsense *) 
				infop->tm_cbs[index].sh.cb_sense;
			cp = ((u_char *) sensebuf) + 
				((sensebuf->rs_addlen == 3) ? 9 : 12);
			count = (u_int) *cp << 8 | (u_int) *(cp + 1);
			if (count) {
				CPRINTF("tm%d: wrote ", infop - tm_info);
				CPRINTF("%d blocks with %d recoverable errors\n",
					dbtob(infop->tm_iocount), count);
			}
			infop->tm_iocount = 0;
		}

		/* Notify the user level of any errors that occurred. */
		if (infop->tm_fflags & TMF_FAIL) {
			uprintf("tm%d: error writing buffer to tape\n",
				infop - tm_info);
			/*
			 *+ The tm driver received a bad command
			 *+ completion status while attempting to
			 *+ flush its buffered output to the tape.
			 *+ Probable causes are: a) medium protected
			 *+ against writes, b) medium not positioned
			 *+ to beginning of tape or logical end of
			 *+ data, c) invalid medium type for this
			 *+ drive, d) attempt to write past the
			 *+ physical end of the medium.
			 */
			infop->tm_openf = saved_openf;
			return (0);
		}
		break;
	case TMM_GENERAL:
		break;
	}
	infop->tm_openf = saved_openf;
	return (1);
}
 
/*
 * tm_iobuf_init()
 *	Locate the iobuf information associated with each 
 *	CB and attempt to allocate non-paged memory for its 
 *	i/o buffer.  If the allocation succeeds initialize 
 *	the descriptor information associated with it and 
 *	fill in its dma map.  If allocation fails deallocate 
 *	all resources allocated up to that point and return 
 *	an error (0).  Upon success return 1.
 *
 * 	Assume the devices info structure is not locked 
 * 	upon entry; wmemall() could sleep holding the
 *	lock otherwise.
 */
static int
tm_iobuf_init(infop)
	struct tm_info *infop;
{
	register u_long *maps, bufaddr;
	struct tm_iobuf *iobp;
	int i, j;

#ifdef DEBUG
	ASSERT(infop, "tm_iobuf_init: NULL info-structure pointer");
#endif DEBUG

	/*
	 * Fetch and initialize the next iobuf description. 
	 */
	for (iobp = infop->tm_iobuf, i = NCBPERSCSI; i--; iobp++) {

		iobp->ci_buffer = (caddr_t) wmemall(infop->tm_bufsize, 
			(infop->tm_cflags & TMC_OPENFAIL) ? 0 : 1);

		if (!(bufaddr = (u_long)iobp->ci_buffer)) {
			/* Out of memory, free all aquired resources */
			for (i++, iobp--; i++ < NCBPERSCSI; iobp--)
				wmemfree(iobp->ci_buffer, infop->tm_bufsize);
			return (0);
		}

		/* Initialize the iobuffer state. */
		iobp->ci_err_flag = 0;
		iobp->ci_nblks = 0;
		iobp->ci_curbyte = 0;

		/* Fill in its dma map table.  */
		for (j = iobp->ci_nmaps, maps = iobp->ci_maps; j--; 
		     maps++, bufaddr += CLBYTES) 
			*maps = PTETOPHYS(Sysmap[btop(bufaddr & 
				~(CLBYTES - 1))]) + (bufaddr & CLBYTES - 1);
	}
	return (1);
}
 
/*
 * tm_iobuf_free()
 * 	Deallocate the i/o buffer space associated with 
 *	the CBs for the device whose info structure address 
 *	is an argument to this function.
 */
tm_iobuf_free(infop)
	register struct tm_info *infop;
{
	register struct tm_iobuf *iobp;
	register int i;

#ifdef DEBUG
	ASSERT(infop, "tm_iobuf_free: NULL info-structure pointer");
#endif DEBUG

	for (iobp = infop->tm_iobuf, i = NCBPERSCSI; i--; iobp++) {
#ifdef DEBUG
		ASSERT(iobp->ci_buffer, 
			"tm_iobuf_free: NULL buffer pointer");
#endif DEBUG
		wmemfree(iobp->ci_buffer, infop->tm_bufsize);
	}
}
 
/*
 * gen_scsi_cmd()
 *	Fill out a CB for the specified SCSI command 
 *	and issue the command to the SSM.  If the 
 *	'delay' argument is TM_WAIT then sleep until 
 *	its completion and return 1 upon successful 
 *	completion, zero otherwise.  If the 'delay' 
 *	argument is TM_NOWAIT then return 1.
 *
 *	Assumes that the info structure is locked 
 *	upon entry.
 *
 *	Note: opcount pertains only to the SCSI_WFM,
 *	SCSI_MODES, and  SCSI_SPACE commands.  qualifier 
 *	pertains to the TM_UNLOAD_LOAD command (either
 *	 'load' or 'unload'), the SCSI_SPACE command 
 *	(qualifies 'end-of-data' or 'filemarks'), and 
 *	SCSI_MODES command (specifies tape density).
 */		
static int
gen_scsi_cmd(cmd, infop, opcount, qualifier, delay)
	u_char cmd;
	register struct tm_info *infop;
	int opcount;
	u_int qualifier;
	u_char delay;
{
	register struct scsi_cb *cb;
	int index, temp, saved_atten;
	spl_t s;

#ifdef DEBUG
	ASSERT(infop, "tm gen_scsi_cmd: NULL info-structure pointer");
#endif DEBUG

	/*
	 * Aquire a CB for this command.  If necessary, 
	 * wait for one to become available.
	 */
	index = tm_pop_cb(&infop->tm_free, infop, TM_UNLOCK);

#ifdef DEBUG
	ASSERT(index >= 0 && index < NCBPERSCSI, "gen_scsi_cmd: bad CB index");
#endif DEBUG

	cb = infop->tm_cbs + index;

	/*
	 * Clear the shared portion of the cb, except 
	 * the address and size of the request-sense 
	 * buffer.
	 */
	bzero(SWBZERO(cb), SWBZERO_SIZE);

	/* 
	 * Perform SCSI command specific initialization.
	 */
	switch (cb->sh.cb_scmd[0] = cmd) {
	case SCSI_ERASE:
		infop->tm_fflags &= ~(TMF_FAIL | TMF_EOF | TMF_EOM 
			  	       | TMF_LASTIOR | TMF_LASTIOW);
		cb->sh.cb_cmd = SCB_READ;
		cb->sh.cb_clen = SCSI_CMD6SZ;
		cb->sh.cb_scmd[1] = 
			infop->tm_devno << SCSI_LUNSHFT | infop->tm_rwbits;
		break;
	case SCSI_LOAD_UNLOAD:
		infop->tm_fflags &=
			~(TMF_FAIL | TMF_EOF | TMF_EOM | TMF_LASTIOR);
		cb->sh.cb_cmd = SCB_READ;
		cb->sh.cb_clen = SCSI_CMD6SZ;
		cb->sh.cb_scmd[1] = infop->tm_devno << SCSI_LUNSHFT;
		cb->sh.cb_scmd[4] = qualifier;
		break;
	case SCSI_REWIND:
		infop->tm_fflags &= ~(TMF_FAIL | TMF_EOF | TMF_EOM 
				       | TMF_LASTIOR | TMF_LASTIOW);
		cb->sh.cb_cmd = SCB_READ;
		cb->sh.cb_clen = SCSI_CMD6SZ;
		cb->sh.cb_scmd[1] = infop->tm_devno << SCSI_LUNSHFT;
		break;
	case SCSI_INQUIRY:
		infop->tm_fflags &= ~TMF_FAIL;
		cb->sh.cb_cmd = SCB_READ;
		cb->sh.cb_clen = SCSI_CMD6SZ;
		cb->sh.cb_addr = cb->sh.cb_sense;
		cb->sh.cb_count = sizeof(struct scinq);
		cb->sh.cb_scmd[1] = infop->tm_devno << SCSI_LUNSHFT;
		cb->sh.cb_scmd[4] = sizeof(struct scinq);
		break;
	case SCSI_MODES:
		infop->tm_fflags &= ~TMF_FAIL;
		cb->sh.cb_cmd = SCB_WRITE;
		cb->sh.cb_clen = SCSI_CMD6SZ;
		cb->sh.cb_scmd[1] = infop->tm_devno << SCSI_LUNSHFT;
		cb->sh.cb_scmd[4] = TM_MSEL_PARMLEN + 
			((infop->tm_type == TMT_TANDBERG) ? 1 : 0);

		/*
		 * This is a one-shot per device operation, 
		 * so use the request-sense buffer for the 
		 * mode selection parameters.
		 */
		bzero((char *)(cb->sh.cb_addr = cb->sh.cb_sense), 
			(u_int)(cb->sh.cb_count = (u_long)cb->sh.cb_scmd[4]));
		((u_char *)cb->sh.cb_addr)[2] = MSEL_BFM_ASYNC;
		((u_char *)cb->sh.cb_addr)[3] = SCSI_MODES_DLEN;	
		((u_char *)cb->sh.cb_addr)[4] = (u_char)qualifier; /* density */
		((u_char *)cb->sh.cb_addr)[10] = 2;	/* Block size 0x200 */
		/* 
		 * Suppress check condition reports of 
		 * recoverable errors on Emulex drives.
		 */
		if (infop->tm_type == TMT_EMULEX)
			((u_char *)cb->sh.cb_addr)[12] = 1;	
		break;
	case SCSI_WFM:
		infop->tm_fflags &= ~(TMF_FAIL | TMF_LASTIOR);
		cb->sh.cb_cmd = SCB_READ;
		cb->sh.cb_clen = SCSI_CMD6SZ;
		cb->sh.cb_scmd[1] = infop->tm_devno << SCSI_LUNSHFT;
		cb->sh.cb_scmd[2] = (u_char)(opcount >> 16);
		cb->sh.cb_scmd[3] = (u_char)(opcount >> 8);
		cb->sh.cb_scmd[4] = (u_char)opcount;
		break;
	case SCSI_SPACE:
		if (qualifier == SCSI_SPACE_FILEMARKS) {
			if (infop->tm_fflags & TMF_EOM)
				opcount = 0;	/* Force return below */
			if (infop->tm_fflags & TMF_EOF) {
				opcount--;
				infop->tm_fflags &= 
					~(TMF_FAIL | TMF_EOF | TMF_LASTIOR);
			}
			if (opcount <= 0) {
				/* 
				 * Free the CB already allocated and 
				 * return good status 
				 */
				tm_q_cb(&infop->tm_free, index);
				return (1);
			}
		}
		infop->tm_fflags &= ~TMF_FAIL;
		cb->sh.cb_cmd = SCB_READ;
		cb->sh.cb_clen = SCSI_CMD6SZ;
		cb->sh.cb_scmd[1] = 
			infop->tm_devno << SCSI_LUNSHFT | qualifier;
		cb->sh.cb_scmd[2] = (u_char)(opcount >> 16);
		cb->sh.cb_scmd[3] = (u_char)(opcount >> 8);
		cb->sh.cb_scmd[4] = (u_char)opcount;
		break;
	case SCSI_TEST:
		saved_atten = infop->tm_fflags & TMF_ATTEN;
		infop->tm_fflags &= ~(TMF_FAIL | TMF_ATTEN);
		cb->sh.cb_cmd = SCB_READ;
		cb->sh.cb_clen = SCSI_CMD6SZ;
		cb->sh.cb_scmd[1] = infop->tm_devno << SCSI_LUNSHFT;
		break;
	case SCSI_RSENSE:
		cb->sh.cb_cmd = SCB_READ;
		cb->sh.cb_clen = SCSI_CMD6SZ;
		cb->sh.cb_addr = cb->sh.cb_sense;
		cb->sh.cb_count = cb->sh.cb_slen;
		cb->sh.cb_scmd[1] = infop->tm_devno << SCSI_LUNSHFT;
		cb->sh.cb_scmd[4] = cb->sh.cb_slen;
		break;
	default:
		/* 
		 * Free the CB already allocated and 
		 * return failure.
		 */
		tm_q_cb(&infop->tm_free, index);
		return (0);		/* Invalid command */
	}

	/*
	 * Complete cmd-independent initialization of the 
	 * CB, put it on the infop->tm_active queue,  and 
	 * notify the SSM to process it.  
	 */
	cb->sh.cb_cmd |= SCB_IENABLE;	
	cb->sh.cb_compcode = SCB_BUSY;
	tm_q_cb(&infop->tm_active, index);
	s = splhi();
	mIntr(infop->tm_ssm->ssm_slicaddr, SCSI_BIN, cb->sw.cb_unit_index);
	splx(s);

	/*
	 * If 'delay' is TM_NOWAIT then return (1) immediately;
	 * the termination will be checked later.
	 * Otherwise, await the termination of the CB by waiting
	 * for the ISR to move it to the infop->tm_gen_cmd
	 * queue.  The CB is deleted from there, its status 
	 * handled, and it is freed.
	 */
	if (delay == TM_NOWAIT) 
		return (1);
	temp = tm_pop_cb(&infop->tm_gen_cmd, infop, TM_UNLOCK);
	ASSERT(temp == index, "gen_scsi_cmd: unexpected CB termination order");
	/*
	 *+ SCSI commands from the tm driver that were being executed
	 *+ by the SSM did not complete in sequential order.
	 *+ This indicates a potential problem with the SSM.
	 */

	/* Command specific termination processing */
	switch (cmd) { 
	case SCSI_ERASE:
	case SCSI_REWIND:
	case SCSI_LOAD_UNLOAD:
	case SCSI_MODES:
		if (infop->tm_fflags & TMF_FAIL) {
			tm_q_cb(&infop->tm_free, index);
			return (0);	/* Notify caller of failure */
		}
		infop->tm_curmode = TMM_GENERAL;
		break;
	case SCSI_INQUIRY:
		if (infop->tm_fflags & TMF_FAIL) {
			tm_q_cb(&infop->tm_free, index);
			return (0);	/* Notify caller of failure */
		}
		/* Set the device type based on inquiry data. */
		if (((struct scinq *)cb->sh.cb_sense)->sc_vlength)
			infop->tm_type = TMT_TANDBERG;
		else
			infop->tm_type = TMT_EMULEX;
		break;
	case SCSI_TEST:
		if ((infop->tm_fflags |= saved_atten) & TMF_ATTEN) 
			infop->tm_fflags &= ~(TMF_EOF | TMF_EOM | TMF_LASTIOR);
		if (infop->tm_fflags & TMF_FAIL) {
			tm_q_cb(&infop->tm_free, index);
			return (0);	/* Notify caller of failure */
		}
		break;
	case SCSI_WFM:
		if (infop->tm_fflags & TMF_FAIL) {
			tm_q_cb(&infop->tm_free, index);
			return (0);	/* Notify caller of failure */
		}
		infop->tm_curmode = TMM_WRITE;
		break;
	case SCSI_SPACE:
		if (infop->tm_fflags & TMF_FAIL) {
			tm_q_cb(&infop->tm_free, index);
			return (0);	/* Notify caller of failure */
		}
		if (qualifier == SCSI_SPACE_ENDOFDATA) {
			infop->tm_curmode = TMM_GENERAL;
			infop->tm_fflags |= TMF_EOM | TMF_EOF;
		} else
			infop->tm_curmode = TMM_READ;
		break;
	case SCSI_RSENSE:
		break;
	}

	/* Free the CB and return good status */
	tm_q_cb(&infop->tm_free, index);
	return (1);
}

/*
 * tm_check_condition()
 *	Determine if the specified SCSI command 
 *	termination was due to a check condition.  
 *	If so, analyze the request sense information
 *	associated with it setting state flags for 
 *	the device accordingly and saving error 
 *	message information about it.  This 
 *	information may be used elsewhere.
 *	
 *	Assumes the info-structure is locked by
 * 	the caller.
 *
 *	Returns 1 if there is a check-condition,
 *	zero otherwise.
 */
static int
tm_check_condition(infop, cb, iobp, resid)
	register struct tm_info *infop;
	register struct scsi_cb *cb;
	register struct tm_iobuf *iobp;
	long resid;
{
	struct scrsense *sensebuf;

#ifdef DEBUG
	ASSERT(infop, "tm_check_condition: NULL info-structure pointer");
	ASSERT(cb, "tm_check_condition: NULL CB pointer");
	ASSERT(iobp, "tm_check_condition: NULL CB/buffer descriptor pointer");
#endif DEBUG

	sensebuf = (struct scrsense *)cb->sh.cb_sense;

	if (SCSI_CHECK_CONDITION(cb->sh.cb_status)) {
		if ((sensebuf->rs_error & RS_ERRCODE) != RS_CURERR) {
			/* 
			 * Not a current error; use the driver 
			 * dependent sensekey and minimal length.
			 */
			infop->tm_fflags |= TMF_FAIL;
			iobp->ci_key = RS_RES;
			iobp->ci_slen = sizeof(struct scrsense);
		} else {
			/*
			 * Determine current error type 
			 * and take action.
			 */
			if (sensebuf->rs_error & RS_VALID) {
				resid = (long)sensebuf->rs_info[0] << 24;
				resid |= (long)sensebuf->rs_info[1] << 16;
				resid |= (long)sensebuf->rs_info[2] << 8;
				resid |= (long)sensebuf->rs_info[3];
			}


			switch(iobp->ci_key = sensebuf->rs_key & RS_KEY) {
			case RS_NOSENSE:
			case RS_RECERR:
				/*
				 * At end-of-media early warning 
				 * when writing don't set the FAIL flag.
				 * This allows writes to be shutdown
				 * gracefully and buffers to be flushed
				 * before hitting real end-of-media.
				 */
				if (sensebuf->rs_key & RS_EOM)
					infop->tm_fflags |= 
						(infop->tm_fflags & TMF_LASTIOW)
						? TMF_EOM : TMF_FAIL | TMF_EOM;
				else if (sensebuf->rs_key & RS_FILEMARK)
					infop->tm_fflags |= TMF_FAIL | TMF_EOF;
				break; 	
			case RS_BLANK:
				/* Blank media or end-of-data */
				infop->tm_fflags |= TMF_FAIL | TMF_EOF | TMF_EOM;
				break;
			case RS_UNITATTN:
				/* Media change or unit was reset */
				infop->tm_fflags |= TMF_FAIL | TMF_ATTEN;
				break;
			default:
				/* Hard error or unknown sensekey */

				if (sensebuf->rs_key & RS_EOM)
					infop->tm_fflags |= TMF_FAIL | TMF_EOM;
				else if (sensebuf->rs_key & RS_FILEMARK)
					infop->tm_fflags |= TMF_FAIL | TMF_EOF;
				else
					infop->tm_fflags |= TMF_FAIL;
				break;
			}
			iobp->ci_slen = min(TM_RSENSE_LEN, 
						sizeof(struct scrsense) + 
						(u_int)sensebuf->rs_addlen);
		}
		iobp->ci_resid = resid;
		iobp->ci_msg = tm_errors[iobp->ci_key].se_data;
		return (1);
	} else {
		iobp->ci_msg = NULL;
		return (0);
	}
}

/*
 * tm_report_sense_info()
 *	Information about a SCSI check condition has 
 *	been saved in the associated CB.  Determine 
 *	if this information should be displayed on the 
 *	console; if so print an error message and the 
 *	sense buffer data for the check condition, leave 
 *	the error message field null as an indicator 
 *	that this information has been displayed.  
 *	Otherwise don't take any action.
 *
 *	Assumes the info-structure is locked by
 * 	the caller.
 */
static int
tm_report_sense_info(infop, cb, iobp)
	register struct tm_info *infop;
	register struct scsi_cb *cb;
	register struct tm_iobuf *iobp;
{
	register u_char *cp;
	struct scrsense *sensebuf;
	u_int command;
	int count = 0;
	int unit;

#ifdef DEBUG
	ASSERT(infop, "tm_report_sense_info: NULL info-structure pointer");
	ASSERT(cb, "tm_report_sense_info: NULL CB pointer");
	ASSERT(iobp, "tm_report_sense_info: NULL CB/buffer pointer");
#endif DEBUG

	sensebuf = (struct scrsense *)cb->sh.cb_sense;
	command = (u_int)cb->sh.cb_scmd[0];

	if (infop->tm_cflags & TMC_PRSENSE 
	||  tm_errors[iobp->ci_key].se_prntflag
	||  iobp->ci_msg 
	    && (infop->tm_cflags & TMC_RSENSE && command == SCSI_READ 
	        || infop->tm_cflags & TMC_SSENSE && command == SCSI_SPACE
	        || infop->tm_cflags & TMC_WSENSE 
		   && (command == SCSI_WRITE || command == SCSI_WFM))) { 
		unit = infop - tm_info;
		/* 
		 * Attempt to display the message and 
		 * sense buffer data.  If displayed
		 * NULL the message pointer field to
		 * prevent additional printing of it.
		 */
		if (iobp->ci_msg) {
			CPRINTF("tm%d: %s on command %s", unit,
				iobp->ci_msg, tm_cmd[command]);
			CPRINTF("; error code=%x", 
				sensebuf->rs_error & RS_ERRCODE);
			if (sensebuf->rs_key & RS_FILEMARK)
				CPRINTF("; filemark");
			if (sensebuf->rs_key & RS_EOM)
				CPRINTF("; end of media");
			if (sensebuf->rs_addlen >= 3) {
				/* Report device dependent information. */
				CPRINTF("\ntm%d: recoverable errors=", unit);
				if (infop->tm_type == TMT_EMULEX) {
					/* Emulex controller */
					cp = ((u_char *) sensebuf) + 9;
					CPRINTF("%d", (u_int) *cp << 8 | 
						 (u_int)*(cp + 1));
				} else {
					/* Tandberg drive */
					cp = ((u_char *) sensebuf) + 12;
					CPRINTF("%d", (u_int) *cp << 8 | 
						 (u_int)*(cp + 1));
					cp += 4;
					count = (u_int)*cp++ << 16;
					count |= (u_int)*cp++ << 8; 
					count |= (u_int)*cp; 
					CPRINTF("; block counter=%d", count);
				}
			}
			CPRINTF("\n");
			iobp->ci_msg = NULL;
		}
		CPRINTF("tm%d: sense buf:", unit);
		for (cp = (u_char *)sensebuf; iobp->ci_slen--; )
			CPRINTF("%x ", *cp++);
		CPRINTF("\ntm%d: cmd buf:", unit);
		for (cp = (u_char *)cb->sh.cb_scmd; cb->sh.cb_clen--; )
			CPRINTF("%x ", *cp++);
		CPRINTF("\n");
	} else { 
		/*
		 * Avoid printing a annoying messages 
		 * if the criteria above were not met.
		 */
		switch (iobp->ci_key) {
		case RS_NOSENSE:
		case RS_UNITATTN:
		case RS_BLANK:
			iobp->ci_msg = NULL;
		}
	}
}

/*
 * tm_pop_cb()
 *	Returns the index of the next available I/O
 *	CB/buffer pair from the specified device
 *	queue.  If the queue is empty and the caller
 *  	indicates the info struct may be unlocked then 
 *	wait on its semaphore until it is not (the
 *	semaphore is also the queue counter). Otherwise
 * 	assume the caller knows a CB is ready and
 *	don't unlock.
 *
 *	Assumes the device' info-structure is
 *	locked by the caller.
 * 	Also assumes that sequential elements
 *	on the queue have sequential indexes.
 */
static int 
tm_pop_cb(q, infop, unlock)
	register struct tm_queue *q;
	register struct tm_info *infop;
	u_char unlock;
{
	register int index;
#ifdef DEBUG
	bool_t success;

	ASSERT(infop, "tm_pop_cb: NULL info-structure pointer");
#endif DEBUG

	if (unlock) {
		p_sema_v_lock(&q->sq_sync, PRIBIO, &infop->tm_lock, 
			infop->tm_spl);
		infop->tm_spl = p_lock(&infop->tm_lock, TM_SPL);
	} else {
#ifdef DEBUG
		success = cp_sema(&q->sq_sync);
		ASSERT(success, "tm_pop_cb: cp_sema failed - no cb ready");
#else DEBUG
		(void) cp_sema(&q->sq_sync);
#endif DEBUG
	}

	index = q->sq_head++;
	q->sq_head &= NCBPERSCSI - 1;
	q->sq_count--;

#ifdef DEBUG
	ASSERT(index >= 0 && index < NCBPERSCSI, 
		"tm_pop_cb: CB index is less than zero");
#endif DEBUG

	return (index);
}


/*
 * tm_q_cb()
 *	Given the address of a device' queue 
 *	structure and the index of one of its 
 *	CB/buffer pairs, append that index to
 *	that queue.
 *
 * 	Assumes elements being appended to a
 *	non-empty queue have sequential indexes.
 *	Also assumes the device' info-structure is
 *	locked by the caller.
 */
static void 
tm_q_cb(q, index)
	register struct tm_queue *q;
	register int index;
{
#ifdef DEBUG
	ASSERT(q->sq_count < NCBPERSCSI,
		"tm_q_cb: queue count is too large");
	ASSERT(index >= 0, "tm_q_cb: CB index is less than zero");
	ASSERT(index < NCBPERSCSI, "tm_q_cb: CB index is too large");
#endif DEBUG

	if (q->sq_count <= 0)
		/* Empty queue; save index as its new head */
		q->sq_head = index;	
#ifdef DEBUG
	else {
		ASSERT((q->sq_head + (q->sq_count) &
			NCBPERSCSI - 1) == index, 
			"tm_q_cb: CB index is not sequential");
	}
#endif DEBUG
	q->sq_count++;
	v_sema(&q->sq_sync);
}

/*
 * tm_push_cb()
 *	Given the address of a device' queue 
 *	structure and the index of one of its 
 *	CB/buffer pairs, prepend that index to
 *	that queue.
 *
 * 	Assumes elements being prepended to a
 *	non-empty queue have sequential indexes.
 *	Also assumes the device' info-structure is
 *	locked by the caller.
 */
static void 
tm_push_cb(q, index)
	register struct tm_queue *q;
	register int index;
{
#ifdef DEBUG
	ASSERT(q->sq_count < NCBPERSCSI,
		"tm_push_cb: queue count is too large");
	ASSERT(index >= 0, "tm_push_cb: index is less than zero");
	ASSERT(index < NCBPERSCSI, "tm_push_cb: index is too large");

	if (q->sq_count > 0) {
		ASSERT((q->sq_head + NCBPERSCSI - 1 & 
			NCBPERSCSI - 1) == index, 
			"tm_q_cb: CB index is not sequential");
	}
#endif DEBUG
	q->sq_head = index;
	q->sq_count++;
	v_sema(&q->sq_sync);
}
