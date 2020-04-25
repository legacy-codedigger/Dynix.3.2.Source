/* $Copyright:	$
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
static	char	rcsid[] = "$Header: sd.c 2.36 91/01/15 $";
#endif

/*
 * sd.c 
 *	SCSI disk device driver
 */

/* $Log:	sd.c,v $
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
#include "../h/cmn_err.h"

#include "../balance/engine.h"
#include "../balance/slic.h"

#include "../machine/ioconf.h"
#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/plocal.h"

#include "../sec/sec.h"			/* SCSI common data structures */
#include "../sec/sec_ctl.h"		/* SCSI drivers' common stuff */
#include "../sec/sd.h"			/* driver local structures */
#include "../sec/scsi.h"
#include "../sec/scsiioctl.h"

/*
 * Externs and global data structures.
 */
extern	gate_t	sdgate;			/* Gate number for locks... */
extern	struct	sd_bconf sdbconf[];	/* Binary configuration info (bc)*/
struct	sd_info	**sdifd;		/* Unit interrupt mapping base ptr */
extern	u_char *SEC_rqinit();
extern	struct timeval	time;
extern	caddr_t topmem;			/* highest addressable location */
extern	u_char sddevtype;		/* byte 0 of INQUIRY return data */
extern	u_char sdinq_targformat;	/* byte 3 of above on target adaptors */
extern	u_char sdinq_ccsformat;		/* byte 3 of above on embedded SCSI */
extern	bool_t CCS_present;		/* is there an embedded SCSI drive */

extern int	sdsensebuf_sz;		/* Sense buffer info - bc */
extern int	sdmaxminor;		/* Device numbering info - bc */
extern int	sdretrys;		/* Number of retrys before giving up */

#ifdef SDDEBUG
#define SCSIFIRMWARE_SANITY 
#endif SDDEBUG

int	sd_baselevel;			/* Base interrupt lvl for all devices*/


int	sdstrat(), sdiatsz();		/* Forward references */
int	sdprobe(), sdboot(), sdintr();
daddr_t sdreadc();
int	sdcstrat();

struct sec_driver sd_driver = {
/*	name base	flags			 probe	  boot	  intr */
	"sd", 0x20,	SED_TYPICAL|SED_IS_SCSI, sdprobe, sdboot, sdintr
};


/*
 * Driver development helpers.
 */
#ifdef SDDEBUG
int	sd_debug = 4;	/* 0=off 1=little 2=more 3=lots >3=all */
#endif SDDEBUG

/*
 * The following data structures are only used for probing
 */
struct sec_dev_prog sd_devprog;	/* one device program */
u_char *sd_datap = 0;
u_char *sd_data_queue;

/*
 * sdprobe(sd) - probe procedure 
 *	struct sec_dev *sd;
 *
 * This procedure polls a device with the test unit ready command
 * to determine if the device is present.
 *
 * Note: This procedure tracks the header file sec.h
 * which must track the firmware for queue size information.
 * Also the status address MUST be below 4 Meg because the h/w
 * dma can not address above 4 Meg.
 */
sdprobe(sed)
	struct sec_probe *sed;
{
	int resp;
	struct sdreqsense *sense;
	struct sdinq *sdinq;

	if (! sd_datap) {
		sd_datap = (u_char *) calloc(SDMAXDATASZ);
		sd_data_queue = SEC_rqinit(sd_datap, SDMAXDATASZ);
	}

	if ((resp = sd_docmd_pr(SDC_TEST, sed)) == 2) {

		/*
		 * could be a CCS disk - if so, we need to clear out
		 * the unit attention error condition.
		 */

		if (sd_docmd_pr(SDC_REQUEST_SENSE, sed) != 0)
			return(SECP_NOTFOUND);

		sense = (struct sdreqsense *) sd_datap;
		if ((sense->sdr_sensekey & SD_SENSEKEYMASK) != SD_UNIT_ATTN)
			return(SECP_NOTFOUND);

	/*
	 * timeout occurred - no target present
	 */

	} else if (resp == 1) {
		return(SECP_NOTFOUND|SECP_NOTARGET);

	} else if (resp != 0) {
		return(SECP_NOTFOUND);
	}

	/*
	 * discover the type of target adaptor
	 */

	if (sd_docmd_pr(SDC_INQUIRY, sed) != 0)
		return(SECP_NOTFOUND);
	sdinq = (struct sdinq *) sd_datap;

	/*
	 * confirm that this is a disk.  This keeps the sd driver
	 * from responding to a SCSI tape.
	 */

	if (sdinq->sdq_devtype != sddevtype)
		return(SECP_NOTFOUND);

	/*
	 * non-CCS format disk
	 */

	if (sdinq->sdq_format == sdinq_targformat)
		return(SECP_FOUND);

	if (sdinq->sdq_format != sdinq_ccsformat) {
		printf("sd: not a supported embedded SCSI drive, target %d, unit %d\n",
			sed->secp_target, sed->secp_unit);
                /*
                 *+ While the sd driver was probing for SCSI disks on an SEC,
                 *+ it found a drive
                 *+ that does not have the right format for the CCS drives.
                 *+ The driver supports only CCS drives.  Use a supported drive.
                 */

		return(SECP_NOTFOUND|SECP_ONELUN);
	}

	/*
	 * we have found a CCS drive.
	 */

	CCS_present = 1;
	return(SECP_FOUND|SECP_ONELUN);
}


/*
 * sdboot - initialize all channels of this device driver.
 *
 * Called once after all probing has been done.
 *
 * This procedure initializes and allocates all device driver data
 * structures based off of the configuration information passed in from
 * autoconfig() and from the device drivers binary configuration tables.
 * The boot procedure also maps interrupt levels to unit number by
 * placing the channels communications structure pointer into a
 * major/minor number mapped dynamically allocated array (sdifd[]).
 */
sdboot(ndevs, sed_array)
	int ndevs;
	struct sec_dev sed_array[];
{
	register struct sec_dev *sed;
	register struct	sd_info *ifd;
	register struct	seddc *dc;
	struct	sd_bconf    *bconf;	/* Binary configuration info */
	int dev;
	daddr_t	capacity;
	struct v_open *vo;

	sdifd = (struct sd_info **)calloc(((sdmaxminor+1) * (sizeof(struct sd_info *))));
	sd_baselevel = sed_array[0].sd_vector;
	
	/*
	 * for each configured device
	 */
	for (dev = 0; dev < ndevs; ++dev) {
		sed = &sed_array[dev];
#ifdef SDDEBUG
		if(sd_debug) {
			printf("sdboot: info=0x%x cib=0x%x ta=%d un=%d bin=%d slic=%d vec=%d, alive is %s\n",
				sdifd,
				sed->sd_cib,
				sed->sd_target,
				sed->sd_unit,
				sed->sd_bin,
				sed->sd_desc->sec_slicaddr,
				sed->sd_vector,
				(sed->sd_alive ? "Found" : "Not Found")
			);
		}
#endif SDDEBUG
		if(!sed->sd_alive)
			continue;

		/*
		 * Verify that this device has been included into
		 * the devices binary configuration table. 
		 */
		if(dev >= sdmaxminor) {
			CPRINTF("sd%d: non-binary configured device found in config table ... deconfiguring.\n", dev);
			sed->sd_alive = 0;	/* force deconfigure */
			continue;
		}

		if (topmem > (caddr_t) MAX_SCED_ADDR_MEM) {
			CPRINTF("sd%d: Unsupported memory configuration with SCSI disks ... deconfiguring.\n", dev);
			sed->sd_alive = 0;
			continue;
		}

		/*
		 * determine the disk's usable size by doing a
		 * READ CAPACITY scsi command and shaving off the
		 * last three cylinders, (used by diagnostics)
		 */

		if ((capacity = sdreadc(sed)) <= 0) {
			CPRINTF("sd%d: problem doing READ CAPACITY %s\n",
			dev, "... deconfiguring");
			sed->sd_alive = 0;
			continue;
		}

		/*
		 * Initialize the device structure and
		 * copy pertenent info into it for faster/easier access.
		 */

		bconf = &sdbconf[dev];

		/*
		 * round number of iat's up to minimum boundary
		 */
		bconf->bc_num_iat = MAX(4, bconf->bc_num_iat);

		/*
		 * Info structure.
		 */
		sdifd[dev] = ifd = (struct sd_info *)calloc(sizeof(struct sd_info));
#ifdef SDDEBUG
		if(sd_debug)
			printf("ifd=0x%x, info=0x%x\n", ifd, sdifd[dev]);
#endif SDDEBUG
		vo = (struct v_open *)calloc(sizeof(struct v_open));
		ifd->sd_part = &vo->v_v;
		ifd->sd_opens = &vo->v_vo.v_opens[0];
		ifd->sd_modes = &vo->v_vo.v_modes[0];
		ifd->sd_desc = sed;
		ifd->sd_lun = sed->sd_unit << SCSI_LUNSHFT;
		ifd->sd_size = capacity + 1;
		/* Point to compatibility partition info from conf_sd.c */
		ifd->sd_compat = bconf->bc_part;
		ifd->sd_retrys = sdretrys;
		ifd->sd_vtoc_read = 0;
		ifd->sd_thresh = MIN(bconf->bc_thresh, sed->sd_doneq_size);
		ifd->sd_low = MIN(bconf->bc_low, sed->sd_doneq_size);
		if(ifd->sd_thresh <= 0)
			CPRINTF("sd%d: Warning queueing disabled\n", dev);

		/*
		 * Ioctl raw buffer.
		 */
		ifd->sd_rawbuf = (caddr_t)calloc(bconf->bc_rawbuf_sz * sizeof(unsigned char)); /* next free mem */

		/*
		 * Init locks, semas ...
		 */
		init_lock(&ifd->sd_lock, sdgate);
		init_sema(&ifd->sd_sema, 0, 0, sdgate);
		init_sema(&ifd->sd_usrsync, 1, 0, sdgate);
		bufinit(&ifd->sd_rbufh, sdgate);

		/*
		 * Init channel structure.
		 */
		ifd->sd_dc = dc = (struct seddc *)calloc(sizeof(struct seddc));		/* next free mem */

		/* 
		 * Device program for a request sense operation
		 * (separate to avoid fussing diq ptrs too much)
		 */
		dc->dc_sense = (struct sec_dev_prog *)calloc(sizeof(struct sec_dev_prog)); /* sense info */

		/*
		 * Set up the iat table area
		 */
		dc->dc_iat = (struct sec_iat *) calloc(sizeof(struct sec_iat) * bconf->bc_num_iat); /* next free mem */
		dc->dc_isz = bconf->bc_num_iat;
		dc->dc_ifree = dc->dc_isz;

		/*
		 * Device input queue.
		 */
		dc->dc_diq = sed->sd_requestq;
		SEC_fill_progq(dc->dc_diq, sed->sd_req_size, sizeof(struct sec_dev_prog));
		dc->dc_devp = dc->dc_diq->pq_un.pq_progs[0];
		dc->dc_dsz = sed->sd_req_size;
		dc->dc_dfree = dc->dc_dsz;
		ASSERT(dc->dc_dsz > 0, "sdboot: no device programs");
                /*
                 *+ There is a problem in initializing the
                 *+ sd driver.  The number of device programs
                 *+ configured for this device is not valid.
                 */

		/*
		 * Device Output queue.
		 */
		dc->dc_doq = sed->sd_doneq;

		/*
		 * Request sense information buffer,
		 * one per channel.
		 * Note: Assumes channel can only generate one fault
		 * at a time.
		 */
		ifd->sd_sensebuf = (u_char *)calloc(sizeof(u_char) * sdsensebuf_sz);
		ifd->sd_sensebufptr = SEC_rqinit(ifd->sd_sensebuf, sdsensebuf_sz);

#ifdef SDDEBUG
		if(sd_debug>1) {
			printf("diq0x%x,doq0x%x,cib0x%x,bin%d,device%d,vector%d, all at 0x%x\n",dc->dc_diq,
			dc->dc_doq, sed->sd_cib, sed->sd_bin, dev, sed->sd_vector);
			printf("info=0x%x, ifd=0x%x\n",ifd,sdifd[dev]);
		}
#endif SDDEBUG
		/*
		 * Initialize the statistics 
		 *
		 * Note: No seek statistics are
		 * recorded because of the logical SCSI interface.
		 */
		if(dk_nxdrive < dk_ndrives) {
			ifd->sd_stat_unit = dk_nxdrive;
			ifd->sd_dk = &dk[dk_nxdrive++];
			bcopy("sdX", ifd->sd_dk->dk_name , 4);
			ifd->sd_dk->dk_name[2] = '0' + dev;
			ifd->sd_dk->dk_bps = bconf->bc_blks_per_sec;
		}else
			ifd->sd_stat_unit = SD_NO_UNIT;
	}

#ifdef SDDEBUG
	printf("sd_debug@0x%x, set to %d\n", &sd_debug, sd_debug);
#endif SDDEBUG

}

/*
 *	sdopen - open a channel
 *
 *	Check for validity of device.
 *
 *	Call readdisklabel() to read vtoc.  If no vtoc, we will get
 *	default partition info as listed in conf_sd.c.
 *
 *	Tmp assumptions: No mods needed for TMP because sdopen doesn't
 *	*modify* any concurrently accessed common data structures.
 *
 *	This assumes that the device is formated in DEV_BSIZE size blocks
 *	and won't work other wise.
 */

sdopen(dev, flags)
	dev_t	dev;
	int	flags;
{
	register int unit;
	register int part;
	register struct sd_info	*ifd;
	spl_t	s;
	int	err;
	int	i;
	struct	vtoc	*vtp;

#ifdef SDDEBUG
	if(sd_debug)
		printf("O");
#endif SDDEBUG
	unit = VUNIT(dev);
	part = VPART(dev);
	err = 0;

	ifd = sdifd[unit];
	if(ifd == NULL || ifd->sd_part == NULL)	/* Configured properly? */
		return(ENXIO);

	if( unit >= sdmaxminor			/* Valid channel number? */
	|| (!ifd->sd_desc->sd_alive))			/* Passed probing? */
		return(ENXIO);
	
	p_sema(&ifd->sd_usrsync, PRIBIO);
	s = p_lock(&ifd->sd_lock, SDSPL);
	if (ifd->sd_flags & SDF_EXCLUSIVE) {
#ifdef SDDEBUG
		printf("sdopen: exclusive set - EBUSY\n");
#endif /* SDDEBUG */
		err = EBUSY;
	} else if (flags & FEXCL) {
#ifdef SDDEBUG
		printf("sdopen: FEXCL\n");
#endif /* SDDEBUG */
		if (!suser()) {
#ifdef SDDEBUG
			printf("sdopen: !suser\n");
#endif /* SDDEBUG */
			err = EPERM;
		} else if (ifd->sd_nopen) {
#ifdef SDDEBUG
			printf("sdopen: EBUSY\n");
#endif /* SDDEBUG */
			err = EBUSY;
		} else {
			ifd->sd_flags |= SDF_EXCLUSIVE;
		}
	}
	/*
	 * Ensure that a writer to the V_ALL device holds out all other
	 * accessors of the V_ALL device
	 */
	if (V_ALL(dev) && (ifd->sd_flags & SDF_ALLBUSY))
		err = EBUSY;

	/*
	 * drop lock as this open routine is still protected with
	 * ifd->sd_usrsync.
	 */
	v_lock(&ifd->sd_lock, s);

	if (!err) {
		/*
	 	 * Read vtoc from disk.  If no vtoc, readdisklabel() 
		 * will return default conf_sd.c partition info in
		 * vtoc structure. 
		 */
		err = vtoc_opencheck(dev, flags, sdcstrat,
			ifd->sd_part, ifd->sd_compat, (daddr_t)0,
			ifd->sd_size, ifd->sd_modes[part], "sd");

		if (!err && !V_ALL(dev)) {
			/*
			 * convert any end of disk (SD_END) to the capacity
			 */
			vtp = ifd->sd_part;
			for (i = 0; i < vtp->v_nparts; i++) {
				if (vtp->v_part[i].p_size == SD_END) {
					vtp->v_part[i].p_size = 
				          ifd->sd_size - vtp->v_part[i].p_start;
				}
			}
		}
	}

	if (!err) {
		/*
		 * Succesful open
		 */
		ifd->sd_nopen++;
		if (V_ALL(dev)) {
			/*
			 * Successful open on whole-disk;
			 * lock others out.
			 */
			if (flags & FWRITE){
				s = p_lock(&ifd->sd_lock, SDSPL);
				ifd->sd_flags |= SDF_ALLBUSY;
				v_lock(&ifd->sd_lock, s);
			}
		} else {
			ifd->sd_opens[part] += 1;
			ifd->sd_modes[part] |= flags;
		}
	}
#ifdef SDDEBUG
	printf("sdopen: dev 0x%x -- %d opens %d\n", dev, ifd->sd_nopen,err);
#endif /* SDDEBUG */
	v_sema(&ifd->sd_usrsync);
	return(err);
}


/*
 * sdclose - close a channel 
 *
 * Decrement reference count, make sure temporary settings
 * are set back to their default values.
 */
/*ARGSUSED*/
sdclose(dev, flag)
	dev_t dev;
	int flag;
{
	struct sd_info *ifd;
	spl_t s;
	int part;

	part = VPART(dev);
	ifd = sdifd[VUNIT(dev)];
	s = p_lock(&ifd->sd_lock, SDSPL);
	ifd->sd_nopen--;

	if (!V_ALL(dev)) {
		ifd->sd_opens[part]--;
		ifd->sd_modes[part] &= ~(flag&FUSEM);
	}
	
	ifd->sd_flags &= ~(SDF_EXCLUSIVE|SDF_FORMATTED);

	/*
	 * If this is last close, clear flag so vtoc will be read on
	 * first open.  (Just in case this is a removeable media drive.)
	 */
	if (ifd->sd_nopen == 0) {
		ifd->sd_vtoc_read = 0;
	}

	/*
	 * If we're holding the whole-disk partition, flag it as free now
	 */
	if (V_ALL(dev) && (ifd->sd_flags & SDF_ALLBUSY))
		ifd->sd_flags &= ~SDF_ALLBUSY;

	v_lock(&ifd->sd_lock, s);
#ifdef SDDEBUG
	printf("sdclose: dev 0x%x - nopens = %d\n", dev, ifd->sd_nopen);
#endif /* SDDEBUG */
}


/*
 * sdminphys - correct for too large a request.
 *
 * Note: IATSIZE should reflect typical iat table entry size (currently 1k)
 */
sdminphys(bp)
	register struct buf *bp;
{
	struct sd_info *ifd;
	struct seddc	*dc;

	ifd = sdifd[VUNIT(bp->b_dev)];
	dc = ifd->sd_dc;

#ifdef SDDEBUG
	if(bp->b_bcount<=0) {
		printf("sdminphys: bad bp detected, bcount=%d\n", bp->b_bcount);
		bp->b_bcount = 0;
	}
#endif SDDEBUG
	
#ifdef SCSIFIRMWARE_SANITY 

	/*
	 * The iat size is enough except for the use of an extra
	 * iat used for debug in sdiatsz() for driver fault firmware
	 * tests.
	 *
	 * Added space for an iat entry so that #define SCSIFIRWARE_SANITY
	 * doesn't deadlock on iat allocation in sdstart call.
	 *
	 * buf_iat(bp)+2	1 for scsifirmwaresanity 1 for deadlock
	 * dc_isz-IATVARIANCE-4 (above adds total of 4) plus 1 so that
	 *			non-aligned xfers don't get rounded up.
	 */
	if(buf_iatsz(bp)+2 >= dc->dc_isz)		
		bp->b_bcount = (dc->dc_isz-IATVARIANCE-4)*IATBYTES;
#else

	/*
	 * Waste an extra couple of iat's to insure driver doesn't
	 * deadlock if a request tries to allocate the entire iat
	 * list to a single request.
	 */
	if(buf_iatsz(bp)+1 >= dc->dc_isz)		
		bp->b_bcount = (dc->dc_isz-IATVARIANCE-3)*IATBYTES;
#endif SCSIFIRMWARE_SANITY

#ifdef SDDEBUG
	if(sd_debug>1) 
		printf("sdminphys: cnt=%d, ni=%d\n", bp->b_bcount, buf_iatsz(bp));
	else if(sd_debug)
		printf("M");
#endif SDDEBUG

	/*
	 * Limit the xfer (because of SCSI command spec)
	 * to no more that 255 blocks
	 * at a time (note: 256 blocks are possible).
	 * 255 limit is based on paranoia. A zero count
	 * in the command structure results in xfer of 256 blocks.
	 */
	if(bp->b_bcount > 255*DEV_BSIZE)
		bp->b_bcount = 255*DEV_BSIZE;	
}


/*
 * sdwrite	-	Standard raw write procedure.
 */
sdwrite(dev, uio)
	dev_t	dev;
	struct uio *uio;
{
	int err, diff;
	off_t lim;

#ifdef SDDEBUG
	ASSERT(VUNIT(dev) <sdmaxminor, "sdwrite dev");
	if(sd_debug>1)
		printf("Dev=0x%x\n", dev);
#endif SDDEBUG

	if (V_ALL(dev)) {
		return(physio(sdcstrat, (struct buf *)0, dev, B_WRITE, sdminphys, uio));
	}
	lim = (sdifd[VUNIT(dev)])->sd_part->v_part[VPART(dev)].p_size;
	lim <<= DEV_BSHIFT;
	err = physck(lim, uio, B_WRITE, &diff);
	if (err != 0) {
		if (err == -1)	/* not an error, but request of 0 bytes */
			err = 0;
		return(err);
	}
	err = physio(sdstrat, (struct buf *)0, dev, B_WRITE, sdminphys, uio);
	uio->uio_resid += diff;
	return(err);
}


/*
 * sdread	-	Standard raw read procedure.
 */
sdread(dev, uio)
	dev_t	dev;
	struct uio *uio;
{
	int err, diff;
	off_t lim;

#ifdef SDDEBUG
	ASSERT(VUNIT(dev) < sdmaxminor, "sdread dev");
	if(sd_debug>1)
		printf("Dev=0x%x\n", dev);
#endif SDDEBUG

	if (V_ALL(dev)) {
		return(physio(sdcstrat, (struct buf *)0, dev, B_READ, sdminphys, uio));
	}
	lim = (sdifd[VUNIT(dev)])->sd_part->v_part[VPART(dev)].p_size;
	lim <<= DEV_BSHIFT;
	err = physck(lim, uio, B_READ, &diff);
	if (err != 0) {
		if (err == -1)	/* not an error, but request of 0 bytes */
			err = 0;
		return(err);
	}
	err = physio(sdstrat, (struct buf *)0, dev, B_READ, sdminphys, uio);
	uio->uio_resid += diff;
	return(err);
}


/*
 * sdstrat - SCSI strategy routine.
 *	check the block sizes and limits.
 *
 * TMP assumptions:
 *	Only one processor can access the sd_info structure for the
 *	state variable on each devices channel. Interrupts will block
 *	and spin on this lock for each channel. 
 *
 *	Concurrent interrupts on different channels is assumed possible.
 */
sdstrat(bp)
	register struct	buf	*bp;
{
	register struct	partition *pt;
	register struct	sd_info	*ifd;
	register struct	buf *bufh;
	int	length;
	int	x;

	
	ifd = sdifd[VUNIT(bp->b_dev)];
	pt = &(ifd->sd_part->v_part)[VPART(bp->b_dev)];

	/* 
	 * Grab length of the partition from vtoc info.
	 */
	length = pt->p_size;

#ifdef SDDEBUG
	printf("ifd=0x%x, pt=0x%x un=%d p=%d ", ifd, pt, VUNIT(bp->b_dev), VPART(bp->b_dev));
	printf("start=0x%x, len=0x%x,\n", pt->p_start, length);
	printf("sdstrat(%s) bp=0x%x, dev=0x%x, cnt=%d, blk=0x%x, vaddr=0x%x\n",
		(bp->b_flags & B_READ) ? "READ" : "WRITE",
		bp, bp->b_dev, bp->b_bcount, bp->b_blkno,
		bp->b_un.b_addr);
	ASSERT(ifd->sd_part!=0,"sdstrat: partition ptr zero");
#endif /* SDDEBUG */

	bufh = &ifd->sd_bufh;

	/*
	 * Size and partitioning check.
	 *
	 * Fail request if bogus byte count, if address not aligned to
	 * SD_ADDRALIGN boundary, or if transfer is not entirely within a
	 * disk partition.
	 * If the disk has just been formatted do not check bounds.
	 */
	if ((bp->b_bcount <= 0)
	||  ((bp->b_iotype == B_RAWIO) &&
		(((int)bp->b_un.b_addr & (SD_ADDRALIGN - 1)) != 0))
	||  (!(ifd->sd_flags & SDF_FORMATTED) && 
	      ( (bp->b_blkno >= length) ||
	      ((bp->b_blkno + howmany(bp->b_bcount, DEV_BSIZE)) > length)))) {
		bp->b_resid = bp->b_bcount;
		bp->b_flags |= B_ERROR;
		bp->b_error = EINVAL;
		u.u_error = EINVAL;
		iodone(bp);
		return;
	}

	bp->b_resid = bp->b_blkno + pt->p_start;	/* for disksort */

	/*
	 * Note: To this point in the procedure only static data
	 * has been accessed.
	 */
	 
	x = p_lock(&ifd->sd_lock, SDSPL);
	disksort(bufh, bp);
	if(bufh->b_active == SDS_IDLE) {
		bufh->b_active = SDS_BUSY;
		sdstart(bufh, ifd);
	}
	v_lock(&ifd->sd_lock, x);
}

/*
 * sdstart - start a request to a channel.
 *
 * TMP concerns - bufh access mutexed above.
 * Assumptions:
 *	device program pointers are filled in at boot time
 *	iat table pointers in ifd->sd_dc->dc_iat are filled in at boot time.
 *
 */

sdstart(bufh, ifd)
	register struct buf *bufh;
	register struct sd_info *ifd;
{
	register struct	seddc	*dc;		/* Device control block */
	register struct	sec_dev_prog *devp;	/* Device program pointer */
	register struct	buf	*bp;		/* First request from system */
	struct	buf		*qbp;		/* Last request made to device*/
	struct	sec_iat		*iat;		/* Base iat pointer */
	struct	sec_progq	*diq;		/* Device input queue */
	unsigned char		head;		/* Diq head value */
	int	numiat;				/* Number of iats needed */
	int	numqueue;			/* Number taken off each start */
	int	dstart;				/* Begining of empty slots */


	if(bufh->b_actf == NULL)		/* Check for requests */
		return;
	
	numqueue = ifd->sd_thresh;

	qbp = &ifd->sd_bp;
	dc = ifd->sd_dc;
	dstart = dc->dc_dfree;
	
	/*
	 * We have at least one request to the disk so start it up.
	 * Does not assume there is space for a request in the input queue.
	 */
	for(;;)	{					/* ever */
		/*
		 * All completed?
		 */
		if((bp=bufh->b_actf) == NULL)		/* No more to queue */
			break;

#ifdef SDDEBUG
	if(sd_debug>1) 
		printf("bp=0x%x,ifd=0x%x,bufh=0x%x,bp=0x%x\n",bufh->b_actf, ifd ,bufh, bp);
#endif SDDEBUG

		/*
		 * See if there is room on the devices input queue
		 * and allocate the number of iat's needed for this
		 * request.
		 */
		if(dc->dc_dfree<=2) {
			ASSERT(qbp->b_actf != NULL, "sd: deadlock, no room & no requests");
                        /*
                         *+ The sd driver is in a bad internal state.
                         *+ Although there are no outstanding requests,
                         *+ all of its device programs appear to be in use.
                         */

			ASSERT(dc->dc_dfree>=0, "sd: dfree < zero");
                        /*
                         *+ The sd driver is in a bad internal state;
                         *+ its count of free device programs
                         *+ is less than zero.
                         */

			/*
			 * Check to see if the device has plenty of devp's 
			 * in the queue and if the queue is full wait till 
			 * it drains down so that disksorting is even more 
			 * affective.
			 */
			ifd->sd_flags |= SDF_NOSTART;
#ifdef SDDEBUG
			if(sd_debug)
				printf("F");
#endif SDDEBUG
			break;
		}
		
		/* 
		 * Allocate the iat's and handle wrap around.
		 */
		numiat = sdiatsz(bp, dc);
		if(numiat==0) {
#ifdef SDDEBUG
			if(sd_debug)
				printf("i");
#endif SDDEBUG
			break;				/* Not enough for this one */
		}
		
		/*
		 * Mark the idle to non idle time stamp on all units.
		 */
		if(dc->dc_dfree != dc->dc_dsz)	 	/* all device programs free */
			ifd->sd_starttime = time;
			
		dc->dc_dfree--;				/* Allocate a device program */
#ifdef SDDEBUG
		if((dc->dc_dfree+1) == dc->dc_dsz)	
			ASSERT(qbp->b_actf == NULL, "sdstart: qbp Non NULL");
#endif SDDEBUG

		/*
		 * Seems there is room on the input queue, so do this request.
		 */
		diq = dc->dc_diq;
		iat = dc->dc_istart;
		head = diq->pq_head;
		devp = (diq->pq_un.pq_progs)[head];
#ifdef SDDEBUG
	if(sd_debug>2)
		printf("devp=0x%x diq=0x%x head=%d numiat=%d\n", devp, diq, head, numiat);
#endif SDDEBUG
		


		/*
		 * fill out the iat and the device program to be loaded
		 */
		if (!((bp->b_flags & B_IOCTL) && (bp->b_bcount == 0)))
			buf_iat(bp, iat, numiat);
		devp->dp_un.dp_iat = (struct sec_iat *)((int)iat | SEC_IAT_FLAG);
		devp->dp_data_len = roundup(bp->b_bcount, DEV_BSIZE);

		devp->dp_cmd_len = (bp->b_flags&B_IOCTL) ? 
				   SCSI_IOCTLLEN(bp) : SCSI_CMD6SZ;

		devp->dp_status1 = NULL;
		devp->dp_status2 = NULL;

		ASSERT(devp->dp_next==NULL,"sd: next_com ");
                /*
                 *+ The device program indicates that another command
                 *+ is linked.  This functionality is not currently
                 *+ supported by the driver.
                 */

		/*
		 * The SCSI command itself.
		 */
		if(bp->b_flags & B_IOCTL) {		/* SCSI command */
			bcopy((caddr_t)
			      (&((struct scsiioctl *)bp->b_sioctl)->sio_cmd),
			      (caddr_t)devp->dp_cmd,
			      (u_int)devp->dp_cmd_len);
#ifdef SDDEBUG
			if(sd_debug>2)
				printf("sdstart: dp_cmd = 0x%x\n", devp->dp_cmd[0]);
#endif /* SDDEBUG */
		}else{
			devp->dp_cmd[0]  =  (bp->b_flags&B_READ) ? SDC_READ : SDC_WRITE;	/* install cmd */
			devp->dp_cmd[1] = ifd->sd_lun | (u_char)((int)bp->b_resid >> 16);	/* msb blk number */
			devp->dp_cmd[2] = (u_char)((int)bp->b_resid >> 8);
			devp->dp_cmd[3] = (u_char)(bp->b_resid);				/* lsb blk number */
			devp->dp_cmd[4] = (u_char)howmany(bp->b_bcount,DEV_BSIZE);			/* number of blks */
			devp->dp_cmd[5] = 0;							/* status, always 0 */
		}
		
		/*
		 * Increment the device input queue.
		 */
		diq->pq_head = (head+1) % dc->dc_dsz;

		/* 
		 * dequeue this request from the main buffer queue
		 */
#ifdef SDDEBUG
	if(sd_debug>3)
		printf("qbp=0x%x, b_actf=0x%x, b_actl=0x%x, bp=0x%x, f=0x%x, l=0x%x\n",
			qbp, qbp->b_actf, qbp->b_actl, bp, bp->b_actf, bp->b_actl);
	else if(sd_debug)
		printf("s");
#endif SDDEBUG
		if(qbp->b_actf == NULL)
			qbp->b_actf = bp;
		else
			qbp->b_actl->b_actf = bp;
		qbp->b_actl = bp;
		bufh->b_actf = bp->b_actf;
		bp->b_actf = NULL;			/* Interrupt sanity */
#ifdef SDDEBUG
		if(bufh->b_actf == NULL)		/* XXX */
			ASSERT(qbp->b_actf != NULL, "sdstart:qbp NULL bufh NULL");
#endif SDDEBUG

		if(--numqueue <= 0)			/* Only take sd_thresh at a time, and at least one! */
			break;
	}
	
	/*
	 * To kick or not to kick, that is the question...
	 *
	 * Here we decide if there is a need to interrupt
	 * the board. Here send an interrupt on the first
	 * two queued request then none until it drains off.
	 */
	if(dstart+1 >=dc->dc_dsz) {

		ifd->sd_stat = 0;			/* zap return status */

		/*
		 * Start the request's out.
		 */
		sec_startio(	SINST_STARTIO, 		/* command */
				&ifd->sd_stat,		/* return status loc */
				ifd->sd_desc
			);

		if(ifd->sd_stat != SINST_INSDONE) {
			printf("sd: Device Status Error 0x%x SCED dev#=0x%x\n",
				ifd->sd_stat,	ifd->sd_desc->sd_chan
			);
                        /*
                         *+ The SCED returned a bad completion
                         *+ status on the specified command.
                         */

			panic("sd: SCED hard error status");
                        /*
                         *+ The operation being performed received a bad 
			 *+ completion status from the SCED.  This
                         *+ indicates a hard SCED error.
                         */
		}
	}
}

/*
 * sdiatsz(bp, dc) -	get the number of iats needed for this request
 *			and adjust the iat queue.
 * returns the number of iat's used.
 */
int
sdiatsz(bp, dc)
	register struct buf *bp;
	register struct seddc *dc;
{
	register int tail;
	register int needed;
	int	head;

	needed = buf_iatsz(bp);		/* How many needed for this request */
#ifdef SCSIFIRMWARE_SANITY
	needed++;
#endif SCSIFIRMWARE_SANITY
	/* 
	 * check for wraparound.
	 */
	head = dc->dc_ihead;
	tail = dc->dc_itail;

	/*
	 * If the head and tail pointers are equal then
	 * start over at the beginning to handle the largest
	 * size request generated by minphys. This allows
	 * the device to block waiting for iats to freeup
	 * because they aren't contiguous.
	 */
	if(head==tail) 
		head = tail = dc->dc_ihead = dc->dc_itail = 0;
#ifdef SDDEBUG
	if(head==tail)
		ASSERT(dc->dc_ifree==dc->dc_isz, "head says free");
#endif SDDEBUG
	

	/*
	 * See if there is enough room.
	 * Note: It is possible to have a case
	 * where the number of iat's goes off the
	 * end and even though there are enough iats
	 * for the request the iat's aren't contiguous,
	 * the second test handles this.
	 */
	if( dc->dc_ifree<=needed
	|| ((tail + needed > dc->dc_isz)
	&& (needed >= head))) {
#ifdef SDDEBUG
		if(sd_debug>1)
			printf("ifree %d<%d needed\n", dc->dc_ifree, needed);
#endif SDDEBUG
		return(0);		/* Not enough */
	}
	
	if(tail+needed > dc->dc_isz) {
		dc->dc_ifree -= (dc->dc_isz - tail + needed);
		tail = needed;		/* Wrap it around */
		dc->dc_istart = (struct sec_iat *)&(dc->dc_iat)[0];
	}else{
		tail = (dc->dc_itail+needed) % dc->dc_isz;
		dc->dc_istart = (struct sec_iat *)&(dc->dc_iat)[dc->dc_itail];/* tell the start */
		dc->dc_ifree -= needed;
	}
	dc->dc_itail = tail;			/* reflect the allocation */

#ifdef SCSIFIRMWARE_SANITY
	/*
	 * zero all iat's before filling them in to insure that the last one
	 * contains a count of zero for the firmware sanity to be enabled.
	 */
	bzero((char *) dc->dc_istart, (u_int) needed*sizeof(struct sec_iat));
#endif SCSIFIRMWARE_SANITY

#ifdef SDDEBUG
	if(sd_debug>1)
		printf("Need %d iat's starting at 0x%x\n", needed, dc->dc_istart);
	ASSERT(dc->dc_itail<dc->dc_isz, "sdiat: iat overflow");
#endif SDDEBUG

	return(needed);
}

/*
 * sdiatfree(bp, dc) -	free the number of iats needed for this request
 *			and adjust the iat queue.
 * Assumes that this is in sync with sdiatsz above for wrap around.
 */
sdiatfree(bp, dc)
	register struct buf *bp;
	register struct seddc *dc;
{
	register int free;
	register int head;

	free = buf_iatsz(bp);		/* How many needed for this request */
#ifdef SCSIFIRMWARE_SANITY
	free++;
#endif SCSIFIRMWARE_SANITY
	/* 
	 * check for wraparound.
	 */
	head = dc->dc_ihead;
	
	if(head+free > dc->dc_isz) {
		dc->dc_ifree += (dc->dc_isz - head + free);
		head = free;		/* Wrap it around */
	}else{
		head = (head+free) % dc->dc_isz;
		dc->dc_ifree += free;
	}
	dc->dc_ihead = head;			/* reflect the allocation */

#ifdef SDDEBUG
	if(sd_debug>1)
		printf("freed %d iat's\n", free);
	ASSERT(dc->dc_ihead<dc->dc_isz, "sdiatfree: iat underflow");
	ASSERT(dc->dc_ifree<=dc->dc_isz,"sdintr: ring overrun");
	ASSERT(dc->dc_ifree>=0,"sdintr: ring underflow");
#endif SDDEBUG
}

#undef SCSIFIRMWARE_SANITY

/*
 * sdintr(level) - interrupt routine.
 *
 * While there are outstanding requests that have completed
 * service and complete them.
 */

sdintr(level)
	int	level;
{
	register struct sd_info *ifd = sdifd[level-sd_baselevel];
	register struct seddc *dc;
	register struct	buf *bp;
	register struct sec_progq *doq;
	register struct sec_dev_prog *devp;
	struct	buf *qbp;
	struct	buf *bufh;
	int	tail;
	spl_t	x;

#ifdef SDDEBUG
	int	dfreexxx;		/* XXXX */

	ASSERT(level-sd_baselevel>=0, "sdintr: config bad, level negative check baselevel");
	ASSERT(level-sd_baselevel<sdmaxminor, "sdintr: config bad, vec maps no-device");
	if(sd_debug>2)
		printf("sdintr: ifd=0x%x, level %d\n",ifd, level-sd_baselevel);
	else if(sd_debug)
		printf("I");
#endif SDDEBUG


	ASSERT(ifd!=0,"sdintr: interrupt to non-configured device");
        /*
         *+ The sd driver received an interrupt from a
         *+ nonconfigured device.  This can indicate
         *+ that the system interrupt vector table has
         *+ been corrupted.
         */

	qbp = &ifd->sd_bp;
	dc = ifd->sd_dc;
	doq = dc->dc_doq;
	bufh = &ifd->sd_bufh;

	x = p_lock(&ifd->sd_lock, SDSPL);

#ifdef SDDEBUG
	for(dfreexxx=dc->dc_dfree, bp=qbp->b_actf; dfreexxx<dc->dc_dsz; bp=bp->b_actf,dfreexxx++)						/* XXXX */
		ASSERT(bp != NULL, "sdintr:Lost a bp");		/* XXXX */
	ASSERT(bp==NULL,"sdintr:Extra bps");			/* XXXX */
#endif SDDEBUG


	for(;;)	{		/* ever */
		
		if((bp = qbp->b_actf) == NULL)	/* current request */
			break;				/* all done... */

#ifdef SDDEBUG
		if ((sd_debug > 2) && (bp->b_flags & B_IOCTL))
			printf("<I>\n");
#endif /* SDDEBUG */

		if(doq->pq_tail == doq->pq_head)	/* All done */
			break;

		tail = doq->pq_tail;
		devp = (doq->pq_un.pq_progs)[tail];
		doq->pq_tail = (tail+1) % dc->dc_dsz;	/* tail ptr */
		/*
		 * Check for command completion
		 */
		if(devp->dp_status1 != SEC_ERR_NONE) {
			printf("sdintr: status1=%d\n", devp->dp_status1);
                        /*
                         *+ An error occurred on a SCSI command
                         *+ from the sd driver.
                         */

			/*
			 * Insert a rezero unit command iff it is known that
			 * this command will not hard error the device and the
			 * command is not a rezero command!
			 */
			if(ifd->sd_retrys > 0 && devp->dp_cmd[0] != SCSI_REZERO)
				sd_rezero_insert(dc->dc_diq, devp->dp_cmd[1]&SDLUNMSK, dc->dc_dsz);
			ifd->sd_savedp = devp;		/* save it to print it later */
			ifd->sd_flags |= SDF_SENSE;	/* DO request sense */
			devp->dp_status1 = 0;		/* remove error condition */
			/*
			 * fill in request sense dp and issue
			 * a request sense command.
			 *
			 * Todo: move next 9 lines to sdboot()
			 * because it's static (get it working first!)
			 */
			ifd->sd_statb.rs_dev_prog.dp_un.dp_data = (u_char *)ifd->sd_sensebufptr;
			ifd->sd_statb.rs_dev_prog.dp_next = 0;
			ifd->sd_statb.rs_dev_prog.dp_count = 0;
			ifd->sd_statb.rs_dev_prog.dp_data_len = sdsensebuf_sz;
			ifd->sd_statb.rs_dev_prog.dp_cmd_len = 6;
			ifd->sd_statb.rs_dev_prog.dp_status1 = 0;
			ifd->sd_statb.rs_dev_prog.dp_status2 = 0;
			ifd->sd_statb.rs_dev_prog.dp_cmd[0] = SDC_REQUEST_SENSE; 
			ifd->sd_statb.rs_dev_prog.dp_cmd[4] = (u_char)sdsensebuf_sz;
			ifd->sd_statb.rs_dev_prog.dp_cmd[1] = devp->dp_cmd[1] & SDLUNMSK; /* Masked lun in */
			ifd->sd_statb.rs_status = 0;

			sec_startio( SINST_REQUESTSENSE,
				(int	*)&ifd->sd_statb,
				ifd->sd_desc
			);
			v_lock(&ifd->sd_lock, x);
			return;
		}

		if(ifd->sd_flags & SDF_SENSE) {
			ifd->sd_flags &= ~SDF_SENSE;
#ifdef SDDEBUG
			if(sd_debug) {
				int	i;

				printf("Request sense bytes: ");
				for(i=0; i<sdsensebuf_sz; i++)
					printf("0x%x ", ifd->sd_sensebuf[i]);
				printf("\n");
				printf("Device prog: ");
				for(i=0; i<6; i++)
					printf("0x%x ", devp->dp_cmd[i]);
				printf("\n");
			}
#endif SDDEBUG
			devp->dp_status1 = (u_char)(((ifd->sd_sensebuf[0]&SDKEY) == SDKEY) ? ifd->sd_sensebuf[2]:ifd->sd_sensebuf[0]);

#ifdef notdef
			if (((ifd->sd_sensebuf[0] & SDKEY) == SDKEY) &&
			  ((ifd->sd_sensebuf[2] & SD_SENSEKEYMASK) == SD_RECOVERED)) {

				/*
				 * recovered error on CCS disks.  Means the
				 * drive discovered an error, but corrected it,
				 * and we need not try it again.
				 */
				printf("Soft error dev <0x%x,0x%x>, blk=0x%x, cnt=%d err=0x%x\n",
					major(bp->b_dev), minor(bp->b_dev),
					bp->b_blkno, bp->b_bcount,
					devp->dp_status1
				);
				printf("(Drive recovered)\n");
                                /*
                                 *+ An I/O error occurred on the sd device
                                 *+ whose major and minor numbers are shown.
                                 *+ The drive was able to correct the
                                 *+ error and recover valid data.
                                 */


				/*
				 * Print out all information about
				 * the error as possible.
				 */
				{
					int	i, j;
					
					i = sdsensebuf_sz;
					CPRINTF("LBA: %d\n",
						XgetLBA(ifd->sd_sensebuf));
					CPRINTF("Request sense bytes: ");
					for(j=0; j<i; j++)
						CPRINTF("0x%x ", ifd->sd_sensebuf[j]);
					CPRINTF("\n");
					/*
					 * print out the error'ing device program
					 */
					CPRINTF("Device prog: ");
					for(i=0; i<6; i++)
						CPRINTF("0x%x ", ifd->sd_savedp->dp_cmd[i]);
					CPRINTF("\n");
				}

				devp->dp_status1 = SEC_ERR_NONE; /* fake it */

				sec_startio( SINST_RESTARTIO,
					&ifd->sd_stat,
					ifd->sd_desc
				);
			} else
#endif notdef
			if(ifd->sd_retrys--) {		/* retry it */

				printf("Soft error dev <0x%x,0x%x>, blk=0x%x, cnt=%d err=0x%x\n",
					major(bp->b_dev), minor(bp->b_dev),
					bp->b_blkno, bp->b_bcount,
					devp->dp_status1
				);
                                /*
                                 *+ An I/O error occurred on the sd device
                                 *+ whose major and minor numbers are shown.
                                 *+ The command that resulted in the error
                                 *+ will be retried.
                                 */

				/*
				 * Print out all information about
				 * the error as possible.
				 */
				{
					int	i, j;
					
					if((ifd->sd_sensebuf[0]&SDKEY) == SDKEY){
						i = sdsensebuf_sz;
						CPRINTF("LBA: %d\n",
							XgetLBA(ifd->sd_sensebuf));
					}else{
						i = 4;
						CPRINTF("LBA: %d\n",
							getLBA(ifd->sd_sensebuf));
					}
					CPRINTF("Request sense bytes: ");
					for(j=0; j<i; j++)
						CPRINTF("0x%x ", ifd->sd_sensebuf[j]);
					CPRINTF("\n");
					/*
					 * print out the error'ing device program
					 */
					CPRINTF("Device prog: ");
					for(i=0; i<6; i++)
						CPRINTF("0x%x ", ifd->sd_savedp->dp_cmd[i]);
					CPRINTF("\n");
				}

				devp->dp_status1 = SEC_ERR_NONE; /* fake it */
				sec_startio( SINST_RESTARTCURRENTIO,
					&ifd->sd_stat,
					ifd->sd_desc
				);

				v_lock(&ifd->sd_lock, x);
				return;
			}else{
				printf("Hard error dev <0x%x,0x%x>, blk=0x%x, cnt=%d err=0x%x\n",
					major(bp->b_dev), minor(bp->b_dev),
					bp->b_blkno, bp->b_bcount,
					devp->dp_status1
				);
                                /*
                                 *+ On an sd device, either a hard
                                 *+ error occurred or a soft error
                                 *+ that could not be
                                 *+ corrected by retrying the command occurred
                                 *+ (retry limit exceeded).  
				 *+ Corrective action: consider
                                 *+ adding the offending block to the
                                 *+ defect list for the device.
                                 */

				/*
				 * Print out all information about
				 * the error as possible.
				 */
				{
					int	i, j;
					
					if((ifd->sd_sensebuf[0]&SDKEY) == SDKEY){
						i = sdsensebuf_sz;
						CPRINTF("LBA: %d\n",
							XgetLBA(ifd->sd_sensebuf));
					}else{
						i = 4;
						CPRINTF("LBA: %d\n",
							getLBA(ifd->sd_sensebuf));
					}
					CPRINTF("Request sense bytes: ");
					for(j=0; j<i; j++)
						CPRINTF("0x%x ", ifd->sd_sensebuf[j]);
					CPRINTF("\n");
					/*
					 * print out the error'ing device program
					 */
					CPRINTF("Device prog: ");
					for(i=0; i<6; i++)
						CPRINTF("0x%x ", ifd->sd_savedp->dp_cmd[i]);
					CPRINTF("\n");
				}

				devp->dp_status1 = SEC_ERR_NONE; /* fake it */

				/* 
				 * Uncorrectable error occured.
				 */
				bp->b_flags |= B_ERROR;
				sec_startio( SINST_RESTARTIO,
					&ifd->sd_stat,
					ifd->sd_desc
				);
			}
		}

		devp->dp_status1 = SEC_ERR_NONE;		/* Paranoid */

		/*
		 * Avoid looping forever on retries.
		 */
		if(devp->dp_cmd[0] == SCSI_REZERO)
			continue;

		ifd->sd_retrys = sdretrys;
		/*
		 * Adjust queues
		 */
		qbp->b_actf = bp->b_actf;
		bp->b_actf = 0;
		dc->dc_dfree++;

		/*
		 * Update stats for xfers and blks and
		 * mark do time stamp magic if all device programs
		 * are free. Note: sd_dk may not be a valid ptr on all units.
		 */
		if(ifd->sd_stat_unit != SD_NO_UNIT) {
			ifd->sd_dk->dk_xfer++;
			ifd->sd_dk->dk_blks += howmany(bp->b_bcount, DEV_BSIZE);
			
			if(dc->dc_dfree == dc->dc_dsz) {
				struct	timeval	elapsed;
				
				elapsed = time;
				timevalsub(&elapsed, &ifd->	sd_starttime);
				timevaladd(&ifd->sd_dk->dk_time, &elapsed);

				ASSERT(qbp->b_actf == NULL,"sdintr: free nonNULL qbp");
				/*
				 *+ The buffer on the devices active free list
				 *+ is not NULL.
				 */
			}
		}
		
		sdiatfree(bp, dc); 		/* free used iats */

		bp->b_resid = 0;
#ifdef SDDEBUG
		if ((sd_debug > 2) && (bp->b_flags & B_IOCTL))
			printf("<D>\n");
#endif /* SDDEBUG */

		iodone(bp);

	}	/* end forever loop */
	
	/*
	 * If non-startable check to see if its time to start again
	 * else Start any out standing requests
	 */
	if(ifd->sd_flags & SDF_NOSTART) {
		if((dc->dc_dsz - dc->dc_dfree) <= ifd->sd_low) {
			ifd->sd_flags &= ~SDF_NOSTART;
			sdstart(bufh, ifd);
		}
	}else{
		sdstart(bufh, ifd);
	}

	/*
	 * Device will remain busy until bufhq is sucked dry
	 */
	if(qbp->b_actf == NULL)
		bufh->b_active = SDS_IDLE;

	v_lock(&ifd->sd_lock, x);
}

/*
 * sdioctl
 */
/* ARGSUSED */
sdioctl(dev, cmd, addr)
	int	cmd;
	dev_t	dev;
	caddr_t	addr;
{
	struct sd_info *ifd;
	struct scsidev	*scsidev;
	struct vtoc *vtp;
	int	i, errno;

	ifd = sdifd[VUNIT(dev)];

	switch (cmd) {

	case RIOFIRSTSECT:
		*(int *)addr = 0;
		return(0);

	case RIODRIVER:		/* success indicates we're a driver! */
		return(0);

	case SIOCDEVDATA:
#ifdef SDDEBUG
		if (sd_debug>1)
			printf("sdioctl: SIOCDEVDATA\n");
#endif /* SDDEBUG */
		scsidev = (struct scsidev *)addr;
		scsidev->scsi_devno = (ifd->sd_desc->sd_target << 3)
				     | ifd->sd_desc->sd_unit;
		scsidev->scsi_ctlr = ifd->sd_desc->sd_sec_idx;
		return(0);

	case SIOCSCSICMD:
#ifdef SDDEBUG
		if (sd_debug>1)
			printf("sdioctl: SCSICMD\n");
#endif /* SDDEBUG */
		if (!suser())
			return(EACCES);
		/* SCSI commands */
		return(sd_ioctlcmd(dev, ifd, (struct scsiioctl *)addr));

	case V_READ:
		vtp = *(struct vtoc **)addr;
		if (!readdisklabel(dev, sdcstrat, ifd->sd_part, 
					 (struct cmptsize *)0, (daddr_t)0))
			return(EINVAL);
		return( copyout((caddr_t)ifd->sd_part, (caddr_t)vtp,
						sizeof(struct vtoc)));
	case V_WRITE:
		vtp = (struct vtoc *)wmemall(sizeof(struct vtoc), 1);
		if (copyin(*((caddr_t *)addr), (caddr_t)vtp, 
							  sizeof(struct vtoc))) {
			wmemfree((caddr_t)vtp, sizeof(struct vtoc));
			return(EFAULT);
		}
		if (!readdisklabel(dev, sdcstrat, ifd->sd_part, ifd->sd_compat,
					                       (daddr_t)0)) {
			wmemfree((caddr_t)vtp, sizeof(struct vtoc));
			return(EINVAL);
		}
		if (errno = setdisklabel(ifd->sd_part, vtp, ifd->sd_opens)) {
			wmemfree((caddr_t)vtp, sizeof(struct vtoc));
			return ((errno > 0) ? errno : EINVAL);
		}
		wmemfree((caddr_t)vtp, sizeof(struct vtoc));
		return(writedisklabel(dev, sdcstrat, ifd->sd_part, (daddr_t)0));

	case V_PART:
		if (!readdisklabel(dev, sdcstrat, ifd->sd_part, 
					 ifd->sd_compat, (daddr_t)0))
			return(EINVAL);
		/*
		 * convert any end of disk (SD_END) to the capacity
		 */
		vtp = ifd->sd_part;
		for (i = 0; i < vtp->v_nparts; i++) {
			if ((vtp->v_part[i].p_size == SD_END) ||
			    (vtp->v_part[i].p_start + vtp->v_part[i].p_size >
								ifd->sd_size)) {
				vtp->v_part[i].p_size = 
				  ifd->sd_size - vtp->v_part[i].p_start;
			}
			if (vtp->v_part[i].p_start > ifd->sd_size) {
				vtp->v_part[i].p_start = 0;
				vtp->v_part[i].p_size = 0;
			}
		}
		vtp = *(struct vtoc **)addr;
		return( copyout((caddr_t)ifd->sd_part, (caddr_t)vtp,
						sizeof(struct vtoc)));
	}
	return(ENXIO);				/* not supported yet */
}
	
static
sd_ioctlcmd(dev, ifd, sio)
	dev_t		dev;
	struct sd_info *ifd;
	struct scsiioctl *sio;
{
	struct buf *bp;			/* ioctl buffer */
	int dir = 0;			/* zero == READ */
	int error;
	spl_t s;

	bp = &ifd->sd_rbufh;

	/* make sure command is for the current device! */
	if (SCSI_UNIT(sio->sio_cmd6.cmd_lun >> SCSI_LUNSHFT) != 
		SCSI_UNIT(ifd->sd_lun >> SCSI_LUNSHFT)) {
#ifdef SDDEBUG
		if (sd_debug>1)
			printf("sd_ioctl: bad lun 0x%x - for 0x%x\n",
			  sio->sio_cmd6.cmd_lun, ifd->sd_lun);
#endif /* SDDEBUG */
		return(EINVAL);
	}
	if ((int)sio->sio_datalength < 0) {		/* invalid parm */
#ifdef SDDEBUG
		if (sd_debug>1)
			printf("sd_ioctl: negative data length\n");
#endif /* SDDEBUG */
		return(EINVAL);
	}

	switch(sio->sio_cmd6.cmd_opcode) {

	case SCSI_FORMAT:
#ifdef SDDEBUG
		if (sd_debug>1)
			printf("sd_ioctlcmd: SCSI_FORMAT\n");
#endif /* SDDEBUG */
		sio->sio_cmdlen = SCSI_CMD6SZ;
		s = p_lock(&ifd->sd_lock, SDSPL);
		ifd->sd_flags |= SDF_FORMATTED;
		v_lock(&ifd->sd_lock, s);
		break;

	case SCSI_INQUIRY:
#ifdef SDDEBUG
		if (sd_debug>1)
			printf("sd_ioctlcmd: SCSI_INQUIRY\n");
#endif /* SDDEBUG */
		sio->sio_cmdlen = SCSI_CMD6SZ;
		break;

	case SCSI_READC:
#ifdef SDDEBUG
		if (sd_debug>1)
			printf("sd_ioctlcmd: SCSI_READC\n");
#endif /* SDDEBUG */
		sio->sio_cmdlen = SCSI_CMD10SZ;
		break;

	case SCSI_REASS:
#ifdef SDDEBUG
		if (sd_debug>1)
			printf("sd_ioctlcmd: SCSI_REASS\n");
#endif /* SDDEBUG */
		sio->sio_cmdlen = SCSI_CMD6SZ;
		dir = 1;
		break;

	case SCSI_READ_DEFECTS:
#ifdef SDDEBUG
		if (sd_debug>1)
			printf("sd_ioctlcmd: SCSI_READ_DEFECTS\n");
#endif /* SDDEBUG */
		sio->sio_cmdlen = SCSI_CMD10SZ;
		break;

	default:
#ifdef SDDEBUG
		if (sd_debug>1)
			printf("sd_ioctlcmd: default - not supported yet\n");
#endif /* SDDEBUG */
		return(EINVAL);

	} /* end switch */

#ifdef SDDEBUG
	if (sd_debug>1)
		printf("addr = 0x%x, datalength = %d\n", sio->sio_addr,
			  sio->sio_datalength);
#endif /* SDDEBUG */
	/*
	 * Make sure user has access to specified data buffer
	 */
	if (sio->sio_datalength)
		if (!useracc((char *)sio->sio_addr,
			     (u_int)sio->sio_datalength,
			     ((dir) ? B_WRITE : B_READ))) {
#ifdef SDDEBUG
			if (sd_debug>1)
				printf("sd_ioctl: no user access\n");
#endif /* SDDEBUG */
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
#ifdef SDDEBUG
	if (sd_debug>1)
		printf("sd_ioctl: locking proc and pages\n");
#endif /* SDDEBUG */
	++u.u_procp->p_noswap;
	if (bp->b_bcount)
		vslock(bp->b_un.b_addr, (int)bp->b_bcount,
			(bool_t)(bp->b_flags & B_READ));

	/*
	 * Insert request at front of queue, set retry count to 0
	 */
	s = p_lock(&ifd->sd_lock, SDSPL);
#ifdef SDDEBUG
	if (sd_debug>1)
		printf("sd_ioctl: insert in queue\n");
#endif /* SDDEBUG */
	bp->b_actf = ifd->sd_bufh.b_actf;
	ifd->sd_bufh.b_actf = bp;
	ifd->sd_retrys = 0;
	if (ifd->sd_bufh.b_active == SDS_IDLE) {
#ifdef SDDEBUG
		if (sd_debug>1)
			printf("sd_ioctl: call sdstart\n");
#endif /* SDDEBUG */
		ifd->sd_bufh.b_active = SDS_BUSY;
		sdstart(&ifd->sd_bufh, ifd);
	}
	v_lock(&ifd->sd_lock, s);

#ifdef SDDEBUG
	if (sd_debug>1)
		printf("sd_ioctl: wait\n");
#endif /* SDDEBUG */
	biowait(bp);
	--u.u_procp->p_noswap;
	error = geterror(bp);
	buffree(bp);
#ifdef SDDEBUG
	if (sd_debug>1)
		printf("sd_ioctl: return %d\n", error);
#endif /* SDDEBUG */
	return(error);
}


/*
 * sdsize()
 *	Used for swap-space partition calculation.
 *
 * Doesn't assume that the device is open or alive at all
 * Hence there is more parameter checks than really needed.
 * Partitions can be designated as going out to the end of the
 * drive by specifying the length as SD_END.  When this is done,
 * the actual drive capacity is used.
 */
 
sdsize(dev)
	dev_t	dev;
{
	int	part, unit;
	register struct	partition *pt;
	register struct	sd_info	*ifd;

	unit = VUNIT(dev);
	part = VPART(dev);
	ifd = sdifd[unit];

	if(ifd == NULL				/* set in boot? */
	|| ifd->sd_part == NULL			/* table present? */
	|| unit >= sdmaxminor			/* Binary conf'd? */
	|| !ifd->sd_desc->sd_alive		/* Passed probing? */
	|| ifd->sd_size == 0)
		return(-1);

	pt = &(ifd->sd_part->v_part)[part];

	if (pt->p_size == 0) 			/* valid partition? */
		return(-1);

	return(pt->p_size);
}

/*
 * Rezero insert 
 *
 * Insert a device program into the device input queue
 * ahead of the current request to recalibrate the device
 * (rezero unit) before a retry operation. The insertion
 * must be done before the request sense command is issued
 * to the device so that when the device restarts the 
 * recalibration is automatic and requires no further fuss.
 *
 * Note: Because of this 'recommended' condition the device
 * input queue may never be allowed to get empty below two
 * open positions, 1 for a rezero and one for wrap testing
 * for the firmware.
 */

sd_rezero_insert(diq, lun, size)
	register	struct sec_progq *diq;
	u_char		lun;
	int		size;
{
	register	struct sec_dev_prog *idevp;
	register	tail;

	tail = diq->pq_tail;
	if(tail==0)		/* handle wrap around */
		tail = size-1;
	else 
		tail--;
	/*
	 * get previous now unused device program pointer and use
	 * it for the rezero command that is being inserted.
	 */
	idevp = (diq->pq_un.pq_progs)[tail];
	idevp->dp_cmd[0] = SCSI_REZERO;
	idevp->dp_cmd[1] = lun;
	bzero((char *)&idevp->dp_cmd[2], 4);
	idevp->dp_un.dp_iat = 0;
	idevp->dp_data_len = 0;
	idevp->dp_cmd_len = 6;
	idevp->dp_status1 = NULL;
	idevp->dp_status2 = NULL;

	/*
	 * adjust diq ptrs to reflect new device program inserted.
	 */
	diq->pq_tail = tail;
}

daddr_t
sdreadc(sed)
	struct sec_dev *sed;
{
	struct sdcap *sdcap;
	u_long	nblocks = 0;		/* number of blocks on disk */
	daddr_t	capacity;		/* usable disk capacity */

	if (sd_docmd(SDC_READ_CAPACITY, sed) != 0)
		return((daddr_t) -1);

	sdcap = (struct sdcap *)sd_datap;

	/*
	 * Reverse the byte order
	 */

	nblocks = (sdcap->sdc_nblocks0 << 24) |
		  (sdcap->sdc_nblocks1 << 16) |
		  (sdcap->sdc_nblocks2 << 8)  |
		  (sdcap->sdc_nblocks3) ;

	/*
	 * decide which kind of disk we have based on which range of
	 * capacities the size falls into.  This allows us to reserve the
	 * right ammount for the diag tracks.
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

#ifdef SDDEBUG
	if (sd_debug > 3)
		printf("sdreadc: size=%d, capacity = %d\n", nblocks, capacity);
#endif SDDEBUG

	return(capacity);
}


/*
 * Do an sd_docmd from sdprobe().  There is not a sec_dev structure availble
 * for the device at this point, so we must transmogrify the sec_probe
 * struct into a sec_dev struct for sd_docmd().
 */

sd_docmd_pr(cmd,sed)
	u_char cmd;
	struct sec_probe *sed;
{
	struct sec_dev sdev;

	sdev.sd_desc	= sed->secp_desc;
	sdev.sd_target	= sed->secp_target;
	sdev.sd_unit	= sed->secp_unit;
	sdev.sd_chan	= sed->secp_chan;
	sdev.sd_flags	= sed->secp_flags;
	sdev.sd_bin	= SD_ANYBIN;

	return(sd_docmd(cmd, &sdev));
}

/*
 * Do a general SCSI command on the target adaptor.
 */

sd_docmd(cmd, sed)
	u_char cmd;
	struct sec_dev *sed;
{
	register struct	sec_powerup *iq = sed->sd_desc->sec_powerup;
	register int	i;
	u_char		data_length;
	u_char		cmd_length;
	int		stat = 0;

	ASSERT_DEBUG(((int)&stat < 0x400000),"sd_docmd: configuration problem - address of \"stat\" greater then 0x400000");

#ifdef SDDEBUG
	if(sd_debug>3) {
		printf("sd_docmd: init_q 0x%x, slicid 0x%x stat=0x%x,",
			(int)iq, sed->sd_desc->sec_slicaddr, &stat);

		printf(" device=%d\n", sed->sd_chan);
	}
	printf("sd_docmd: cmd=0x%x\n", cmd);
#endif SDDEBUG 

	bzero((caddr_t)&sd_devprog, sizeof(struct sec_dev_prog));

	switch (cmd) {

	case SDC_READ_CAPACITY:
		data_length = SDD_READC;
		cmd_length  = SD_CMD10SZ;
		break;

	case SDC_TEST:
		data_length = SDD_TEST;
		cmd_length  = SD_CMD6SZ;
		break;

	case SDC_REQUEST_SENSE:
		data_length = SDD_REQSEN;
		cmd_length  = SD_CMD6SZ;
		break;

	case SDC_MODE_SENSE:
		sd_devprog.dp_cmd[2] = SDM_ERROR;
		data_length = SDD_MODE;
		cmd_length  = SD_CMD6SZ;
		break;

	case SDC_MODE_SELECT:
		sd_devprog.dp_cmd[1] = SDM_PF;
		data_length = SDD_MODE;
		cmd_length  = SD_CMD6SZ;
		break;

	case SDC_INQUIRY:
		data_length = SDD_INQ;
		cmd_length  = SD_CMD6SZ;
		break;
	}

	/*
	 * Fill out the device program for a single command.
	 */

	sd_devprog.dp_un.dp_data = sd_data_queue;
	sd_devprog.dp_data_len = data_length;
	sd_devprog.dp_cmd_len = cmd_length;

	/*
	 * fill out the CDB - command goes in byte 0 and the logical unit
	 * number in byte 1
	 */

	sd_devprog.dp_cmd[0] = cmd;
	sd_devprog.dp_cmd[1] |= (unsigned char)(sed->sd_unit << SCSI_LUNSHFT);

	/*
	 * Some commands take an allocation length in byte 4 of the CDB.
	 */

	switch (cmd) {
	case SDC_REQUEST_SENSE:
	case SDC_MODE_SELECT:
	case SDC_MODE_SENSE:
	case SDC_INQUIRY:
		sd_devprog.dp_cmd[4] = data_length;
	}

	/*
	 * Insert device program into SCED queue and mark I/O in progress.
	 */

	i = iq->pu_requestq.pq_head;
	iq->pu_requestq.pq_un.pq_progs[i] = &sd_devprog;
	iq->pu_requestq.pq_head = (i+1) % SEC_POWERUP_QUEUE_SIZE;

	SEC_startio(SINST_STARTIO, 			/* command */
		    &stat,				/* return status loc */
		    sed->sd_bin, 			/* bin number on SEC */
		    sed->sd_chan,			/* Disk channel # */
		    &iq->pu_cib, 			/* device input q loc */
		    (u_char)sed->sd_desc->sec_slicaddr	/* SEC slic id number */
		);

	/*
	 * SCSI command in progress
	 */

	if(stat == SINST_INSDONE) {
		
		/*
		 * spin, waiting for command to complete
		 */

		while(iq->pu_doneq.pq_tail == iq->pu_doneq.pq_head)
			;

		/*
		 * Inform the SCED that the I/O is done by updating the
		 * tail of the device queue.
		 */

		iq->pu_doneq.pq_tail = 
			(iq->pu_doneq.pq_tail + 1) % SEC_POWERUP_QUEUE_SIZE;

	} else {

		iq->pu_requestq.pq_tail = iq->pu_requestq.pq_head = NULL;
		printf("sd: probe issued a totally bad command (params)\n");
		printf("sd: stat = 0x%x\n", stat);
                /*
                 *+ An internal sd driver error occurred.  The
                 *+ sd driver might be in an inconsistent state.
                 */
		return(stat);
	}

	if(sd_devprog.dp_status1 != SEC_ERR_NONE) {
		int cmdret;

#ifdef SDDEBUG
		printf("sd: err status=0x%x\n", sd_devprog.dp_status1);
#endif SDDEBUG
		cmdret = sd_devprog.dp_status1 ;
		sd_devprog.dp_status1 = 0;

		/*
		 * After an error, we need to tell the SCED to keep going
		 * with a RESTARTIO.
		 */

		SEC_startio(SINST_RESTARTIO, &stat, sed->sd_bin, sed->sd_chan,
			&iq->pu_cib, (u_char)sed->sd_desc->sec_slicaddr);

		return(cmdret);
	}
	return(0);
}

/*
 * sdcstrat - SCSI strategy routine.
 * This version does not check the limits on the disk.
 */

sdcstrat(bp)
	register struct	buf	*bp;
{
	register struct	sd_info	*ifd;
	register struct	buf *bufh;
	int	x;

	
	ifd = sdifd[VUNIT(bp->b_dev)];
	bufh = &ifd->sd_bufh;

	if ((bp->b_bcount <= 0)
	||  ((bp->b_iotype == B_RAWIO) &&
		(((int)bp->b_un.b_addr & (SD_ADDRALIGN - 1)) != 0))) {
		bp->b_resid = 0;		/* must be 0 when done! */
		bp->b_flags |= B_ERROR;
		bp->b_error = EINVAL;
		iodone(bp);
		return;
	}

	bp->b_resid = bp->b_blkno;	/* for disksort */

	/*
	 * Note: To this point in the procedure only static data
	 * has been accessed.
	 */
	 
	x = p_lock(&ifd->sd_lock, SDSPL);
	disksort(bufh, bp);
	if(bufh->b_active == SDS_IDLE) {
		bufh->b_active = SDS_BUSY;
		sdstart(bufh, ifd);
	}
	v_lock(&ifd->sd_lock, x);
}

