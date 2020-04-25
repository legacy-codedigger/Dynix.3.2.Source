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

#ident		"$Header: tg.c 1.14 1991/08/15 00:03:29 $"

/*
 * tg.c
 *	Driver for HP88780A tape drive (SSM-based SCSI)
 */


#include "../h/param.h"			/* Sizes */
#include "../h/mutex.h"			/* Gates, semaphores, and such */
#include "../h/user.h"			/* User info and errno.h */
#include "../h/file.h"			/* Flag defines */
#include "../machine/pte.h" 			/* Page tables info */
#include "../h/vmmac.h"			/* VM related conversion functions */
#include "../h/buf.h"			/* struct buf, header and such */
#include "../h/uio.h"			/* struct uio */
#include "../h/ioctl.h"			/* I/O copin's... */
#include "../h/cmn_err.h"
#include "../h/mtio.h"			/* Ioctl cmds from mt/others.. */
#include "../machine/intctl.h"			/* Spl declarations */
#include "../ssm/ioconf.h"
#include "../balance/cfg.h"
#include "../h/scsi.h"
#include "../ssm/ssm.h"                 /* SCSI common data structures */
#include "../ssm/ssm_misc.h"
#include "ssm_scsi.h"			/* Driver local structures */
#include "tg.h"				/* Driver local structures */

extern int strncmp();
extern char *scsi_errors[];
extern char *scsi_commands[];
extern caddr_t calloc();
extern unsigned max(), min();
static struct tg_info *tg_info;
static int tg_baselevel;

/*
 * Forward references.
 */
int tgminphys();		
int  tgstrat();
static tgprobe(), tgboot(), tgintr();
static void tg_raw_io();
static int tg_space_cmd();
static void tg_check_term();
static u_long tg_start_cmd();
static u_long tg_mode_start_cmd();
struct ssm_driver tg_driver = {
      /* driver prefix, configuration flags, probe(),  boot(),  interrupt() */
        "tg",           SDR_TYPICAL_SCSI,    tgprobe, tgboot, tgintr
};
#ifdef DEBUG

int tg_debug=1;				  /* 0=off */
#define TGPRINTF1	if (tg_debug & 1) CPRINTF
#define TGPRINTF2	if (tg_debug & 2) CPRINTF
#define TGPRINTF4	if (tg_debug & 4) CPRINTF

#endif /* DEBUG */
 
/*
 * tgprobe()
 *	Probe for an HP 88780A tape drive on an SSM SCSI bus.
 *      If the device is found return SCP_ONELUN if the
 *	device has an embedded SCSI target adapter and
 *	SCP_FOUND if it does not. 
 *	If not found return SCP_NOTARGET if the target
 *	adapter does not respond, and SCP_NOTFOUND otherwise.
 */
static
tgprobe(sp)
	register struct ssm_probe *sp;
{

	register struct scsi_cb *cb;
	struct scb_init sinit;
	struct scinq *inquiry;
	int retries, status;
	struct tg_bconf *bconfp;
	ASSERT(sp, "tgprobe: Unexpected NULL probe descriptor.");
	/*
	 *+ The probe function of the tg driver received a false
	 *+ descriptor address from the configuration code.  The
	 *+ configuration code might be broken.
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
	ASSERT(cb, "tgprobe: CB not allocated");
	/*
	 *+ A CB was supposed to be allocated by the init_ssm_scsi_dev() 
	 *+ function but was not.  Probable cause is lack of system memory.
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
	status  = ssm_scsi_probe_cmd(SCSI_INQUIRY, cb, sp);
	inquiry = (struct scinq *) cb->sh.cb_addr;
	ASSERT(inquiry, "tgprobe: Inquirybuf not allocated");
	/*
	 *+ An inquiry buffer was supposed to be allocated by the 
	 *+ ssm_scsi_probe_cmd() function but was not.  Probable 
	 *+ cause is lack of system memory.
	 */

	switch (status) {
	case SSTAT_NOTARGET:
		return (SCP_NOTARGET);
	case SSTAT_CCHECK:
	case SSTAT_DCHECK:
		for (retries = 4; retries-- && 
			ssm_scsi_probe_cmd(SCSI_TEST, cb,sp) != SSTAT_OK; ) 
			;
		if ((ssm_scsi_probe_cmd(SCSI_INQUIRY, cb, sp)) != SSTAT_OK) 
			return(SCP_NOTFOUND);
	case SSTAT_OK:
		break;
	default:
		return (SCP_NOTFOUND);
	}

	if (inquiry->sc_devtype != INQ_SEQ 
	||  inquiry->sc_qualif != INQ_REMOVABLE) 
		return (SCP_NOTFOUND); /* Incorrect media/device type */

	for (bconfp = tg_bconf; bconfp < tg_bconf + tg_max_ndevs ; bconfp++) 
		if ( strlen(bconfp->vendor) == 0) {
			if (inquiry->sc_vlength == 0) {
				return (SCP_FOUND); 
			}
		} else if (strncmp(bconfp->vendor, inquiry->sc_vendor,
				strlen(bconfp->vendor)) == 0
		&&	strncmp(bconfp->product, inquiry->sc_product,
				strlen(bconfp->product)) == 0) {
				return ((bconfp->embedded == TGD_EMBED) ? 
					SCP_ONELUN|SCP_FOUND : SCP_FOUND);
		}
	return (SCP_NOTFOUND);       /* No match in the table */
}

/*
 * tgboot()
 *	Initialize a group of SCSI tape drives.  
 * 	Allocate and initialize the driver information 
 *	structures, descriptors for the i/o buffers, and 
 *	dma mapping entries.  Perform any device specific 
 *	initialization to get things started.
 *
 *	Note that any device that is not fully initialized
 *	and/or is deconfigured will have infop->dev == NULL.
 */
static
tgboot(ndevs, dev_array)
	int     ndevs;
	struct  ssm_dev dev_array[];
{
	register struct tg_info *infop;
	register struct ssm_dev *devp = dev_array;
	struct tg_bconf *bconfp = tg_bconf;
	struct scsi_cb *cb;
	struct scb_init sinit;
	int i, num_maps, buf_size;
	int unit = 0;
	u_long *maps;
	unsigned ibuf_size;

#ifdef DEBUG
	ASSERT(ndevs > 0, 
		"tgboot: # devices to boot is less than one");
	ASSERT(dev_array, "tgboot: NULL device array address");
	ASSERT(tg_bconf, 
		"tgboot: NULL binary configuration data address");

	TGPRINTF1( "tgboot: %d drives\n", ndevs);
#endif DEBUG

	/*
 	 * Ignore device descriptions for which there 
	 * is not a binary configuration table entry
 	 * or which are in excess of the driver maximum.
	 */
	if (ndevs > TG_UNITMAX + 1) {
	 	printf("tgboot: %d minor device limit exceeded - extras ignored.\n",
 			ndevs = TG_UNITMAX + 1);
		/*
		 *+ The number of tg devices in the system configuration 
		 *+ exceeds the system maximum.  Extraneous entries
		 *+ will not be booted.  The system configuration should 
		 *+ be corrected.
		 */
	}
	if (ndevs > tg_max_ndevs) {
		printf("tgboot: %d device binary configuration ",
			 ndevs = tg_max_ndevs);
		/*
		 *+ The number of tg devices configured in the
		 *+ system is greater than the number of binary configuration
		 *+ entries in conf_tg.c.
		 */
		CPRINTF("limit exceeded (from conf_tg.c).\n");
		CPRINTF("\tadditional configured tape ");
		CPRINTF("drives ignored.\n");
	} else 
		tg_max_ndevs = ndevs;  /* Save actual # configured. */


	/*
	 * Allocate an array of info structure, one per
	 * possible device, and initialize them.
	 */
	/*
	 * Store device group's base interrupt vector,
	 */
	tg_baselevel = devp->sdv_sw_intvec;	
	tg_info = infop = (struct tg_info *) 
		calloc(tg_max_ndevs * sizeof(struct tg_info));
#ifdef DEBUG
	TGPRINTF1( "tgboot: tg_info @@ 0x%x\n", tg_info);
#endif /* DEBUG */

	ASSERT(tgboot, "tgboot: Info structure allocation failed");
	/* 
	 *+ There is not enough memory for the tg driver to 
	 *+ allocate its per device information structures.
	 *+ Either another device has taken more than its
	 *+ share or more memory needs to be added to your
	 *+ machine.
	 */

	/* 
	 * For each unit locate its configuration
	 * information attempt to boot it.  If
	 * this fails, deconfigure the unit.
	 */
	for (; ndevs; ndevs--, infop++, devp++, bconfp++) {
		if (!devp->sdv_alive) 
			continue;
#ifdef DEBUG
		TGPRINTF1("tgboot: tg%d is alive.\n", bconfp);
#endif /* DEBUG */

	 /*
	  * Determine the size of request-sense buffer
	  * that the each device needs. It  should  be big
	  * enough to accomodate the sense and mode sense buffers.
	  */
		ibuf_size = max((unsigned)TG_RSENSE_LEN, (unsigned)TG_MSEL_PARMLEN);
		buf_size = roundup(bconfp->bc_bufsz * 1024, CLBYTES);

		num_maps = buf_size / CLBYTES;

		if (devp->sdv_maps_avail < num_maps * NCBPERSCSI) {
			CPRINTF("tg%d: not enough maps available ", 
				tg_max_ndevs - ndevs);
			/*
			 *+ The tg driver doesn't have enough DMA maps 
			 *+ for this drive.  Recheck your system 
			 *+ configuration to verify that it has been 
			 *+ configured properly. 
			 */
			CPRINTF("(%d available, %d needed) ... deconfigured\n",
				devp->sdv_maps_avail, num_maps * NCBPERSCSI);
			devp->sdv_alive = 0;
			continue;
		}
		ASSERT(devp->sdv_maps, "tgboot: DMA map address is NULL");
		/*
		 *+ The base address of DMA maps is zero.  This is most
		 *+ probably due to a lack of DMA maps in the configuration
		 *+ information.
		 */
		
		/*
		 * For each of the unit's i/o descriptors allocate 
		 * a SCSI sense data buffer for its SCSI operations.  
		 * One of these buffers will also be used for mode 
		 * selection each time the unit is opened.
		 * Allocate the sense buffer so that it will accomodate
		 * the sense and mode sense buffers.
		 */
		infop->cmd.sense = (u_long) ssm_alloc(
				ibuf_size, SSM_ALIGN_XFER, SSM_BAD_BOUND);
		infop->cmd.slen = ibuf_size;

		if (!infop->cmd.sense) {
			printf(
				"tg%d: buffer allocation failed\n", unit);
			/*
			 *+ Memory allocation of the request-sense
			 *+ mode select buffer for this tg driver
			 *+ unit failed.  Something else in system
			 *+ initialization is out of control or
			 *+ your system lacks sufficient physical
			 *+ memory.
		 	 */
		}

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
		sinit.si_control = 0;
		sinit.si_host_bin = devp->sdv_bin;
		sinit.si_host_basevec = devp->sdv_sw_intvec;

		init_ssm_scsi_dev(&sinit);
                if (sinit.si_id < 0) {
	               CPRINTF("tg%d: initialization error - exceeded",
		       		tg_max_ndevs - ndevs);
		       CPRINTF(" SCSI bus devices limit ... deconfigured\n");
		       devp->sdv_alive = 0;
		       continue;
		}

                infop->tg_devno = SCSI_DEVNO(devp->sdv_target, devp->sdv_unit);
		infop->tg_bufsize = buf_size;
		infop->tg_cflags = bconfp->bc_cflags,
		infop->ssm = devp->sdv_desc;
		infop->cbs = cb = sinit.si_cbs;
		infop->avail = NCBPERSCSI;
		infop->next = 0;
		for (i = 0,maps = (u_long *)devp->sdv_maps;
			i < NCBPERSCSI; i++, cb++, maps += num_maps) {

			cb->sw.cb_unit_index = SCVEC(sinit.si_id, i);
			cb->sw.cb_iovstart = maps;
		}

		/* 
		 * Initialize the device's synchonization
		 * mechanisms, including its locks and semaphores.  
		 */
		init_lock(&infop->lock, tg_gate);
		init_sema(&infop->usrsync, 1, 0, tg_gate);
		init_sema(&infop->gensync, 0, 0, tg_gate);

		/*
		 * Initialize statistics gathering.
		 */

		/*
		 * Complete the initialization, indicating 
		 * to the driver that this device is alive.
		 */
		infop->access = TGA_CLOSED;
		infop->dev = devp;
	}
#ifdef DEBUG
	TGPRINTF1( "tgboot: DONE. \n");
#endif /* DEBUG */
}

/*
 * tgopen()
 *	Attempt to open the specified device if 
 *	it corresponds to a unit which is alive 
 *	and available for exclusive use.
 */
tgopen(dev,flag)
	dev_t dev;
	int flag;
{
	int unit = TG_UNIT(dev);
	register struct tg_info *infop = tg_info + unit;
	u_char density, mode; 
	/* 
	 * Validate the minor device number.
	 */
	if (minor(dev) > TG_MAXMINOR 
	||  TG_UNIT(dev) >= tg_max_ndevs
	||  !infop->dev)
		return (ENXIO);		/* Device is not available */

	switch (TG_DENSITY(dev)) {
	case MTD_NONE:
		density = TG_DENS_NOOP;
		break;
	case MTD_MED:
		density = MSEL_DEN_X339;
		break;
	case MTD_HIGH:
		density = MSEL_DEN_X354;
		break;
	default:
		uprintf("tg%d: Illegal or unsupported density requested\n", TG_UNIT(dev));
		/*
		 *+ This device was opened with a density-select code that is
		 *+ not supported by this tape drive.
		 */
		return (ENXIO);
	}
	(void)p_sema(&infop->usrsync, PRIBIO);
	infop->spl = p_lock(&infop->lock, TG_SPL);
	/* 
	 * This device can only be opened for exclusive 
	 * access.  Verify that it is closed. 
	 */
	switch (infop->access) { 
	case TGA_RWNDCLS:
		/*
 		 * The previous close started a rewind
		 * operation.  Wait for its termination.
	 	 */
		tg_check_term(infop, TGC_PRSENSE);
		infop->access = TGA_CLOSED;
		break;
	case TGA_CLOSED:
		break;
	default:
		v_lock(&infop->lock, infop->spl);
		v_sema(&infop->usrsync);
		return (EBUSY);
	}
	/*
	 * Verify that the the unit is online.
	 */ 
	infop->sflags &= ~TGF_FAIL;
	(void) scsi_test_unit_ready_cmd(&infop->cmd.scmd,(infop->tg_devno&0x7));
	infop->cmd.dlen = 0;
	infop->term_id = tg_start_cmd(infop,(struct buf *)NULL);
	tg_check_term(infop, TGC_PRSENSE);
	if (infop->sflags & TGF_FAIL) goto openfail;
	/* 
	 * Select the specified drive mode.
	 */
	infop->cmd.data = infop->cmd.sense;
	infop->cmd.dlen = TG_MSEL_PARMLEN;
	bzero((char *)infop->cmd.data, (unsigned) infop->cmd.dlen);
	infop->cmd.iov = NULL;
	(void) scsi_mode_select_cmd(&infop->cmd.scmd, (infop->tg_devno&0x7),
		infop->cmd.dlen);

	mode = MSEL_BFM_ASYNC;
	infop->term_id = tg_mode_start_cmd(infop,density,mode);
	tg_check_term(infop, TGC_PRSENSE);
	if (infop->sflags & TGF_FAIL) goto openfail;
	/* 
	 * Sense the drive's mode and
	 * determine if it is accessible.
	 */
	infop->cmd.data = infop->cmd.sense;
	infop->cmd.dlen = TG_MSEL_PARMLEN ;
	bzero((char *)infop->cmd.data, (unsigned) infop->cmd.dlen);
	infop->cmd.iov = NULL;
	(void) scsi_mode_sense_cmd(&infop->cmd.scmd, (infop->tg_devno&0x7), 
	       infop->cmd.dlen);
	infop->term_id = tg_mode_start_cmd(infop,0,0);
	tg_check_term(infop, TGC_PRSENSE);
	if (infop->sflags & TGF_FAIL) goto openfail;

	if ((flag & FWRITE) && (((u_char *)infop->cmd.sense)[2] & MSENSE_WP)) {
		uprintf("tg%d: no write ring\n", TG_UNIT(dev));
		
		/*+ The requested tape drive is loaded with a tape that has
		 *+ no write ring.  Corrective action:  Unload the tape and
		 *+ attach a write ring to the reel.
		 */
		goto openfail;
	}
	infop->access = TGA_OPEN;
	infop->sflags &= ~(TGF_LASTPOS | TGF_LASTIOW | TGF_EOF | TGF_EOM | 
		TGF_FAIL | TGF_UNLOAD);
	infop->blkno = (daddr_t)0;
	infop->nxrec = INF;
	v_lock(&infop->lock, infop->spl);
	v_sema(&infop->usrsync);
	return (0);

openfail:
	v_lock(&infop->lock, infop->spl);
	v_sema(&infop->usrsync);
	return (EIO);
}

/* tgclose()
 *	Perform required cleanup and mark the 
 *	device as no longer open, notifing 
 *	user level of any errors. 
 */
tgclose(dev, flags)
	dev_t  dev;
	int flags;
{
	register struct tg_info *infop = tg_info + TG_UNIT(dev);
	bool_t write_fm = 0;

	/*
	 * Since the usrsync semaphore controls access to the drive,
	 * all i/o will be completed when the p_sema returns.
	 */
	(void)p_sema(&infop->usrsync, PRIBIO);
	infop->access = TGA_CLOSED;

	if (infop->sflags & TGF_UNLOAD) { 
		/* Just mark it closed and return */
		v_sema(&infop->usrsync);
		return;
	}
	infop->spl = p_lock(&infop->lock, TG_SPL);

	if (flags == FWRITE || infop->sflags & TGF_LASTIOW) {
		/*
		 * The file was open for writing or was written 
		 * to on its last operation, write two filemarks to 
		 * mark the end-of-file and logical-end-of-data.
		 * Then clear the state modifier flags so we
		 * don't accidently write another filemark
		 */
		infop->sflags &= ~(TGF_FAIL | TGF_LASTIOW);
		(void) scsi_SA_write_filemarks_cmd(&infop->cmd.scmd,
		       (infop->tg_devno&0x7), (u_long) 2);
		infop->cmd.dlen = 0;
		infop->term_id = tg_start_cmd(infop,(struct buf *)NULL);
		tg_check_term(infop, TGC_PRSENSE | TGC_WSENSE);

		if ((infop->sflags & (TGF_FAIL | TGF_EOM)) == TGF_FAIL) {
			u.u_error = EIO;
			uprintf( 
				"tg%d: error writing file mark on close\n",
				TG_UNIT(dev));
			/*
			 *+ The tg driver's SCSI write file mark command
			 *+ failed.  This probably occurred, along with
			 *+ an error, while the system was attempting to write to the 
			 *+ medium. The medium might have been protected against 
			 *+ writes or against an attempt to write while not at the 
			 *+ beginning of tape or logical end of data.
			 */
			v_lock(&infop->lock, infop->spl);
			v_sema(&infop->usrsync);
			return;
		} else
			write_fm = 1;
		infop->tg_fileno += 1;
	}

	if (TG_REWIND(dev)) {
		/* 
	 	 * This is a rewind-on-close device; 
	 	 * mark it no longer open and start
		 * an asynchronous rewind.
		 */
		infop->access = TGA_RWNDCLS;
		(void) scsi_SA_rewind_cmd(&infop->cmd.scmd,(infop->tg_devno&0x7)			,0);
		infop->cmd.dlen = 0;
		infop->term_id = tg_start_cmd(infop,(struct buf *)NULL);
		infop->tg_fileno = 0;

	} else if (write_fm || infop->blkno <= infop->nxrec
	       && (infop->sflags & TGF_LASTPOS) == 0) {
		/* 
		 * The media isn't going to be rewound. 
	 	 * If filemarks were just written position
		 * the media between them.  Otherwise, position
		 * to the end of the current file, unless a
		 * positioning ioctl just occurred.
		 */
		(void) scsi_SA_space_cmd(&infop->cmd.scmd, (infop->tg_devno&0x7)
			,(u_long)(write_fm ? -1 : 1), SCSI_SPACE_FILEMARKS);
		infop->cmd.dlen = 0;
		infop->term_id = tg_start_cmd(infop,(struct buf *)NULL);
		tg_check_term(infop, TGC_PRSENSE | TGC_SSENSE);

		if ((infop->sflags & (TGF_FAIL|TGF_EOM)) == TGF_FAIL) {
			u.u_error = EIO;
			uprintf( 
				"tg%d: error spacing over file mark on close\n",
				TG_UNIT(dev));
			/*
			 *+ The tg driver's SCSI space command
			 *+ failed while spacing over a tape mark
			 *+ on close.  This probably resulted 
			 *+ from an attempt to skip past the logical
			 *+ end of data, such as opening and closing 
			 *+ an unwritten tape.
			 */
		}
	}
	v_lock(&infop->lock, infop->spl);
	v_sema(&infop->usrsync);
}

/* 
 * tgwrite()
 *	Invoke physio() to lock down user buffer 
 *	in memory and invoke the driver strategy 
 *	routine to perform the output.
 */
tgwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	int status;
	/*
	 * Call tgiocheck to make sure the i/o size is in range.
	 * If we simply allow a minphys interface to truncate the
	 * i/o operation, odd results may occur on the tape.
	 */
	if (status = tgiocheck(dev, uio))
		return(status);
	return (physio(tgstrat, (struct buf *)NULL, dev, B_WRITE,tgminphys, uio));
}


/* 
 * tgread()
 *	Invoke physio() to lock down user buffer 
 *	in memory and invoke the driver strategy 
 *	routine to perform the output.
 */
tgread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	int status;
	/*
	 * Call tgiocheck to make sure the i/o size is in range.
	 * If we simply allow a minphys interface to truncate the
	 * i/o operation, odd results may occur on the tape.
	 */
	if (status = tgiocheck(dev, uio))
		return(status);
	return (physio(tgstrat, (struct buf *)NULL, dev, B_READ,tgminphys, uio));
}
 
 
/* 
 * tgiocheck()
 *	Make sure that the i/o request can be mapped by the allocated
 *	number of maps.
 */
tgiocheck(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register daddr_t blk;
	register struct tg_info *infop = tg_info + TG_UNIT(dev);
	long offset;
	
	offset = (long) uio->uio_iov->iov_base & (NBPG -1);
	if ((offset + uio->uio_resid) > infop->tg_bufsize)
		return(ENXIO);
	blk = bdbtofsb(uio->uio_offset >> DEV_BSHIFT);
	infop->blkno = blk;
	infop->nxrec = blk + 1;
	return(0);
}



/* 
 * tgminphys()
 *	Since the transfer size has been checked by tgiocheck
 *	from tgread or tgwrite, just return.
 */
/*ARGSUSED*/
tgminphys( bp)
	register struct buf *bp;
{
	return(bp->b_bcount);
}
 

/*
 * tgstrategy()
 * 	READ or WRITE to the device via the raw 
 *	device interface using a buf-structure.
 * 
 *	If the standard block interface is being 
 *	used and the requested block is not within
 * 	the current file, reject the attempt to 
 *	perform I/O and generate user level error.
 *
 *	Note that the raw interface always sets
 *	infop->nxrec to be one greater than the 
 *	requested block number.
 */
/*ARGSUSED*/
tgstrat(bp)
	struct buf *bp;
{
	register struct tg_info *infop = tg_info + TG_UNIT(bp->b_dev);
	int opcount;

	ASSERT(bp, "tgstrat: Unexpected NULL buf-struct pointer");
	/*
	 *+ The physio function passed an invalid address
	 *+ describing a transfer request
	 *+ to the tg driver's strategy function.
	 */
	bp->b_resid = bp->b_bcount;	/* Initially nothing transferred */
	(void)p_sema(&infop->usrsync, PRIBIO);
	infop->spl = p_lock(&infop->lock, TG_SPL);
#ifdef DEBUG
	TGPRINTF2("tgstrat: b_blkno %d, blkno %d, nxrec %d, sflags %d\n",
		bp->b_blkno, infop->blkno, infop->nxrec, infop->sflags);
#endif /* DEBUG */

	/*
	 * If we have crossed the End-of-tape sticker while writing
	 * or had a reported failure, and have not successfully 
	 * repositioned the tape since, reject this I/O as an error.
	 * Successful tape positioning or other such operations will
	 * clear the TGF_FAIL flag.
	 */
	if (infop->sflags & TGF_FAIL) {
		if ((bp->b_flags & B_READ) == B_WRITE 
		&&  infop->sflags & TGF_EOM) {
#ifdef DEBUG
			TGPRINTF2("tgstrat: EOM failure not cleared.\n");
#endif /* DEBUG */
			bp->b_error = ENOSPC;
		} else {
#ifdef DEBUG
			TGPRINTF2("tgstrat: previous failures not cleared.\n");
#endif /* DEBUG */
			bp->b_error = EIO;
		}
		/*bp->b_status |= B_S_XFER_ERR;*/
		bp->b_flags |= B_ERROR;
		goto stratdone;
	}
		
	infop->sflags &= ~TGF_LASTPOS;

	/*
	 * The following checks handle boundary cases for operation
	 * on non-raw tapes.  On raw tapes the initialization of
	 * infop->nxrec by tgminphys causes them to be skipped normally.
	 */
	if (bdbtofsb(bp->b_blkno) > infop->nxrec) {
		/*
		 * Can't read past known end-of-file.
		 */
		bp->b_flags |= B_ERROR;
		bp->b_error = ENXIO;
#ifdef DEBUG
		TGPRINTF2("tgstrat: past eof,nxrec is %d\n",infop->nxrec);
#endif /* DEBUG */
		goto stratdone;
	}
	if (bdbtofsb(bp->b_blkno) == infop->nxrec &&
	    	(bp->b_flags & B_READ) == B_READ) {
		/*
		 * Reading at end of file returns 0 bytes.
		 */
		clrbuf(bp);
#ifdef DEBUG
		TGPRINTF2("tgstrat: reading at EOF, returning zero bytes\n");
#endif /* DEBUG */
		goto stratdone;
	}

	if (infop->blkno != bdbtofsb(bp->b_blkno)) {
		/*
	 	 * Tape positioned incorrectly; position 
		 * forwards or backwards to the correct spot.
	 	 */
		opcount = bdbtofsb(bp->b_blkno) - infop->blkno;

		if (infop->blkno > infop->nxrec) {
			/*
			 * We must backspace over the filemark 
			 * just read (count it as a record).
			 */
			if (!tg_space_cmd(infop, -1, SCSI_SPACE_FILEMARKS)
			&&  (infop->sflags & (TGF_EOM | TGF_EOF)) == 0) {
				bp->b_flags |= B_ERROR;
				bp->b_error = ENXIO;
#ifdef DEBUG
				TGPRINTF2("tgstrat: backspace of FM failed\n");
#endif /* DEBUG */
				goto stratdone;
			}
			infop->blkno--;
			opcount++;
		}

		if (!tg_space_cmd(infop, opcount, SCSI_SPACE_BLOCKS)) {
			if (opcount > 0) {
				infop->blkno += opcount - infop->resid;
				if (infop->sflags & TGF_EOF)
					infop->nxrec = infop->blkno - 1;
				bp->b_flags |= B_ERROR;
				bp->b_error = ENXIO;
#ifdef DEBUG
				TGPRINTF2("tgstrat: space %d blocks failed\n",
					opcount);
#endif /* DEBUG */
				goto stratdone;
			} else if (infop->sflags & (TGF_EOM | TGF_EOF)) {
				infop->blkno = 0;	/* Beginning of file */
			} else {
				infop->blkno += opcount + infop->resid;
				bp->b_flags |= B_ERROR;
				bp->b_error = ENXIO;
#ifdef DEBUG
				TGPRINTF2("tgstrat: bkspace %d blocks failed\n",
					opcount);
#endif /* DEBUG */
				goto stratdone;
			}
		} else
			infop->blkno += opcount;
	}

 	tg_raw_io(infop, bp);

stratdone:
#ifdef DEBUG
	TGPRINTF1("tgstrat: done, blkno %d, nxrec %d, sflags %d\n",
		infop->blkno, infop->nxrec, infop->sflags);
#endif /* DEBUG */
	v_lock(&infop->lock, infop->spl);
	v_sema(&infop->usrsync);
	biodone(bp);			/* Indicate I/O termination.  */
}

/*
 * tgintr()
 *	Interrupt service routine for the SCSI tape 
 *	driver.  Verify its a valid interrupt, then
 *	save termination status, where it can be 
 * 	checked by the process waiting upon it.
 */
static
tgintr(level)
	int level;
{
	register struct scsi_cb *cb;
	int unit = (level -= tg_baselevel) / NCBPERSCSI;
	int cb_num = level & NCBPERSCSI - 1;
	register struct tg_info *infop = tg_info + unit;
	int status;
	/*
	 * Verify that the interrupt is not bogus
	 * and that the device is alive.
	 */
	if (unit < 0 || unit >= tg_max_ndevs) {
		CPRINTF("tgintr: Invalid interrupt vector; ignored\n");
		return;
	}
	infop = tg_info + unit;
	infop->spl = p_lock(&infop->lock, TG_SPL);
	cb = infop->cbs + cb_num;


	if (cb != (struct scsi_cb *)infop->term_id) {
		printf( 
			"tg_intr: tg%d: bad interrupt received- ignored",
				unit);
		/*
		 *+ The tg driver received an interrupt for one
		 *+ its units which appears to be inactive.  This 
		 *+ may indicate a driver internal problem, a problem
		 *+ with the SCSI adapter in the host, or a 
		 *+ malfunctioning drive unit.
		 */
		v_lock(&infop->lock, infop->spl);
		return;
	}	

	switch(cb->sh.cb_compcode) {
	case SCB_OK:
	status = scsi_status(cb->sh.cb_status, 
			(struct scrsense *)cb->sh.cb_sense);
	break;
	case SCB_NO_TARGET:
		status = SSTAT_NOTARGET;
		break;
	case SCB_SCSI_ERR:
		status =  (SSTAT_BUSERR);
		break;
	case SCB_BUSY:
		 status =  (SSTAT_BUSYTARGET);
		break;
	case SCB_BAD_CB:
		printf("tgintr: Invalid CB reported\n");
		/*
		 *+ The SSM firmware rejected a SCSI command
		 *+ block as being invalid and nonexecutable.
		 *+ The possible cause is defective SSM firmware
		 *+ or memory corruption from an unknown source.
		 */
		break;
	default:
		panic("tgintr - Bad SCSI command block completion status");
		/*
		 *+ The tg driver received an invalid SCSI termination
	  	 *+ code for a SCSI command.  The SCSI adapter or
		 *+ operating system are corrupt.
		 */
	}
	/* Make the CB available for another SCSI command request*/
	infop->avail++;
	/*
	 * Save the termination status and notify the
 	 * process awaiting the command's termination.
	 */
	 
	infop->status = status;
	v_sema(&infop->gensync);
	v_lock(&infop->lock, infop->spl);
}
 
/*
 * tgioctl() 
 *	Driver asynchronous i/o control requests.
 *	Verify that the command is a tape operation 
 *	and verify that it is compatible with the 
 * 	current mode of operation before executing 
 *	the request and reporting its termination 
 *	status.  
 */
tgioctl(dev, cmd, data)
	dev_t   dev; 
	int	cmd;
	caddr_t data;
{
	register struct tg_info *infop = tg_info + TG_UNIT(dev);
	struct mtop  *mtop = (struct mtop *)data;
	int success = 1;
	int opcount;
	struct mtget *mtget;

	(void)p_sema(&infop->usrsync, PRIBIO);
	infop->spl = p_lock(&infop->lock, TG_SPL);
	switch (cmd) {
	case MTIOCTOP:		/* tape operation */
		opcount = mtop->mt_count;

		switch (mtop->mt_op) {
		case MTNOP:
			break;			/* Boy, that was easy... ;-) */
		case MTFSF: 
			/*
		       	 * Space 'opcount' filemarks.
		 	 */
			success = tg_space_cmd(infop, opcount, SCSI_SPACE_FILEMARKS);
			infop->sflags |= TGF_LASTPOS;
			break;
		case MTBSF: 
			/*
		       	 * Space 'opcount' filemarks backwards.
		 	 */
			success = tg_space_cmd(infop, -opcount, SCSI_SPACE_FILEMARKS);
			infop->sflags |= TGF_LASTPOS;
			break;
		case MTFSR: 
			/*
		       	 * Space 'opcount' records.
		 	 */
			if (success = tg_space_cmd(infop, opcount, SCSI_SPACE_BLOCKS))
				infop->blkno += opcount;
			infop->sflags |= TGF_LASTPOS;
			break;
		case MTBSR: 
			/*
		       	 * Space 'opcount' records backwards.
		 	 */
			if (success = tg_space_cmd(infop, -opcount, SCSI_SPACE_BLOCKS))
				infop->blkno -= opcount;
			infop->sflags |= TGF_LASTPOS;
			break;
		case MTSEOD: 
			/*
		       	 * Space to the logical end-of-data.
		 	 */
			success = tg_space_cmd(infop, 0, SCSI_SPACE_ENDOFDATA);
			infop->sflags |= TGF_LASTPOS;
			break;
		case MTWEOF:
			/*
		 	 * Write 'opcount' filemarks on the media.
		 	 */
			infop->sflags &= ~(TGF_FAIL | TGF_LASTIOW);
			infop->sflags |= TGF_LASTPOS;

			/* 
		 	 * Initiate the SCSI write-filemarks
		 	 * operation, then await its termination.
		 	 */
			(void)scsi_SA_write_filemarks_cmd(&infop->cmd.scmd, 
				(infop->tg_devno&0x7), (u_long) opcount);
			infop->cmd.dlen = 0;
			infop->term_id = tg_start_cmd(infop,(struct buf *)NULL);
			tg_check_term(infop, TGC_PRSENSE | TGC_WSENSE);
			/* 
			 * Handle termination status and 
			 * mode/state transitions.  
			 */
			if ((infop->sflags & (TGF_FAIL | TGF_EOM)) == TGF_FAIL)
				success = 0;
			break;
		case MTERASE:
			/*
		 	 * Erase the media from its current
			 * position to the end-of-tape.
		 	 */
			infop->sflags &= ~(TGF_FAIL | TGF_EOF | 
				TGF_EOM | TGF_LASTIOW);
			infop->sflags |= TGF_LASTPOS;

			/* 
		 	 * Initiate a long-mode SCSI erase operation 
		 	 * and await its termination.
		 	 */
			(void) scsi_SA_erase_cmd(&infop->cmd.scmd, 
				(infop->tg_devno&0x7), 1);
			infop->cmd.dlen = 0;
			infop->term_id = tg_start_cmd(infop,(struct buf *)NULL);
			tg_check_term(infop, TGC_PRSENSE);

			/*
		 	 * Handle termination status and
		 	 * mode/state transitions.
		  	 */
			if (infop->sflags & TGF_FAIL) 
				success = 0;
			break;
		case MTOFFL:
			/*
		 	 * Unload the tape.
		 	 * 
		 	 * Clear the state modifier flags 
		 	 * so we don't write another filemark
		 	 * if the device is closed next.
		 	 */
			infop->sflags &= ~(TGF_FAIL | TGF_LASTIOW);

			/* 
		 	 * Initiate the SCSI unload tape 
		 	 * operation, then await its termination.
		 	 */
			(void)scsi_SA_load_unload_cmd(&infop->cmd.scmd,
				(infop->tg_devno&0x7), 0);
			infop->cmd.dlen = 0;
			infop->term_id = tg_start_cmd(infop,(struct buf *)NULL);
			tg_check_term(infop, TGC_PRSENSE);
			/* 
			 * Handle termination status and 
			 * mode/state transitions.  
			 */
			if (infop->sflags & TGF_FAIL)
				success = 0;
			else  {
				infop->sflags |= TGF_UNLOAD;
				infop->tg_fileno = 0;
			}
			break;
		case MTREW:
			/*
		 	 * Rewind the media to its load point.
		 	 */ 
			infop->sflags &= ~(TGF_FAIL | TGF_EOF | 
				TGF_EOM | TGF_LASTIOW);
			infop->sflags |= TGF_LASTPOS;

			/* 
		 	 * Initiate a SCSI rewind operation 
		 	 * and await its termination.
		 	 */
			(void) scsi_SA_rewind_cmd(&infop->cmd.scmd,
				(infop->tg_devno&0x7), 0);
			infop->cmd.dlen = 0;
			infop->term_id = tg_start_cmd(infop,(struct buf *)NULL);
			tg_check_term(infop, TGC_PRSENSE);

			/*
		 	 * Handle termination status and
		 	 * mode/state transitions.
		  	 */
			if (infop->sflags & TGF_FAIL)
				success = 0;
			else {
				infop->blkno = 0;
				infop->tg_fileno = 0;
			}
			break;
		default:
			/*
		 	 * Unrecognizable or unsupported
		 	 * operation requested.  Return
		 	 * an error.
		 	 */
			v_lock(&infop->lock, infop->spl);
		 	v_sema(&infop->usrsync);
			return (EINVAL);
		}
		break;
	case MTIOCGET:
		mtget = (struct mtget *)data;
		mtget->mt_type = MT_ISTB;
		mtget->mt_dsreg = (short)infop->access << 8 | 
				 (short)infop->sflags;
		mtget->mt_erreg = 0;
		mtget->mt_resid = 0;	/* always zero */
		mtget->mt_fileno = infop->tg_fileno;
		mtget->mt_blkno = (daddr_t) infop->blkno;
		break;
	default:
		/*
	 	 * Unrecognizable or unsupported
	 	 * operation requested.  Return
	 	 * an error.
	 	 */
		v_lock(&infop->lock, infop->spl);
	 	v_sema(&infop->usrsync);
		return (EINVAL);
	}	
	v_lock(&infop->lock, infop->spl);
 	v_sema(&infop->usrsync);
	return ((success) ? 0 : EIO);
}
 
/*
 * tg_raw_io()
 *	Start a read or write operation.
 *	Assumes that infop is locked.
 */
static void
tg_raw_io(infop, bp)
	register struct tg_info *infop;
	struct buf *bp;
{
	u_char smask;

	ASSERT(bp->b_resid > 0, 
		"tg_raw_io: bp->b_resid < 0");
	/*
	 *+ The tg driver detected a buf structure from 
	 *+ physio that has an invalid, negative residual 
	 *+ count in it or that the driver over-decremented. 
	 */

	infop->cmd.dlen = bp->b_bcount;
	infop->sflags &= ~(TGF_FAIL | TGF_EOF | TGF_EOM | TGF_LASTIOW);

	if (bp->b_flags & B_READ) {
#ifdef DEBUG
	TGPRINTF1("tg_raw_io: READ, resid %d, blkno %d, nxrec %d, sflags %d\n", 
		bp->b_resid, infop->blkno, infop->nxrec, infop->sflags);
#endif /* DEBUG */
		(void) scsi_SA_read_cmd(&infop->cmd.scmd, (infop->tg_devno&0x7) 
			,TG_VARIABLE, (u_long)bp->b_bcount);
		smask = TGF_FAIL;
	} else {
#ifdef DEBUG
	TGPRINTF1("tg_raw_io: WRITE, resid %d, blkno %d, nxrec %d, sflags %d\n",
		bp->b_resid, infop->blkno, infop->nxrec, infop->sflags);
#endif /* DEBUG */
		(void) scsi_SA_write_cmd(&infop->cmd.scmd,(infop->tg_devno&0x7) 
			,TG_VARIABLE, (u_long)bp->b_bcount);
		infop->sflags |= TGF_LASTIOW;
		smask = TGF_FAIL | TGF_EOM;
		infop->nxrec = bdbtofsb(bp->b_blkno) + 1;
	}

	/*
	 * Fire off the command and statistics as well.
	 */
	infop->term_id = tg_start_cmd(infop,bp);
	/*TP_IOSTART_TIME(infop->stats);*/
	tg_check_term(infop, TGC_PRSENSE | TGC_RSENSE);
	/*TP_IODONE_TIME(infop->stats);*/

	if (bp->b_flags & B_READ
	&&  infop->sflags & TGF_EOF) {
		/* 
		 * Reading EOF is not an error; Note EOF
		 * block number and clear the failure flag.
		 * Treat EOM like EOF so that blank tapes
		 * can be used on the block interface, which
		 * attempts to read files prior to writting.
		 */
		infop->nxrec = bdbtofsb(bp->b_blkno);
		infop->sflags &= ~TGF_FAIL;
#ifdef DEBUG
	TGPRINTF1("tg_raw_io: EOF read\n");
#endif /* DEBUG */
	} else if ((infop->sflags & smask) == TGF_FAIL) {
		/* 
		 * Report the occurance of a failure
		 * during the operation.  Note that
		 * smask is used here so that write
		 * can detect early warning (EOM set)
		 * and not call it an error unless the
		 * user attempts yet another write.
		 */
		bp->b_flags |= B_ERROR;
#ifdef DEBUG
	TGPRINTF1("tg_raw_io: I/O failure occurred\n");
#endif /* DEBUG */
	}

	bp->b_resid = infop->resid;
	infop->blkno++;
#ifdef DEBUG
	TGPRINTF1("tg_raw_io: done, resid %d, blkno %d, nxrec %d, sflags %d\n", 
		bp->b_resid, infop->blkno, infop->nxrec, infop->sflags);
#endif /* DEBUG */
}

/*
 * tg_space_cmd()
 *	Build and execute a SCSI space command.
 *	The 'fcode' argument specifies what to
 *	space over (filemarks or to end-of-data).
 *	The 'opcount' is the number of items to
 *	spacing over; forward when greater than
 *	zero, backwards when less than zero.
 *
 *	Returns: 1=successful, 0=failed.
 *
 *	Assumes that the info structure is locked 
 *	upon entry.
 */		
static int
tg_space_cmd(infop, opcount, fcode)
	register struct tg_info *infop;
	int opcount;
	u_char fcode;
{

	if (opcount == 0) return (1);	/* Short curcuit the operation */
	infop->sflags &= ~(TGF_FAIL | TGF_EOF | TGF_LASTIOW);
	(void) scsi_SA_space_cmd(&infop->cmd.scmd,(infop->tg_devno&0x7), 
		(u_long) opcount, fcode);
	infop->cmd.dlen = 0;
	infop->term_id = tg_start_cmd(infop,(struct buf *)NULL);
	tg_check_term(infop, TGC_PRSENSE | TGC_SSENSE);
	if ((fcode == SCSI_SPACE_FILEMARKS) && !(infop->sflags & TGF_FAIL))
		infop->tg_fileno += opcount;
	return ((infop->sflags & TGF_FAIL) ? 0 : 1);
}

/* 
 * tg_summary()
 * 	Display a summary message for a teminated
 *	command through the printf interface.
 */
static void
tg_summary( unitno, error, command)
	u_char unitno;
	char *error, *command;
{
	printf("tg%d: %s on command %s.\n",
		unitno, error, command);
		/*
		 *+ A SCSI command issued by the tg driver
		 *+ had an error termination.  A summary has
		 *+ been displayed containing the reason for
		 *+ the SCSI termination and the SCSI command 
		 *+ being executed.  For more information,
		 *+ refer to the specific error and command.
		 */
}

/* 
 * tg_dump()
 * 	Display request sense information to the 
 * 	console about the command that just terminated.
 * 	It consists of a summary the SCSI command that
 * 	caused the check condition, and the sense data.
 */
static void
tg_dump(infop)
	register struct tg_info *infop;
{
	char msgbuf[200];
	int unit = infop - tg_info;
	struct scrsense *sense = (struct scrsense *)infop->cmd.sense;

	(void) scsi_rsense_string(&infop->cmd.scmd, msgbuf, 200, sense);
	printf( "tg%d: %s", unit, msgbuf);
	/*
	 *+ The tg driver is displaying detailed information 
	 *+ pertaining to a problem that occurred when the
	 *+ indicated SCSI command was executed.  The string
	 *+ containing the message will be of the form:
	  *+ <sensekey> on command <scsi_command>; FILEMARK; EOM; ILI;
	  *+	Info bytes=0xXXXXXXXX; Additional length is xx bytes.
	 */

	scsi_cmd_dump(&infop->cmd.scmd, infop->cmd.dlen, infop->cmd.data, 
		infop->cmd.iov, infop->cmd.slen, infop->cmd.sense);
}

/*
 * tg_check_term()
 *	Wait for the previously initiated SCSI command
 *	to terminate and then verify its termination status 
 *	saved by tgintr() and make appropriate state transitions. 
 *	Also, notify the user console of relevent conditions or
 *	the system console if configured to do so for SCSI
 *	check conditions or upon critical errors. 
 */
static void
tg_check_term(infop, dumpflags)
	register struct tg_info *infop;
	u_char dumpflags;
{
	register struct scrsense *sense = (struct scrsense *)infop->cmd.sense;
	int command = infop->cmd.scmd.cmd[0];
	u_char unit = infop - tg_info;
	long temp;
	u_long utemp;

	/*
	 * Wait for the command to terminate.
	 */
	infop->resid = 0;
	(void)p_sema_v_lock(&infop->gensync, PRIBIO, &infop->lock, infop->spl);
	infop->spl = p_lock(&infop->lock, TG_SPL);

	switch (infop->status) {
	case SSTAT_OK:
		break; 			/* Nothing to report */
	case SSTAT_NODEV:
	case SSTAT_BUSERR:
	case SSTAT_NOTARGET:
	case SSTAT_BUSYTARGET:
	case SSTAT_BUSYLUN:
		infop->sflags |= TGF_FAIL;
		tg_summary( unit, 
			scsi_status_msg[infop->status], scsi_commands[command]);
		break;
	case SSTAT_UCHECK:
		/*
		 * Vendor unique check condition format.
		 * Attempt to recover residue, then fall
		 * through, reporting it and setting failure.  
		 */
		if (scsi_rs_lba((u_char *)infop->cmd.sense, &temp))
			infop->resid = temp;
		break;
	case SSTAT_CCHECK:
	case SSTAT_DCHECK:
		/*
		 * A normal check condition occurred. 
		 * Analyze sense data for failures,
		 * filemarks, or end-of-media, and 
		 * recover the residual count, if valid.
		 */
		if (SCSI_RS_EOM_SET(sense)) infop->sflags |= TGF_FAIL | TGF_EOM;
		if (SCSI_RS_FILEMARK_SET(sense)) 
			infop->sflags |= TGF_FAIL | TGF_EOF;
		if (scsi_rs_info_bytes(sense, &utemp)) {
			if ((int)utemp < 0) {
				infop->resid = 0;
			} else {
				infop->resid = utemp;
			}
		}

		/*
		 * Additional actions and mode transitions are 
		 * based on key codes for the check condition. 
		 */
		switch (SCSI_RS_SENSE_KEY(sense)) {
		case RS_BLANK:
			if (command == SCSI_READ) 
				infop->sflags |= TGF_EOF;  
		case RS_UNITATTN: 	/* Media change or unit reset. */
			if (command != SCSI_TEST) 
				infop->sflags |= TGF_FAIL;  
		case RS_NOSENSE:
		case RS_RECERR:
			/* Report to console if configured to do so */
			if (infop->tg_cflags & dumpflags)
				tg_dump(infop);
			break;

		case RS_NOTRDY:
		case RS_PROTECT:
		case RS_OVFLOW:
			if ((infop->tg_cflags & dumpflags) == 0) {
				/* Note failure and notify user console */
				infop->sflags |= TGF_FAIL;
				tg_summary(unit, 
					scsi_status_msg[infop->status], 
					scsi_commands[command]);
				break;
			}	/* Otherwise fall into default actions... */
		default:
			/* Note these failures and report to system console */
			infop->sflags |= TGF_FAIL;
			tg_dump(infop);	
			break;
		}
		break;
	default:
		printf(
			"tg_check_term - Unrecognized SCSI command block completion status");
		/*
		 *+ The tg driver received an invalid SCSI termination
		 *+ code for a SCSI command.  The SCSI adapter or
		 *+ operating system are corrupt.
		 */
		/*NOTREACHED*/
	}
}

/*
 * tg_start_cmd()
 *	Initiate execution of a SCSI command 
 *	described by 'scac' for the device 
 *	'dev' on the SSM SCSI interface.
 *
 *	Returns NULL when the request is
 *	rejected because the device is not 
 *	fully initialized for interrupts or 
 *	the simultaneous command limit has 
 *	been exceeded (the caller must allow 
 *	an earlier request to terminate).  
 *	Otherwise, returns a magic 'cookie', 
 *	identifying this particular request 
 *	to the caller upon termination.
 */
static u_long
tg_start_cmd(infop,bp)
	register struct	tg_info *infop;
	struct buf *bp;
{
	register scac_t *scac = &infop->cmd;
	register struct scsi_cb *cb;
	u_long offset;
	spl_t spl;

	if (!infop->avail) {
		return (NULL);          /* Reject request */
	} else {
		/* Fetch a CB to execute the request */
		infop->avail--;
		cb = infop->cbs + infop->next++;
		if (infop->next >= NCBPERSCSI)
			infop->next = 0; /* Wrap around CB list */
	}

	/*
	 * Construct the SCSI request for 
	 * the SSM, based on 'scac' passed 
	 * in.  Use the CB just located.
	 */
	if (scac->scmd.dir != SDIR_NONE){
        /*
	 * Build a SCSI command for the
	 * specified buffer.
	 */
	if ((offset = scb_buf_iovects(bp,cb->sw.cb_iovstart))
			  == NULL)
		panic("tg_start_cmd: scb_buf_iovects error\n");
		/*
		 *+ The tg driver was unable to perform
		 *+ DMA mapping for a requested data
		 *+ transfer.  Either the transfer length
		 *+ or the alignment exceeds the resources for
		 *+ this device.
		 */

	scac->data = offset;
	}

	bzero(SWBZERO(cb), SWBZERO_SIZE);
	cb->sh.cb_sense = scac->sense;
	cb->sh.cb_slen = scac->slen;
	cb->sh.cb_cmd = (scac->scmd.dir == SDIR_HTOD) ? 
			 SCB_WRITE | SCB_IENABLE : SCB_READ | SCB_IENABLE;
	bcopy((caddr_t)scac->scmd.cmd, (caddr_t)cb->sh.cb_scmd, 
		cb->sh.cb_clen = scac->scmd.clen);
	if (scac->scmd.dir != SDIR_NONE) {
		cb->sh.cb_addr = scac->data;
		cb->sh.cb_iovec = cb->sw.cb_iovstart;
		cb->sh.cb_count = scac->dlen;
	}
	cb->sh.cb_compcode = SCB_BUSY;

	/*
	 * Initiate SSM execution of the CB.
	 * Then return CB address as its magic 
	 * cookie to identify its termination.
	 */
	spl = splhi();
	mIntr((u_char)infop->ssm->ssm_slicaddr, SCSI_BIN, 
		cb->sw.cb_unit_index);
	splx(spl);
	return ((u_long)cb);
}

static u_long
tg_mode_start_cmd(infop,density,mode)
	register struct	tg_info *infop;
	u_char density;
	u_char  mode;
{
	register scac_t *scac = &infop->cmd;
	register struct scsi_cb *cb;
	spl_t spl;

	if (!infop->avail) {
		return (NULL);          /* Reject request */
	} else {
		/* Fetch a CB to execute the request */
		infop->avail--;
		cb = infop->cbs + infop->next++;
		if (infop->next >= NCBPERSCSI)
			infop->next = 0; /* Wrap around CB list */
	}

	/*
	 * Construct the SCSI request for 
	 * the SSM, based on 'scac' passed 
	 * in.  Use the CB just located.
	 */
	bzero(SWBZERO(cb), SWBZERO_SIZE);
	cb->sh.cb_sense = scac->sense;
	cb->sh.cb_slen = scac->slen;
	cb->sh.cb_cmd = (scac->scmd.dir == SDIR_HTOD) ? 
			 SCB_WRITE | SCB_IENABLE : SCB_READ | SCB_IENABLE;
	bcopy((caddr_t)scac->scmd.cmd, (caddr_t)cb->sh.cb_scmd, 
		cb->sh.cb_clen = scac->scmd.clen);
	if (scac->scmd.dir != SDIR_NONE) {
		cb->sh.cb_addr = scac->data;
		cb->sh.cb_iovec = (u_long *)scac->iov;
		cb->sh.cb_count = scac->dlen;
	}
	((u_char *)cb->sh.cb_addr)[2] = mode;
	((u_char *)cb->sh.cb_addr)[3] = SCSI_MODES_DLEN;
	((u_char *)cb->sh.cb_addr)[4] = (u_char)density; /* density */

	cb->sh.cb_compcode = SCB_BUSY;

	/*
	 * Initiate SSM execution of the CB.
	 * Then return CB address as its magic 
	 * cookie to identify its termination.
	 */
	spl = splhi();
	mIntr((u_char)infop->ssm->ssm_slicaddr, SCSI_BIN, 
		cb->sw.cb_unit_index);
	splx(spl);
	return ((u_long)cb);
}
