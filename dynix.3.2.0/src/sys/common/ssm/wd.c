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
static	char	rcsid[] = "$Header: wd.c 1.12 1991/08/06 18:30:12 $";
#endif

/*
 * wd.c 
 *	SCSI disk device driver for SSM
 */

/* $Log: wd.c,v $
 *
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/uio.h"
#include "../h/systm.h"
#include "../h/vm.h"
#include "../h/dk.h"
#include "../h/ioctl.h"
#include "../h/file.h"
#include "../h/vtoc.h"
#include "../h/scsi.h"
#include "../h/cmn_err.h"

#include "../balance/engine.h"
#include "../balance/slic.h" 
#include "../balance/slicreg.h" 
#include "../balance/cfg.h"
#include "../balance/cntrlblock.h"
#include "../balance/clkarb.h"

#include "../ssm/ioconf.h"
#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/plocal.h"
#include "../ssm/ssm.h"
#include "../ssm/ssm_scsi.h"
#include "../ssm/wd.h"
#include "../sec/scsiioctl.h"

int	wdprobe(), wdboot(), wdintr(), wdstrat(), wdcstrat();
struct ssm_driver wd_driver = {
/*	name   flags	         probe     boot     intr */
	"wd", SDR_TYPICAL_SCSI, wdprobe, wdboot, wdintr
};

wd_info **wdinfo;
int	wd_base_vector;
int     wd_ndevs;

static	void wd_error();
extern  gate_t wdgate;
extern	wdmaxminor;
extern	struct timeval	time;
extern	struct wd_bconf wdbconf[];
extern	int wdretrys;
extern	caddr_t get_cb();
extern	struct cb_desc *alloc_cb_desc();
extern	u_long	scb_buf_iovects();
extern	char *ssm_alloc();

caddr_t	wd_compcode[] = {
	"Command Block Busy",
	"Bad Command Block",
	"No Target",
	"SCSI Bus Error",
	"OK",
	"Unrecognized Completion Code"
};

int	wdncompcodes = sizeof(wd_compcode) / sizeof(wd_compcode[0]);

caddr_t	wd_sensekey[] = {
	"No Sense",
	"Recovered Error",
	"Not Ready",
	"Medium Error",
	"Hardware Error",
	"Illegal Request",
	"Unit Attention",
	"Data Protect",
	"Blank Check",
	"Vendor Unique",
	"Copy Aborted",
	"Aborted Command",
	"Equal",
	"Volume Overflow",
	"Miscompare",
	""
};

int	wdnsensekey = sizeof(wd_sensekey) / sizeof(wd_sensekey[0]);

caddr_t	wd_addsense[] = {
	"No Additional Sense",
	"No Index/Sector Signal",
	"No Seek Complete",
	"Write Fault",
	"Drive Not Ready",
	"Drive Not Selected",
	"No Track Zero Found",
	"Multiple Drives Selected",
	"Communication Failure",
	"Track Following Error",
	"Error 0x0a",
	"Error 0x0b",
	"Error 0x0c",
	"Error 0x0d",
	"Error 0x0e",
	"Error 0x0f",
	"ID Error",
	"Unrecovered Read",
	"No Addr Mark in ID",
	"No Addr Mark in Data",
	"No Record Found",
	"Seek Positioning Error",
	"Data Synch Mark Error",
	"Recovered Read, Retries",
	"Recovered Read, ECC",
	"Defect List Error",
	"Parameter Overrun",
	"Synchronous Transfer Error",
	"Defect List Not Found",
	"Compare Error",
	"Recoevered ID, ECC",
	"Error 0x1f",
	"Invalid Command Code",
	"Illegal Block Address",
	"Illegal Function",
	"Error 0x23",
	"Illegal Field in CDB",
	"Inavalid LUN",
	"Invalid Parameter",
	"Write Protected",
	"Medium Changed",
	"Power On or Reset",
	"Mode Select Changed",
	"Error 0x2b",
	"Error 0x2c",
	"Error 0x2d",
	"Error 0x2e",
	"Error 0x2f",
	"Inacompatible Cartridge",
	"Format Corrupted",
	"No Spare Available",
	"Error 0x33",
	"Error 0x34",
	"Error 0x35",
	"Error 0x36",
	"Error 0x37",
	"Error 0x38",
	"Error 0x39",
	"Error 0x3a",
	"Error 0x3b",
	"Error 0x3c",
	"Error 0x3d",
	"Error 0x3e",
	"Error 0x3f",
	"Ram Failure",
	"Data Path Diag Failure",
	"Power On Diag Failure",
	"Message Reject Error",
	"Internal Controller Error",
	"Select/Reselect Failed",
	"Unsuccessful Soft Reset",
	"Parity Error",
	"Initiator Detected Error",
	"Inappropriate/Illegal Message",
	""
};

int	wdnaddsense = sizeof(wd_addsense) / sizeof(wd_addsense[0]);

/*
 * wdprobe - probe device
 *
 * This procedure polls a device with the test unit ready command
 * to determine if the device is present.
 * If a device is found an SCSI_INQUIRY command is issued to make sure
 * we found a SCSI disk and not a tape.
 */
wdprobe(ssmp)
	struct ssm_probe *ssmp;
{
	struct	scsi_cb	*cbs;
	struct	scb_init sinit;
	struct	scinq *wd_inq;
	short	devno = SCSI_DEVNO(ssmp->sp_target, ssmp->sp_unit);
	u_char	compcode;
	static	daddr_t wd_sense_data = NULL;
	static	daddr_t wd_probe_data = NULL;
	
	sinit.si_mode = SCB_PROBE;
	sinit.si_flags = 0;
	sinit.si_ssm_slic = ssmp->sp_desc->ssm_slicaddr;
	sinit.si_scsi = (u_char)ssmp->sp_busno;
	sinit.si_target = (u_char)ssmp->sp_target;
	sinit.si_lunit = (u_char)ssmp->sp_unit;
	sinit.si_control = 0;
	init_ssm_scsi_dev(&sinit);

	if (sinit.si_id >= 0) {
		cbs = sinit.si_cbs;	/* CB to use for SCSI probe commands */
		ASSERT(cbs, "wdprobe: CB not allocated");
		/*
		 *+ The control block for use in probing SCSI devices
		 *+ should have been allocated, but it was not.
		 */
		cbs->sw.cb_unit_index = SCVEC(sinit.si_id, 0);  
		cbs->sw.cb_slic = ssmp->sp_desc->ssm_slicaddr;
	} else {
#ifdef	DEBUG3
		printf("wdprobe: returning not found 1\n");
#endif	DEBUG3
		return (SCP_NOTFOUND);	/* An initialization error occurred */
	}
	
	/* 
	 * Allocate a request sense buffer and a data buffer if they have 
	 * not been allocated.
 	 */
	if (!wd_sense_data) 
		wd_sense_data = (daddr_t) ssm_alloc(WDMAXDATASZ, SSM_ALIGN_XFER,
				   (u_int) SSM_BAD_BOUND);
	if (!wd_probe_data) 
		wd_probe_data = (daddr_t) ssm_alloc(WDMAXDATASZ, SSM_ALIGN_XFER,
				   (u_int) SSM_BAD_BOUND);

	/*
	 * wdp_command is responsible for obtaining 
	 * request sense information if CHECK CONDITION occurs
     	 * after the test unit ready command.
	 */
	compcode = wdp_command(cbs, SCSI_TEST, 
	    (daddr_t)wd_sense_data, (daddr_t)wd_probe_data, devno);
	switch (compcode) {
		case SCB_NO_TARGET:
			if (!SCSI_CHECK_CONDITION(cbs->sh.cb_status)) {
#ifdef	DEBUG3
				printf("wdprobe: returning not found 2\n");
#endif	DEBUG3
				return (SCP_NOTFOUND);
			}
			else {
#ifdef	DEBUG3
			printf("wdprobe: returning not found 3\n");
#endif	DEBUG3
			return (SCP_NOTFOUND | SCP_NOTARGET);
		}
		case SCB_BAD_CB:
		case SCB_SCSI_ERR:
#ifdef	DEBUG3
			printf("wdprobe: returning not found 4\n");
#endif	DEBUG3
			return (SCP_NOTFOUND | SCP_NOTARGET);
		case SCB_OK:
			break;
		default:
#ifdef	DEBUG3
			printf("wdprobe: returning not found 5\n");
#endif	DEBUG3
			return (SCP_NOTFOUND);
		}
			


	/*
	 * Discover type of device found, make sure it is not a 
	 * SCSI tape.
 	 */
	if (wdp_command(cbs, SCSI_INQUIRY, 
	    wd_sense_data, wd_probe_data, devno) != SCB_OK) {
#ifdef	DEBUG3
		printf("wdprobe: returning not found 6\n");
#endif	DEBUG3
		return (SCP_NOTFOUND);
	}

	if (SCSI_CHECK_CONDITION(cbs->sh.cb_status)) {	
#ifdef	DEBUG3
		printf("wdprobe: returning not found 7\n");
#endif	DEBUG3
		return (SCP_NOTFOUND);
	}
	/*
	 * Make sure it is a direct access device.
	 * and that is is an CCS drive.
	 */
	wd_inq = (struct scinq *) wd_probe_data;
	if ((wd_inq->sc_devtype == INQ_DIRECT) && 
 	    (wd_inq->sc_reserved == WD_RES_CCS)) {
#ifdef	DEBUG3
		printf("wdprobe: returning found 8\n");
#endif	DEBUG3

		return (SCP_FOUND | SCP_ONELUN);
	}
	else {
#ifdef	DEBUG3
		printf("wdprobe: returning not found 9\n");
#endif	DEBUG3
		return(SCP_NOTFOUND);
	}

}


/*
 * wdboot - initialize all channels associated with this driver.
 *
 * This procedure initializes and allocates all device driver data
 * structures. It uses the configuration information passed in from
 * autoconfig() and from the device drivers binary configuration tables.
 */
wdboot(ndevs, ssmd)
	int	ndevs;
	struct	ssm_dev	*ssmd;
{
	register wd_info *ip;		/* info structure pointer */ 
	register struct scsi_cb *cb;
	register struct wd_bconf *bconf;
	struct scb_init sinit;
	int	dev, x, capacity;
	u_long	*maps;
	struct v_open *vo;

	wdinfo = (wd_info **) calloc(ndevs * sizeof(wd_info *));
	wd_ndevs = ndevs;
	wd_base_vector = ssmd->sdv_sw_intvec;

	for (dev = 0; dev < ndevs; dev++, ssmd++) {
		if (!ssmd->sdv_alive)
			continue;

		/*
		 * Verify that this device has been included in
		 * the device's binary configuration table. 
		 */
		if (dev > wdmaxminor) {
			CPRINTF("wd%d: non-binary configured device found in config table ... deconfiguring.\n", dev);
			ssmd->sdv_alive = 0;	/* force deconfigure */
			continue;
		}

		/*
		 * Verify that there are enough iovects allocted for each cb
		 * to allow for maximum data transfer. If this condition
 		 * is not met the driver may hang.
		 */
		bconf = &wdbconf[dev];
		if (ssmd->sdv_maps_avail < (bconf->bc_iovects * NCBPERSCSI)) {
			CPRINTF("wd%d: not enough iovects for device ... deconfiguring.\n", dev);
			ssmd->sdv_alive = 0;	/* force deconfigure */
			continue;
		}

		/*
		 * allocate info struct
 		 */	
		wdinfo[dev] = ip = (wd_info *)calloc(sizeof(wd_info));

		ip->wd_devno = SCSI_DEVNO(ssmd->sdv_target, ssmd->sdv_unit);

		/* 
		 * Initialize cb's here,
 		 * including request sense buffers and iovects.
		 */
		sinit.si_mode = SCB_BOOT;
		sinit.si_ssm_slic = ssmd->sdv_desc->ssm_slicaddr;
		sinit.si_scsi = (u_char)ssmd->sdv_busno;
		sinit.si_target = (u_char)ssmd->sdv_target;
		sinit.si_lunit = (u_char)ssmd->sdv_unit;
		sinit.si_host_bin = ssmd->sdv_bin;
		sinit.si_host_basevec = ssmd->sdv_sw_intvec;
		sinit.si_control = 0;


		init_ssm_scsi_dev(&sinit);
		if (sinit.si_id < 0) {
			CPRINTF("wd%d: initialization error - exceeded", dev);
			CPRINTF(" SCSI bus devices limit ... deconfigured\n");
			ssmd->sdv_alive = 0;
			continue;
		} else 

		ip->wd_cbdp = (struct cb_desc *) alloc_cb_desc(
			(char *) sinit.si_cbs,
			(short) NCBPERSCSI,
			(short) sizeof(struct scsi_cb),
		 	(short) FLD_OFFSET(struct scsi_cb, sw.cb_state));
		
		/*
		 * for each cb
	 	 *	allocate request sense buffers
		 * 	allocate data buffers
		 * 	fill in all sw fields of cbs
		 */
		maps = (u_long *)ssmd->sdv_maps;
		for(x = 0, cb = (struct scsi_cb *) ip->wd_cbdp->cb_cbs,
			maps = (u_long *)ssmd->sdv_maps;
		    x < ip->wd_cbdp->cb_count; 
		    x++, cb++, maps += bconf->bc_iovects) {

			cb->sh.cb_sense = (u_long) ssm_alloc(WDMAXDATASZ, 
					SSM_ALIGN_XFER, SSM_BAD_BOUND);
			cb->sw.cb_data = (u_long) ssm_alloc(WDMAXDATASZ, 
					SSM_ALIGN_XFER, SSM_BAD_BOUND);
			cb->sh.cb_slen = WDMAXDATASZ;
			
			cb->sw.cb_iovstart = maps;
			cb->sw.cb_iovnum = bconf->bc_iovects;
			cb->sw.cb_unit_index = SCVEC(sinit.si_id, x);  
			cb->sw.cb_scsi_device = ip->wd_devno;
			cb->sw.cb_slic = ssmd->sdv_desc->ssm_slicaddr;
		}

		/*
		 * Use first cb in cb pair for inquiry and capacity commannds
		 */
		cb = (struct scsi_cb *) ip->wd_cbdp->cb_cbs;
		
#ifdef	WDDEBUG
		printf("wdboot: start inquiry command\n");
#endif	WDDEBUG
		/*
		 * Get inquiry information from drive.
		 */
		wd_command(cb, SCSI_INQUIRY, 0);
		if (wd_cbfin(cb) != SCB_OK) {
			CPRINTF("wd%d: problem doing INQUIRY %s\n",
			dev, "... deconfiguring");
			ssmd->sdv_alive = 0;	/* force deconfigure */
			if (fp_lights)
				FP_IO_ERROR;
			continue;
		}

#ifdef	WDDEBUG
		printf("wdboot: start capacity command\n");
#endif	WDDEBUG
		/*
		 * Get the configured capacity of the drive.
		 */
		if ((capacity = wd_readc(cb)) <= 0) {
			CPRINTF("wd%d: problem doing READ CAPACITY %s\n",
			dev, "... deconfiguring");
			ssmd->sdv_alive = 0;	/* force deconfigure */
			if (fp_lights)
				FP_IO_ERROR;
			continue;
		}

		vo = (struct v_open *)calloc(sizeof(struct v_open));
		ip->wd_part = &vo->v_v;
		ip->wd_opens = &vo->v_vo.v_opens[0];
		ip->wd_modes = &vo->v_vo.v_modes[0];
		ip->wd_vtoc_read = 0;
		ip->wd_ssm_desc = ssmd;
		ip->wd_compat = bconf->bc_part;
		ip->wd_size = capacity;
		ip->wd_retrys = wdretrys;
		ip->wd_iovcount = bconf->bc_iovects;
		ip->wd_inuse = 0;
		ip->wd_flags =  WD_IS_ALIVE;
		
		init_lock(&ip->wd_lock, wdgate);
		init_sema(&ip->wd_usrsync, 1, 0, wdgate);  /* serialize opens */
		bufinit(&ip->wd_rbufh, wdgate);
	
		if (dk_nxdrive < dk_ndrives) {
			ip->wd_statunit = dk_nxdrive;
			ip->wd_dkstats = &dk[dk_nxdrive++];
			bcopy("wdX", ip->wd_dkstats->dk_name, 5);
			ip->wd_dkstats->dk_name[3] = (dev % 10) + '0';
			if (dev >= 10)
				ip->wd_dkstats->dk_name[2] = (dev / 10) + '0';
			ip->wd_dkstats->dk_bps = bconf->bc_blks_per_sec;
		}else
			ip->wd_statunit = WD_NO_UNIT;
	}
	
#ifdef	WDDEBUG
	printf("wdboot: exiting boot\n");
#endif	WDDEBUG
}

		

/*
 * wdopen - open a channel
 *
 *      Check for validity of device.
 *
 *      Call readdisklabel() to read vtoc.  If no vtoc, we will get
 *      default partition info as listed in conf_sd.c.
 *
 *      Tmp assumptions: No mods needed for TMP because sdopen doesn't
 *      *modify* any concurrently accessed common data structures.
 *
 *      This assumes that the device is formated in DEV_BSIZE size blocks
 *      and won't work other wise.
 *
 */

wdopen(dev, flags)
	dev_t dev;
	int   flags;
{
	register int drive;
	register int part;
	register wd_info *ip;
	int	err, i;
	struct	vtoc *vtp;
	spl_t	s;

	drive = VUNIT(dev);
	part = VPART(dev);
	err = 0;
	if (drive >= wd_ndevs)
		return (ENXIO);

	ip = wdinfo[drive];

	if(ip == NULL || ip->wd_part == NULL)	/* Configured properly? */
		return(ENXIO);

	if( drive >= wdmaxminor			/* Valid channel number? */
	|| (!ip->wd_ssm_desc->sdv_alive))	/* Passed probing? */
		return(ENXIO);
	
	p_sema(&ip->wd_usrsync, PRIBIO);  	/* Serialize opens */
	s = p_lock(&ip->wd_lock, WDSPL);

	if (ip->wd_flags & WD_EXCLUSIVE) {
		err = EBUSY;
	} else if (flags & FEXCL) {
		if (!suser()) {
			err = EPERM;
		} else if (ip->wd_nopen) {
			err = EBUSY;
		} else {
			ip->wd_flags |= WD_EXCLUSIVE;
		}
	}
	/*
	 * Ensure that a writer to the V_ALL device holds out all other
	 * accessors of the V_ALL device
	 */
	if (V_ALL(dev) && (ip->wd_flags & WD_ALLBUSY))
		err = EBUSY;

	/*
	 * drop lock as this open routine is still protected with
	 * ip->wd_usrsync.
	 */
	v_lock(&ip->wd_lock, s);

	if (!ip->wd_vtoc_read && !err) {
		/*
	 	 * Read vtoc from disk.  If no vtoc, readdisklabel() 
		 * will return default conf_wd.c partition info in
		 * vtoc structure.
		 */
		err = vtoc_opencheck(dev, flags, wdcstrat,
			ip->wd_part, ip->wd_compat, (daddr_t)0,
			ip->wd_size, ip->wd_modes[part], "wd");

		if (!err && !V_ALL(dev)) {
			/*
			 * convert any end of disk (WD_END) to the capacity
			 */
			vtp = ip->wd_part;
			for (i = 0; i < vtp->v_nparts; i++) {
				if (vtp->v_part[i].p_size == WD_END) {
					vtp->v_part[i].p_size = 
				          ip->wd_size - vtp->v_part[i].p_start;
				}
			}
		}
	}
	if (!err) {
		/*
		 * Succesful open
		 */
		ip->wd_nopen++;
		if (V_ALL(dev)) {
			/*
			 * Successful open on whole-disk;
			 * lock others out.
			 */
			if (flags & FWRITE){
				s = p_lock(&ip->wd_lock, WDSPL);
				ip->wd_flags |= WD_ALLBUSY;
				v_lock(&ip->wd_lock, s);
			}
		} else {
			ip->wd_opens[part] += 1;
			ip->wd_modes[part] |= flags;
			ip->wd_vtoc_read = 1;
		}
	}
	v_sema(&ip->wd_usrsync);
	return(err);
}


/*
 * wdclose - close a channel 
 *
 * Decrement reference count, make sure temporary settings
 * are set back to their default values.
 */
/*ARGSUSED*/
wdclose(dev, flag)
	dev_t dev;
	int flag;
{
	wd_info *ip;
	spl_t s;
	int part = VPART(dev);
	int drive = VUNIT(dev);

	if (drive >= wdmaxminor)
		return(ENXIO);

	ip = wdinfo[drive];
	
	s = p_lock(&ip->wd_lock, WDSPL);
	ip->wd_nopen--;
	if (!V_ALL(dev)) {
		ip->wd_opens[part] -= 1;
		ip->wd_modes[part] &= ~(flag&FUSEM);
	}

	ip->wd_flags &= ~(WD_EXCLUSIVE|WD_FORMATTED);

	/*
	 * If this is last close, clear flag so vtoc will be read on
	 * first open.  (Just in case this is a removeable media drive.)
	 */
	if (ip->wd_nopen == 0)
		ip->wd_vtoc_read = 0;

	/*
	 * If we're holding the whole-disk partition, flag it as free now
	 */
	if (V_ALL(dev) && (ip->wd_flags & WD_ALLBUSY))
		ip->wd_flags &= ~WD_ALLBUSY;

	v_lock(&ip->wd_lock, s);
	return(0);
}

/*
 * wdminphys - correct for too large a request
 */
wdminphys(bp)
	register struct buf *bp;
{
/*	scb_num_iovects(bp);   Commented out since it doesn't do anything */
	if (bp->b_bcount > WD_MAX_XFER * DEV_BSIZE)
		bp->b_bcount = WD_MAX_XFER * DEV_BSIZE;
}

/*
 * wdread - standard raw read procedure
 */
wdread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	int err, diff;
	off_t lim;

	ASSERT(VUNIT(dev) < wd_ndevs, "wdread dev");
	/*
	 *+ A bad device number was passed in to wdread.
	 *+ The system is in an inconsistent internal state.
	 */

	if (V_ALL(dev)) {
		return(physio(wdcstrat, (struct buf *)0, dev, B_READ, wdminphys, uio));
	}
	lim = (wdinfo[VUNIT(dev)])->wd_part->v_part[VPART(dev)].p_size;
	lim <<= DEV_BSHIFT;
	err = physck(lim, uio, B_READ, &diff);
	if (err != 0) {
		if (err == -1)	/* not an error, but request of 0 bytes */
			err = 0;
		return(err);
	}
	err = physio(wdstrat, (struct buf *)0, dev, B_READ, wdminphys, uio);
	uio->uio_resid += diff;
	return(err);
}

/*
 * wdwrite - standard raw write procedure
 */
wdwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	int err, diff;
	off_t lim;
		
	ASSERT(VUNIT(dev) < wd_ndevs, "wdwrite dev");
	/*
	 *+ A bad device number was passed in to wdwrite.
	 *+ The system is in an inconsistent internal state.
	 */

	if (V_ALL(dev)) {
		return(physio(wdcstrat, (struct buf *)0, dev, B_WRITE, wdminphys, uio));
	}
	lim = (wdinfo[VUNIT(dev)])->wd_part->v_part[VPART(dev)].p_size;
	lim <<= DEV_BSHIFT;
	err = physck(lim, uio, B_WRITE, &diff);
	if (err != 0) {
		if (err == -1)	/* not an error, but request of 0 bytes */
			err = 0;
		return(err);
	}
	err = physio(wdstrat, (struct buf *)0, dev, B_WRITE, wdminphys, uio);
	uio->uio_resid += diff;
	return(err);
}

/*
 * wdstrat - SSM SCSI disk strategy routine
 * 	check block sizes and limits.
 *
 * Locking of the wdinfo structure is performed before
 * calls to disksort (needs exclusive access to bufh) and
 * wdstart (needs exclusive access to command block(s) it gets).
 */
wdstrat(bp)
	register struct buf *bp;
{
	register wd_info *ip;
	register struct partition *pt;
	spl_t	s;
	int	length;

	ip = wdinfo[VUNIT(bp->b_dev)];

	ASSERT(ip != NULL, "wdstrat: null info ptr");
	/*
	 *+ The system is attempting to do I/O on an invalid
	 *+ wd device.
	 */

#ifdef WDDEBUG
	ASSERT(ip->wd_part != NULL, "wdstrat: null partition ptr");
#endif WDDEBUG

	pt = &(ip->wd_part->v_part)[VPART(bp->b_dev)];

	length = pt->p_size;

	/*
	 * Size and partitioning check.
	 *
 	 * Fail request on: bad byte count, transfer not entirely 
	 * within a partition, or if it is not aligned properly.
	 * Or if drive went bad.
	 * If the disk has just been formatted, do not check bounds.
	 */
	if ((bp->b_bcount <= 0)
	||  ((bp->b_bcount & (DEV_BSIZE -1)) != 0)		/* size */
	||  ((bp->b_iotype == B_RAWIO) &&
	      (((int)bp->b_un.b_addr & (SSM_ALIGN_XFER - 1)) != 0))
	||  (bp->b_blkno < 0)      
	||  ((bp->b_blkno + howmany(bp->b_bcount, DEV_BSIZE)) > length)
	||  (((wdinfo[VUNIT(bp->b_dev)])->wd_flags & WD_BAD) 
	     == WD_BAD)) {
		bp->b_resid = bp->b_bcount;	
		bp->b_flags |= B_ERROR;
		bp->b_error = EINVAL;
#ifdef	WDDEBUG
		printf("wdstrat: Printing bp\n");
		wd_dump(sizeof(struct buf), bp);
#endif	WDDEBUG
		iodone(bp);
		return;
	}

	bp->b_resid = bp->b_blkno + pt->p_start;	/* req'd by disksort */

	s = p_lock(&ip->wd_lock, WDSPL);
	disksort(&ip->wd_bufh, bp);
	wdstart(ip);
	v_lock(&ip->wd_lock, s);
}

/*
 * wdstart - start a request to a channel.
 * 	if no command blocks are available just return. 
 *
 * Mutual exclusion must take place before calling wdstart
 */
wdstart(ip)
	register wd_info *ip;
{
	register struct buf *bufh = &ip->wd_bufh;
	register struct scsi_cb *cb;
	register struct buf *bp;
	spl_t s;

	if ((bp = bufh->av_forw) == NULL)	/* Check for requests */
		return;

#ifdef	WDDEBUG
	printf("wdstart: bp = \n");
	wd_dump(sizeof(struct buf), bp);
#endif	WDDEBUG

	cb = (struct scsi_cb *) get_cb(ip->wd_cbdp); 
	if (cb == NULL) 
		return;

	if (ip->wd_inuse == 0) {
		ip->wd_starttime = time; 
		if (fp_lights) {
			s = splhi();
			FP_IO_ACTIVE;
			splx(s);
		}
	}

 	ip->wd_inuse ++;
	bufh->av_forw = bp->av_forw;
	if (bp->b_flags & B_IOCTL) {
#ifdef DEBUG
	printf("wdstart: call doioctl\n");
#endif /* DEBUG */
		wd_doioctl(cb, bp);
	} else {
		wd_rw(cb, bp);
	}
}

/*
 * wdintr 
 * 	Command block completion interrupt handler.
 */
wdintr(vector)
	int vector;
{
	register wd_info	*ip; 
	register struct	buf	*bp;
	register struct scsi_cb *cb;
	register u_int level = vector - wd_base_vector;
	struct scrsense *rsense;
	spl_t s, s1;
	u_char	command;

#ifdef	WDDEBUG
	printf("wdintr: Just got interrupted\n");
	printf("wdintr: vector = %x, level=%x\n", vector, level);
#endif	WDDEBUG

	ASSERT((level >> WD_LEVEL_SHIFT) < wd_ndevs, "wdintr: interrupt to non-configured device");
	/*
	 *+ An interrupt was received by the wd driver.
  	 *+ The interrupt vector translates to a unit
	 *+ number that exceeds the range of units configured by
	 *+ this device driver.  This indicates an internal
	 *+ system error such as a corrupt interrupt vector
	 *+ table.
	 */

	ip = wdinfo[level >> WD_LEVEL_SHIFT];
	ASSERT(ip != 0, "wdintr: interrupt to non-configured device");
	/*
	 *+ An interrupt was received by the wd driver.
  	 *+ The interrupt vector translates to a
	 *+ unit that was specified in the system
	 *+ configuration, but was not found at
	 *+ system startup time.  This indicates an internal
	 *+ system error such as a corrupt interrupt vector
	 *+ table.
	 */

	s = p_lock(&ip->wd_lock, WDSPL);

	cb = (struct scsi_cb *) ip->wd_cbdp->cb_cbs + 
		(level & WD_LEVEL_CB);
#ifdef	WDDEBUG
	printf("wdintr: cb = %x\n", cb);
	wd_printcb(cb);
#endif	WDDEBUG

	ASSERT(cb->sw.cb_state & CB_BUSY, "wdintr: interrupt to command block not in use");
	/*
	 *+ A interrupt was received by the wd driver
 	 *+ for a device that appears to be idle. This
	 *+ can indicate a problem with the SSM or an
	 *+ internal error on the driver's part.
	 */


	bp = cb->sw.cb_bp;

	switch(cb->sh.cb_compcode) {
	case SCB_BAD_CB:
		wd_error(cb, bp, "Hard Error: ");
		goto error;

	case SCB_NO_TARGET:
	case SCB_SCSI_ERR:
		wd_error(cb, bp, "Hard Error: ");
		wd_shutdrive(ip);
		v_lock(&ip->wd_lock, s);
		return ;
	
	case SCB_OK:
		if (SCSI_CHECK_CONDITION(cb->sh.cb_status)) {
			rsense = (struct scrsense *) cb->sh.cb_sense;
			if ((rsense->rs_error & RS_ERRCODE) != RS_CLASS_EXTEND) {
				wd_error(cb, bp, "Hard Error bad sense data: ");
				goto error;
			} 
        	 	switch (rsense->rs_key & RS_KEY) {
			case RS_RECERR:
				wd_error(cb, bp, "Soft Error: ");
				bp->b_resid = 0;
				goto finish;

			default:
				if (cb->sw.cb_errcnt >= 4) {
					wd_error(cb, bp, "Hard Error: ");
					goto error;
				}

				wd_error(cb, bp, "Soft Error: ");
				cb->sw.cb_errcnt += 1;
				/*
				 * Restart command
				 */
				command = (cb->sw.cb_bp->b_flags & B_READ) 
				   ? SCSI_READ : SCSI_WRITE;
				wd_command(cb, command, SCB_IENABLE);
				v_lock(&ip->wd_lock, s);
				return ;
			}

		} else			/* Success */
			bp->b_resid = 0;
			goto finish;
	}

error:	
	bp->b_flags |= B_ERROR;
	bp->b_error = EIO;
	bp->b_resid = cb->sh.cb_count;

finish:
	/*
	 * Update stats for xfers and blks and
	 * mark do time stamp magic if all device programs
	 * are free. Note: statistics might not be kept for unit.
	 */
	if(ip->wd_statunit != WD_NO_UNIT) {
		ip->wd_dkstats->dk_xfer++;
		ip->wd_dkstats->dk_blks += howmany(bp->b_bcount, DEV_BSIZE);
			
		if(ip->wd_inuse == 1) {
			struct	timeval	elapsed;
				
			elapsed = time;
			timevalsub(&elapsed, &ip->wd_starttime);
			timevaladd(&ip->wd_dkstats->dk_time, &elapsed);

		}
	}
	
	if (--ip->wd_inuse == 0 && fp_lights) {
		s1 = splhi();
		FP_IO_INACTIVE;
		splx(s1);
	}
	free_cb((caddr_t) cb, ip->wd_cbdp);
#ifdef	WDDEBUG
	printf("wdintr: Printing bp\n");
	wd_dump(sizeof(struct buf), bp);
#endif	WDDEBUG
	iodone(bp);
	wdstart(ip);
	v_lock(&ip->wd_lock, s);
	return;
}


/*
 * wd_rw - start a read or write disk access command.
 *
 * Called only the first time a request is made on the buffer,
 * retries go through wd_command.
 *
 * Assumes exclusive ownership of cb
 */
static
wd_rw(cb, bp)
	struct scsi_cb	*cb;
	struct buf	*bp;
{
	u_char	command;

	cb->sw.cb_bp = bp;
	cb->sw.cb_errcnt = 0;

	command = (bp->b_flags & B_READ) ? SCSI_READ : SCSI_WRITE;

	wd_command(cb, command, SCB_IENABLE);
}


/* 
 * wd_readc - get the capacity of a SCSI disk
 *
 * boot must already have initialized cb's and 
 * request sense buffers for them.
 */
static
wd_readc(cb)
	struct scsi_cb *cb;
{
	struct wdcap *capdat;
	u_long	nblocks = 0;		/* number of blocks on disk */
	daddr_t capacity;

	cb->sw.cb_errcnt = 0;
	
	wd_command(cb, SCSI_READC, 0);
	if (wd_cbfin(cb) != SCB_OK)
		return (-1);

	capdat = (struct wdcap *)cb->sw.cb_data;

	/*
	 * Reverse the byte order
	 */

	nblocks = (capdat->cap_nblocks0 << 24) |
		  (capdat->cap_nblocks1 << 16) |
		  (capdat->cap_nblocks2 << 8)  |
		  (capdat->cap_nblocks3) ;

	/*
	 * decide which kind of disk we have based on which range of
	 * capacities the size falls into.  This allows us to reserve the
	 * right amount for the diag tracks.
	 */

	if	(nblocks <= 140997 && nblocks >= 140869)
		capacity = 140436;			/* fujitsu 2243 */
	else if (nblocks <= 117452 && nblocks >= 117324)
		capacity = 117096;			/* vertex 170 */
	else if (nblocks <= 234089 && nblocks >= 233961)
		capacity = 233324;			/* maxtor 1140 */
	else if (nblocks == 285039)
		capacity = 284480;			/* microp 1375 */
	else if (nblocks == 304604)
		capacity = 303975;			/* CDC wren 3 */
	else if (nblocks == 640298)
		capacity = nblocks - (((9*54)-3)*2);	/* CDC wren 4 */
	else if (nblocks == 270929)
		capacity = 270270;			/* fujitsu m2246sa */
	else if (nblocks == 651524)
		capacity = 650475;			/* fujitsu m2249sa */
	else
		capacity = nblocks;			/* all others */

	return (capacity);
}

/*
 * wd_command - complete and start an SSM SCSI command
 *
 * Commands other than SCSI_READ that need to get data returned from
 * the SCSI device will have said data returned in the request sense
 * buffer. The sc_compcode field must be checked to make sure that
 * the data returned is from the command and not request sense data.
 *
 * The software portion of a SSM SCSI cb must be filled in prior
 * to calling here.
 * 
 * assumes cb is owned exclusively.
 */
static
wd_command(cb, command, flags)
	register struct	scsi_cb	*cb;
	u_char	command;
	u_char	flags;

{
	register struct buf	*bp;
	spl_t	sx;
#ifdef	WDDEBUG
	printf("wd_command: In wd_command\n");
	printf("wd_command: command = %x\n", command);
	printf("wd_command: cb = %x\n", cb);
#endif	WDDEBUG
	bp = cb->sw.cb_bp;
	bzero((caddr_t) SWBZERO(cb), SWBZERO_SIZE);

	switch (command) {

	case SCSI_TEST:
		cb->sh.cb_count = WD_TEST;
		cb->sh.cb_cmd = SCB_READ | flags;
		cb->sh.cb_scmd[0] = command;
		cb->sh.cb_scmd[1] |= cb->sw.cb_scsi_device << SCSI_LUNSHFT; 
		cb->sh.cb_clen = SCSI_CMD6SZ;
		break;

	case SCSI_INQUIRY:
		cb->sh.cb_count = INQ_LEN;
		cb->sh.cb_addr = cb->sw.cb_data;
		cb->sh.cb_cmd = SCB_READ | flags;
		cb->sh.cb_scmd[0] = command;
		cb->sh.cb_scmd[1] |= cb->sw.cb_scsi_device << SCSI_LUNSHFT;
		cb->sh.cb_scmd[4] = INQ_LEN;
		cb->sh.cb_clen = SCSI_CMD6SZ;
		break;

	case SCSI_READC:
		cb->sh.cb_count = WD_READC;
		cb->sh.cb_addr = cb->sw.cb_data;
		cb->sh.cb_cmd = SCB_READ | flags;
		/*
 		 * PMI 0
		 * Returns capacity of entire disk
		 */
		cb->sh.cb_scmd[0] = command;
		cb->sh.cb_scmd[1] |= cb->sw.cb_scsi_device << SCSI_LUNSHFT; 
		cb->sh.cb_clen  = SCSI_CMD10SZ;
		break;

	case SCSI_REZERO:
		cb->sh.cb_cmd = SCB_READ | flags;
		cb->sh.cb_scmd[0] = command;
		cb->sh.cb_scmd[1] |= cb->sw.cb_scsi_device << SCSI_LUNSHFT;
		cb->sh.cb_clen = SCSI_CMD6SZ;
		break;

	case SCSI_READ:
	case SCSI_WRITE:
		if ((cb->sh.cb_addr = scb_buf_iovects(bp, cb->sw.cb_iovstart)) == NULL) 
		panic("wd_command: scb_buf_iovects error\n");
		/*
		 *+ The wd driver was unable to perform
		 *+ DMA mapping for a requested data
		 *+ transfer.  Either the transfer length
		 *+ or the alignment exceeds the resources for
		 *+ this device.
		 */

		cb->sh.cb_iovec = cb->sw.cb_iovstart;
		cb->sh.cb_count = roundup(bp->b_bcount, (u_long) DEV_BSIZE);
		cb->sh.cb_cmd = (command == SCSI_READ) ? SCB_READ : SCB_WRITE;
		cb->sh.cb_cmd |= flags;
		cb->sh.cb_scmd[0] = command;
		cb->sh.cb_scmd[1] |= cb->sw.cb_scsi_device << SCSI_LUNSHFT;
		cb->sh.cb_scmd[1] |= (u_char) ((int) bp->b_resid >> 16);
		cb->sh.cb_scmd[2] = (u_char) ((int) bp->b_resid >> 8);
		cb->sh.cb_scmd[3] = (u_char) bp->b_resid;
		cb->sh.cb_scmd[4] = (u_char) howmany(bp->b_bcount, DEV_BSIZE);
		cb->sh.cb_clen = SCSI_CMD6SZ;
		break;

	default:
		CPRINTF("wd: wd_command unsupported SCSI command %d\n", 
		        command);
		return;
	}

#ifdef	WDDEBUG
	printf("wd_command: About to interrupt ssm, cb =\n");
	wd_printcb(cb);
#endif	WDDEBUG
	/*
	 * start the SSM SCSI command by interrupting the SSM
 	 */
	sx = splhi();
	mIntr(cb->sw.cb_slic, SCSI_BIN, cb->sw.cb_unit_index);
	splx(sx);
}

/*
 * wdsize
 *	Used for swap-space partition calculation.
 *
 * Called by config
 *
 * Doesn't assume that the device is open or alive at all
 * Hence there are more parameter checks than are really needed.
 * Partitions can be designated as going out to the end of the
 * drive by specifying the length as WD_END.  When this is done,
 * the actual drive capacity is used.
 */
wdsize(dev)
	dev_t	dev;
{
	int	part, drive;
	register struct	partition *pt;
	register wd_info *ip;

	drive = VUNIT(dev);
	part = VPART(dev);
	ip = wdinfo[drive];

	if(ip == NULL				/* set in boot? */
	|| ip->wd_part == NULL			/* table present? */
	|| drive >= wdmaxminor			/* Binary conf'd? */
	|| !ip->wd_ssm_desc->sdv_alive 		/* Passed probing? */
	|| ip->wd_size == 0)
		return (-1);

	pt = &(ip->wd_part->v_part)[part];

	if (pt->p_size == 0)			/* valid partition? */
		return(-1);

	return(pt->p_size);
}

/*
 * wdioctl
 */
/* ARGSUSED */
wdioctl(dev, cmd, addr)
	int	cmd;
	dev_t	dev;
	caddr_t	addr;
{
	wd_info *ip;
	struct vtoc *vtp;
	int i, errno;
	struct scsidev *scsidev;


	ip = wdinfo[VUNIT(dev)];

	switch (cmd) {

	case RIOFIRSTSECT:
		*(int *)addr = 0;
		return(0);

	case RIODRIVER:		/* success indicates we're a driver! */
		return(0);

	case SIOCDEVDATA:
		scsidev = (struct scsidev *)addr;
		scsidev->scsi_devno = ip->wd_devno;
		scsidev->scsi_ctlr = ip->wd_ssm_desc->sdv_ssm_idx;
		return(0);

	case SIOCSCSICMD:

		if (!suser())
			return(EACCES);
		/* SCSI commands */
		return(wd_ioctlcmd(dev, ip, (struct scsiioctl *)addr));

	case V_READ:
		vtp = *(struct vtoc **)addr;
		if (!readdisklabel(dev, wdcstrat, ip->wd_part, 
					 (struct cmptsize *)0, (daddr_t)0))
			return(EINVAL);
		return( copyout((caddr_t)ip->wd_part, (caddr_t)vtp,
						sizeof(struct vtoc)));
	case V_WRITE:
		vtp = (struct vtoc *)wmemall(sizeof(struct vtoc), 1);
		if (copyin(*((caddr_t *)addr), (caddr_t)vtp, 
							  sizeof(struct vtoc))) {
			wmemfree((caddr_t)vtp, sizeof(struct vtoc));
			return(EFAULT);
		}
		if (!readdisklabel(dev, wdcstrat, ip->wd_part, 
					 ip->wd_compat, (daddr_t)0)) {
			wmemfree((caddr_t)vtp, sizeof(struct vtoc));
			return(EINVAL);
		}
		if (errno = setdisklabel(ip->wd_part, vtp, ip->wd_opens)) {
			wmemfree((caddr_t)vtp, sizeof(struct vtoc));
			return ((errno > 0) ? errno : EINVAL);
		}
		wmemfree((caddr_t)vtp, sizeof(struct vtoc));
		return(writedisklabel(dev, wdcstrat, ip->wd_part, (daddr_t)0));

	case V_PART:
		if (!readdisklabel(dev, wdcstrat, ip->wd_part, 
					 ip->wd_compat, (daddr_t)0))
			return(EINVAL);
		/*
		 * convert any end of disk (WD_END) to the capacity
		 */
		vtp = ip->wd_part;
		for (i = 0; i < vtp->v_nparts; i++) {
			if ((vtp->v_part[i].p_size == WD_END) ||
			    (vtp->v_part[i].p_start + vtp->v_part[i].p_size >
								ip->wd_size)) {
				vtp->v_part[i].p_size = 
				  ip->wd_size - vtp->v_part[i].p_start;
			}
			if (vtp->v_part[i].p_start > ip->wd_size) {
				vtp->v_part[i].p_start = 0;
				vtp->v_part[i].p_size = 0;
			}
		}
		vtp = *(struct vtoc **)addr;
		return( copyout((caddr_t)ip->wd_part, (caddr_t)vtp,
						sizeof(struct vtoc)));
	}
	return(ENXIO);				/* not supported yet */
}

static
wd_doioctl(cb, bp)
	struct scsi_cb	*cb;
	struct buf	*bp;
{
	spl_t s;

	cb->sw.cb_bp = bp;
	cb->sw.cb_errcnt = 0;

	bzero((caddr_t)SWBZERO(cb), SWBZERO_SIZE);
	if (bp->b_bcount) {
#ifdef WDDEBUG
		printf("wd_doioctl: get scb_buf_iovects\n");
#endif /* WDDEBUG */
		if ((cb->sh.cb_addr = scb_buf_iovects(bp, cb->sw.cb_iovstart)) 
		     == NULL) {
			panic("wd_doioctl: scb_buf_iovects error");
			/*
			 *+ The wd driver was unable to perform
			 *+ DMA mapping for a requested data
			 *+ transfer.  Either the transfer length
			 *+ or the alignment exceeds the resources for
			 *+ this device.
			 */
		} else
			cb->sh.cb_iovec = cb->sw.cb_iovstart;
	}
	cb->sh.cb_count = bp->b_bcount;
	cb->sh.cb_cmd = ((bp->b_flags & B_READ) ? SCB_READ : SCB_WRITE) 
			| SCB_IENABLE;
	cb->sh.cb_clen = ((struct scsiioctl *)bp->b_sioctl)->sio_cmdlen;
#ifdef WDDEBUG
	printf("wd_doioctl: cmdlen = %d, cmd = 0x%x\n",
		  cb->sh.cb_clen, cb->sh.cb_cmd);
#endif /* WDDEBUG */
	bcopy((caddr_t)(&((struct scsiioctl *)bp->b_sioctl)->sio_cmd),
	      (caddr_t)cb->sh.cb_scmd, cb->sh.cb_clen);	

	s = splhi();
	mIntr(cb->sw.cb_slic, SCSI_BIN, cb->sw.cb_unit_index);
	splx(s);
}

static
wd_ioctlcmd(dev, ip, sio)
	dev_t dev;
	wd_info *ip;
	struct scsiioctl *sio;
{

	struct buf *bp;			/* ioctl buffer */
	int dir = 0;			/* zero == READ */
	int error;
	spl_t s;

	bp = &ip->wd_rbufh;

	/* make sure command is for the current device! */
	if (SCSI_UNIT(sio->sio_cmd6.cmd_lun >> SCSI_LUNSHFT)
	     != SCSI_UNIT(ip->wd_devno)) {
#ifdef WDDEBUG
		printf("wd_ioctlcmd: bad lun 0x%x for 0x%x, cmd_lun 0x%x\n",
			 SCSI_UNIT(sio->sio_cmd6.cmd_lun >> SCSI_LUNSHFT),
			 SCSI_UNIT(ip->wd_devno), sio->sio_cmd6.cmd_lun);
#endif /* WDDEBUG */
		return(EINVAL);
	}
	if ((int)sio->sio_datalength < 0) {		/* invalid parm */
#ifdef WDDEBUG
		printf("wd_ioctlcmd: negative data length\n");
#endif /* WDDEBUG */
		return(EINVAL);
	}

	switch(sio->sio_cmd6.cmd_opcode) {

	case SCSI_FORMAT:
#ifdef WDDEBUG
		printf("wd_ioctlcmd: SCSI_FORMAT\n");
#endif /* WDDEBUG */
		sio->sio_cmdlen = SCSI_CMD6SZ;
		break;

	case SCSI_INQUIRY:
#ifdef WDDEBUG
		printf("wd_ioctlcmd: SCSI_INQUIRY\n");
#endif /* WDDEBUG */
		sio->sio_cmdlen = SCSI_CMD6SZ;
		break;

	case SCSI_READC:
#ifdef WDDEBUG
		printf("wd_ioctlcmd: SCSI_READC\n");
#endif /* WDDEBUG */
		sio->sio_cmdlen = SCSI_CMD10SZ;
		break;

	case SCSI_REASS:
#ifdef WDDEBUG
		printf("wd_ioctlcmd: SCSI_REASS\n");
#endif /* WDDEBUG */
		sio->sio_cmdlen = SCSI_CMD6SZ;
		dir = 1;
		break;

	case SCSI_READ_DEFECTS:
#ifdef WDDEBUG
		printf("wd_ioctlcmd: SCSI_READ_DEFECTS\n");
#endif /* WDDEBUG */
		sio->sio_cmdlen = SCSI_CMD10SZ;
		break;

	default:
#ifdef WDDEBUG
		printf("wd_ioctlcmd: default - not supported yet\n");
#endif /* WDDEBUG */
		return(EINVAL);

	} /* end switch */

#ifdef WDDEBUG
	printf("addr = 0x%x, datalength = %d\n", sio->sio_addr,
		  sio->sio_datalength);
#endif /* WDDEBUG */
	/*
	 * Make sure user has access to specified data buffer
	 */
	if (sio->sio_datalength)
		if (!useracc((char *)sio->sio_addr,
			     (u_int)sio->sio_datalength,
			     ((dir) ? B_WRITE : B_READ))) {
#ifdef WDDEBUG
			printf("wd_ioctl: no user access\n");
#endif /* WDDEBUG */
			return(EFAULT);
		}

	/*
	 * Allocate buffer and set fields
	 */
	bufalloc(bp);
	bp->b_flags = ((dir) ? B_WRITE : B_READ) | B_IOCTL;
	bp->b_bcount = sio->sio_datalength;
	bp->b_un.b_addr = (caddr_t)sio->sio_addr;
	bp->b_sioctl = (long)sio;
	bp->b_dev = dev;
	bp->b_blkno = 0;
	bp->b_error = 0;
	bp->b_proc = u.u_procp;
	bp->b_iotype = B_RAWIO;
	BIODONE(bp) = 0;
	/*
	 * Lock process and specified pages in memory
	 */
#ifdef WDDEBUG
	printf("wd_ioctl: locking proc and pages\n");
#endif /* WDDEBUG */
	++u.u_procp->p_noswap;
	if (bp->b_bcount)
		vslock(bp->b_un.b_addr, (int)bp->b_bcount,
			(bool_t)(bp->b_flags & B_READ));

	/*
	 * Insert request at front of queue
	 */
	s = p_lock(&ip->wd_lock, WDSPL);
#ifdef WDDEBUG
	printf("wd_ioctl: insert in queue\n");
#endif /* WDDEBUG */
	bp->av_forw = ip->wd_bufh.av_forw;
	ip->wd_bufh.av_forw = bp;
	wdstart(ip);
	v_lock(&ip->wd_lock, s);

#ifdef WDDEBUG
	printf("wd_ioctl: wait\n");
#endif /* WDDEBUG */
	biowait(bp);
	--u.u_procp->p_noswap;
	error = geterror(bp);
	buffree(bp);
#ifdef WDDEBUG
	printf("wd_ioctl: return %d\n", error);
#endif /* WDDEBUG */
	return(error);
}

/* 
 * wd_cbfin 
 * polls for SSM command completion and returns completion status.
 */
static
wd_cbfin(cb)
	struct scsi_cb	*cb;
{
	int error = 0;
	struct scrsense *rsense;

#ifdef	WDDEBUG
	printf("wd_cbfin: In wd_cbfin\n");
#endif	WDDEBUG
	while (error++ < 4) {
		/*
	 	 * Firmware will timeout if commands take to long
		 */
#ifdef	WDDEBUG
	printf("wd_cbfin: Waiting for compcode\n");
#endif	WDDEBUG
		while (cb->sh.cb_compcode == SCB_BUSY)
			;
#ifdef	WDDEBUG
	printf("wd_cbfin: Got compcode\n");
#endif	WDDEBUG
		switch(cb->sh.cb_compcode) {
		case SCB_BAD_CB:
		case SCB_NO_TARGET:
		case SCB_SCSI_ERR:
			 goto restart ;
		
		case SCB_OK:
			if (SCSI_CHECK_CONDITION(cb->sh.cb_status)) {
				rsense = (struct scrsense *) cb->sh.cb_sense;
				if ((rsense->rs_error & RS_ERRCODE) 
				     != RS_CLASS_EXTEND)
					goto restart ;
				else if ((rsense->rs_key & RS_KEY)==RS_RECERR)
					return SCB_OK;
				else
					goto restart ;
			} else			/* No check condition */
				return SCB_OK;

		default:
			goto restart ;
	
		}
restart:		wd_command(cb, cb->sh.cb_scmd[0], 0);
	}

	switch(cb->sh.cb_compcode) {
	case SCB_BAD_CB:
	case SCB_NO_TARGET:
	case SCB_SCSI_ERR:
		 return(-1);
	
	case SCB_OK:
		if (SCSI_CHECK_CONDITION(cb->sh.cb_status)) {
			rsense = (struct scrsense *) cb->sh.cb_sense;
			if ((rsense->rs_error & RS_ERRCODE) != RS_CLASS_EXTEND) {
				return -1 ;
			} 
       	 	 	if ((rsense->rs_key & RS_KEY) == RS_RECERR) 
				return SCB_OK;

		return -1 ;
		}

		if (SCSI_CHECK_CONDITION(cb->sh.cb_status)) {
			rsense = (struct scrsense *) cb->sh.cb_sense;
			if ((rsense->rs_error & RS_ERRCODE) 
			     != RS_CLASS_EXTEND)
				return -1;
			else if ((rsense->rs_key & RS_KEY)==RS_RECERR)
				return SCB_OK;
			else
				return -1;
		} else			/* No check condition */
			return SCB_OK;

	default:
		return -1 ;
	}
	
}

/* 
 * wdp_cbfin 
 * polls for SSM command completion and retries upto four times.
 * This is used during probe of devices and does not attempt to
 * interpret errors.
 */
static
wdp_cbfin(cb)
	struct scsi_cb	*cb;
{

	/*
 	 * Firmware will timeout if commands take to long
	 */
	while (cb->sh.cb_compcode == SCB_BUSY)
		;

	if (cb->sh.cb_compcode == SCB_OK && 
	    !SCSI_CHECK_CONDITION(cb->sh.cb_status))
		return (cb->sh.cb_compcode);
	else
		return (cb->sh.cb_compcode);
}

/*
 * wdp_command - start a probe command
 *
 * neccesary because cb's are not initialized yet
 */
static
wdp_command(cb, cmd, sense, buffer, devno)
	struct scsi_cb	*cb;
	u_char	cmd;
	daddr_t	sense;
	daddr_t	buffer;
	short	devno;
{
	cb->sh.cb_sense = sense;
	cb->sh.cb_slen = SIZE_MAXDATA;
	cb->sw.cb_data = buffer;
	cb->sw.cb_scsi_device = devno;
	wd_command(cb, cmd, 0);
	return (wdp_cbfin(cb));
}

/*
 * wd_printcb
 *
 * prints out the fields of a command block
 */
static
wd_printcb(cb)
	struct scsi_cb *cb;
{
	int x;

	CPRINTF("command: ");
#ifdef	WDDEBUG
	printf("0x%x 0x%x 0x%x \n", cb->sh.cb_cmd,
		cb->sh.cb_reserved0[0],
		cb->sh.cb_clen); 
#endif	WDDEBUG
	for (x = 0; x < cb->sh.cb_clen; x++) 
		CPRINTF("0x%x ", cb->sh.cb_scmd[x]);
	CPRINTF("\n");
#ifdef WDDEBUG
	printf("0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", 
		*(u_long*) &cb->sh.cb_reserved1[0],	 
		cb->sh.cb_addr,		
		(u_long) cb->sh.cb_iovec,
		cb->sh.cb_count,
		cb->sh.cb_status, 
		*(u_short*) &cb->sh.cb_reserved2[0],
		cb->sh.cb_compcode);
	printf("0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", 
		(u_long) cb->sw.cb_bp, 
		(u_long) cb->sw.cb_iovstart,
		cb->sw.cb_errcnt,
		cb->sw.cb_unit_index,
		cb->sw.cb_scsi_device,
		cb->sw.cb_state,
		cb->sw.cb_data,
		cb->sw.cb_iovnum,
		cb->sw.cb_slic); 
#endif WDDEBUG
}

/*
 * wd_printsense - prints sense information returned from a
 * 	SCSI request sense command
 */
static
wd_printsense(rsense)
	struct scrsense *rsense;
{
	int more, x;
	u_char *moredat;

	CPRINTF("sense: ");
	if ((rsense->rs_error & RS_ERRCODE) != RS_CLASS_EXTEND) 
		CPRINTF("Don't understand request sense data\n"); 
	
	CPRINTF("Error 0x%x, Seg Num 0x%x, Key 0x%x, Info 0x%x, Additional 0x%x, ", 
		rsense->rs_error,
		rsense->rs_seg,
		rsense->rs_key, 
		*(u_long *) &rsense->rs_info[0],
		rsense->rs_addlen);
	
	more = (int) rsense->rs_addlen;
	if (more > WDMAXDATASZ - sizeof (struct scrsense))
		more = WDMAXDATASZ - sizeof (struct scrsense);
	moredat = (u_char *) ((int) (&rsense->rs_addlen) + 1);
	for (x = 0; x < more; x++) 
		CPRINTF("0x%x ", moredat[x]);
}

/*
 * wd_shutdrive
 * Fail all I/O requests queued to the drive.
 * Currently active I/O requests will finish and fail asyncronously.
 * The drive is marked as WD_BAD
 */
static
wd_shutdrive(ip)
	register wd_info *ip;
{
	register struct buf *bp, *nextbp;
	spl_t s;

	if (ip->wd_flags & WD_BAD)
		return;

	bp = ip->wd_bufh.av_forw;
	while (bp != NULL) {
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		bp->b_resid = bp->b_bcount;
		nextbp = bp->av_forw;
#ifdef	WDDEBUG
		printf("wd_shutdrive: Printing bp\n");
		wd_dump(sizeof(struct buf), bp);
#endif	WDDEBUG
		iodone(bp);
		bp = nextbp;
	}

	ip->wd_bufh.av_forw = NULL;
	ip->wd_flags |= WD_BAD;
	if (fp_lights) {
		s = splhi();
		if (ip->wd_inuse)
			FP_IO_INACTIVE;
		FP_IO_ERROR;
		splx(s);
	}
	disk_offline();
}

/*
 * wd_error - disk access error reporting
 */
static
void
wd_error(cb, bp, str)
	struct scsi_cb *cb;
	struct buf *bp;
	char *str;
{
	struct scrsense *rsense;
	char	*command, *key, *code, *completion;
	long lba;

	rsense = (struct scrsense *) cb->sh.cb_sense;	

	command = (bp->b_flags & B_READ) ? "READ" : "WRITE";

	if (cb->sh.cb_compcode >= wdncompcodes)
		completion = wd_compcode[wdncompcodes - 1];
	else
		completion = wd_compcode[cb->sh.cb_compcode];

	if (rsense->rs_key & RS_KEY) {

		if (rsense->rs_key >= wdnsensekey)
			key = wd_sensekey[wdnsensekey -1];
		else
			key = wd_sensekey[rsense->rs_key];

		if (((caddr_t) cb->sh.cb_sense)[12] >= wdnaddsense)
			code = wd_addsense[wdnaddsense -1];
		else
			code = wd_addsense[((caddr_t) cb->sh.cb_sense)[12]];

		/* 
		 * If the sense data is valid then retrieve the
		 * logical block address in question from the
		 * returned sense data.  Otherwise use the
		 * starting block address from the buf-structure.
		 */
		if (rsense->rs_error & RS_VALID) {
			lba = (long)rsense->rs_info[0] << 24;
			lba |= (long)rsense->rs_info[1] << 16;
			lba |= (long)rsense->rs_info[2] << 8;
			lba |= (long)rsense->rs_info[3];
		} else
			lba = bp->b_blkno;

		CPRINTF("wd%d%c: %s %s, %s, %s, lba=%d, %s\n",
			VUNIT(bp->b_dev),
			WD_PRTCHR(bp->b_dev),
			str,
			command,
			key,
			code,
			lba,
			completion
		);
	} else {
		CPRINTF("wd%d%c: %s %s, lba=%d, %s\n",
			VUNIT(bp->b_dev),
			WD_PRTCHR(bp->b_dev),
			str,
			command,
			bp->b_blkno,
			completion
		);
	}
		
	
	CPRINTF("wd%d%c: ", VUNIT(bp->b_dev), WD_PRTCHR(bp->b_dev));
	wd_printcb(cb);

	if (rsense->rs_key & RS_KEY) {
		CPRINTF("wd%d%c: ",VUNIT(bp->b_dev), WD_PRTCHR(bp->b_dev));
		wd_printsense(rsense);
	}
}

#ifdef	WDDEBUG
wd_dump(len, ptr)
	int len;
	char * ptr;
{
	int	x;
	
	for (x = 0; x < len; x++) {
		printf("%x ", (u_char) ptr[x]);
		if ((x % 25 == 0) && (x != 0))
			printf("\n");
	}
	printf("\n");
}

wd_fill(len, ptr)
	int len;
	char * ptr;
{
	int	x;
	
	for (x = 0; x < len; x++) 
		ptr[x] = 55;
}
#endif	WDDEBUG

/*
 * wdcstrat - SSM SCSI disk strategy routine
 *	 	This function does not check limits on the disk.
 */
wdcstrat(bp)
	register struct buf *bp;
{
	register wd_info *ip;
	spl_t	s;

	ip = wdinfo[VUNIT(bp->b_dev)];

	/*
	 * Size and partitioning check.
	 *
 	 * Fail request on: bad byte count, transfer not entirely 
	 * within a partition, or if it is not aligned properly.
	 * Or if drive went bad.
	 * If the disk has just been formatted, do not check bounds.
	 */
	if ((bp->b_bcount <= 0)
	|| ((bp->b_iotype == B_RAWIO) &&
	   (((int)bp->b_un.b_addr & (SSM_ALIGN_XFER - 1)) != 0))) {
		bp->b_resid = bp->b_bcount;	
		bp->b_flags |= B_ERROR;
		bp->b_error = EINVAL;
#ifdef	WDDEBUG
		printf("wdstrat: Printing bp\n");
		wd_dump(sizeof(struct buf), bp);
#endif	WDDEBUG
		iodone(bp);
		return;
	}

	bp->b_resid = bp->b_blkno; 		/* req'd by disksort */

	s = p_lock(&ip->wd_lock, WDSPL);
	disksort(&ip->wd_bufh, bp);
	wdstart(ip);
	v_lock(&ip->wd_lock, s);
}

