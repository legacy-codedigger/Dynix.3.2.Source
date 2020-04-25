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

#ifdef RCS
static char rcsid[]= "$Header: zd.c 1.26 90/07/23 $";
#endif RCS

/*
 * zd.c
 *	Stand-alone ZDC disk driver.
 */

/* $Log:	zd.c,v $
 */

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/vtoc.h>
#include <machine/cfg.h>
#include <machine/slicreg.h>
#include <zdc/zdc.h>
#include <zdc/zdbad.h>
#include "zdc.h"
#include "saio.h"

#define	i_debug	i_cyloff

/* Is x NOT aligned on y boundary? - y must be power of 2 */
#define	NOTALIGNED2(x, y)	(((x) & ((y) - 1)) != 0)

/* Is x NOT a multiple of y?  - y must be power of 2 */
#define	NOTMULT2(x, y)		NOTALIGNED2((x), (y))

#ifndef VTOC
extern	short	cyl_offsets[][NUMPARTS];	/* offsets for partitions */
#endif
extern	int	numzdcs;			/* number of zdcs */
extern	int	nzdtypes;			/* number of disk types */
extern	int	zdcinitime;			/* time for FW to init */
extern	int	zdccmdtime;			/* command completion timeout */
extern	int	zdcretry;			/* command retry count */
extern	u_char	zdctrl;				/* more ZDC_INIT ctrl bits */
extern	int	cpuspeed;			/* relative CPU performance */
caddr_t	calloc();
#ifndef	BOOTXX
static	long	getchksum();
#endif	BOOTXX

/*
 * 1 CB array will be dynamically allocated.
 * Since only 1 I/O request can be active at any one time,
 * 1 CB array can be shared by all controllers.
 */
static	struct	cb	*zdcb;
static	struct	zdc_ctlr zdctrlr[ZDC_MAXCTRLR];		/* 1 per Controller */
static	struct	zdcdd *chancfg;				/* ZDC_GET_CHANCFG */
static	struct	zdbad *zdbsd[ZDC_MAXCTRLR * ZDC_MAXDRIVES];/* bad sector data */

#ifndef	BOOTXX
static	caddr_t	zd_compcodes[] = {
	"Command in progress",
	"Successful completion",
	"Write protect fault",
	"Drive Fault",
	"Seek error",
	"Seek timeout",
	"Channel timeout",
	"DMA timeout",
	"Header ECC error",
	"Soft ECC error",
	"Correctable ECC error",
	"Uncorrectable ECC error",
	"Sector not found",
	"Bad data sector",
	"Sector overrun",
	"No data synch",
	"Fifo data lost",
	"Illegal cb_cmd",
	"Illegal cb_mod",
	"Illegal disk address",
	"cb_addr not 16-byte aligned",
	"Illegal cb_count",
	"cb_iovec not 32-byte aligned",
	"Non-zero cb_iovec and page size invalid",
	"Illegal icb_pagesize",
	"icb_dumpaddr not 16-byte aligned",
	"Bad drive",
	"In-use CB reused",
	"Access error during DMA",
	"Channel not configured",
	"Channel was reset",
	"Unexpected status from DDC",
	"Unknown Completion code"
};
#define	NCOMPCODES	(sizeof(zd_compcodes) / sizeof(zd_compcodes[0]))
#endif	BOOTXX

/*
 * zdopen
 *
 * open a device for read or write.
 *	- set i_boff based on partition table for disk drive type.
 *	- read in bad block list if not already present.
 *
 * On first open for any device, zdopen:
 *	- allocates the CB array used to communicate with any drive/controller.
 *	- allocates memory for channel configuration data DMAs.
 *	- Initializes the controller structures for found ZDCs.
 */
zdopen(io)
	register struct iob *io;
{
	register struct zdc_ctlr *ctlrp;
	register int part;			/* drive partition */
	register struct zdcdd *dd;		/* disk description */
	int ctlrunit;				/* index into zdbsd */
	u_char state;				/* drive state */
	static	u_char firstopen = 1;		/* 1st open flag */

	if (firstopen) {
		/*
		 * Allocate memory for CB array and channel configuration.
		 * Initialize controller structures.
		 */
		callocrnd(sizeof(struct cb));
		zdcb = (struct cb *)calloc(NCBPERZDC * sizeof(struct cb));
		callocrnd(sizeof(struct zdcdd));
		chancfg = (struct zdcdd *)calloc(sizeof(struct zdcdd));
		findzdcs();
		firstopen = 0;
	}

	ctlrp = &zdctrlr[ZDC_CTRLR(io->i_unit)];
	/*
	 * If controller is not found or dead, return error.
	 */
	if (ctlrp->zdc_state != ZDC_ALIVE) {
		io->i_error = EUNIT;
		return;
	}

	/*
	 * Initialize the requested controller
	 */
	io->i_cc = 0;			/* no disk I/O */
	init_zdc(io);
	if (io->i_error)
		return;

	state = ctlrp->zdc_drivecfg[ZDC_DRIVE(io->i_unit)];
	/*
	 * Drive is usable if found and online.
	 */
	if (state == ZD_NOTFOUND || (state & ZD_ONLINE) != ZD_ONLINE) {
		io->i_error = EUNIT;
		return;
	}

	/*
	 * If the drive is not formatted or is formatted differently
	 * than the other drives on the channel, then there is no
	 * partition info to look up. In this case only format operations
	 * will be allowed. That is, read and write will return error.
	 */
	if ((state & (ZD_FORMATTED | ZD_MATCH)) != (ZD_FORMATTED | ZD_MATCH))
		return;

	dd = (ZDC_DRIVE(io->i_unit) & 1) ? &ctlrp->zdc_chanB
					 : &ctlrp->zdc_chanA;
#ifndef	BOOTXX
	if (dd->zdd_drive_type >= nzdtypes) {
		/*
		 * Unknown type
		 */
		io->i_error =  ENXIO;
		return;
	}
#endif	/* BOOTXX */

	/*
	 * Read bad block table - if haven't already done so.
	 */
	ctlrunit = ZDC_CTRLR(io->i_unit) * ZDC_MAXDRIVES + ZDC_DRIVE(io->i_unit);
	if (zdbsd[ctlrunit] == NULL) {
		struct iob tio;
		register int block;		/* index into bad block list */
		int track;			/* track number */
		int size;			/* blocks in bad block list */
		int gotit;			/* Read successfully */
#ifndef	BOOTXX
		struct zdbad *zdp;		/* pointer to bad block list */
#endif	BOOTXX

		/*
		 * First, try track 0. If that fails,
		 * then try successive tracks.
		 */
		tio = *io;
		tio.i_cc = DEV_BSIZE;
		tio.i_flgs |= F_RDDATA | F_NBSF;

		/*
		 * Read bad block list.
		 */
		callocrnd(DEV_BSIZE);
		size = (dd->zdd_sectors - ZDD_NDDSECTORS) >> 1;
		zdbsd[ctlrunit] = (struct zdbad *)calloc(size << DEV_BSHIFT);
#ifndef	BOOTXX
		zdp = zdbsd[ctlrunit];
#endif	BOOTXX

		tio.i_ma = (char *)zdbsd[ctlrunit];
		for (block = 0; block < size; block++) {
			gotit = 0;
			track = 0;
			tio.i_bn = ZDD_NDDSECTORS + block;
			while (track < MIN(dd->zdd_tracks, BZ_NBADCOPY)) {
				if (zdstrategy(&tio, READ) == DEV_BSIZE) {
					gotit = 1;
					break;
				}
				track++;
				tio.i_bn = track * dd->zdd_sectors + block;
			}

			/*
			 * If cannot read the block of bad block list,
			 * mark drive as unformatted and return.
			 */
			if (!gotit) {
#ifndef	BOOTXX
				printf("zd%d: Cannot read block %d of bad block list.\n",
					io->i_unit, block);
				zdbsd[ctlrunit] = NULL;
				ctlrp->zdc_drivecfg[ZDC_DRIVE(io->i_unit)] &= ~ZD_FORMATTED;
#else	BOOTXX
				io->i_error = tio.i_error;
#endif	BOOTXX
				return;
			}
#ifndef	BOOTXX
			if (block == 0) {
				int nsize;	/* blocks to read in bbl */

				nsize = (zdp->bz_nelem * sizeof(struct bz_bad))
					+ sizeof(struct zdbad)
					- sizeof(struct bz_bad);
				nsize = howmany(nsize, DEV_BSIZE);
				if (nsize > size) {
					/*
					 * Size of bad block list unreasonable.
					 * If bad block list corrupted consider
					 * format to be invalid. Allow open
					 * but disallow read/write.
					 */
					printf("zd%d: Bad block list corrupted!\n",
						io->i_unit);
					zdbsd[ctlrunit] = NULL;
					ctlrp->zdc_drivecfg[ZDC_DRIVE(io->i_unit)] &= ~ZD_FORMATTED;
					return;
				}
				size = nsize;
			}
#endif	BOOTXX
			tio.i_ma = (char *)((u_int)tio.i_ma + DEV_BSIZE);
		} /* end of for */ 

#ifndef	BOOTXX
		/*
		 * Confirm data integrity via checksum.
		 */
		size = (zdp->bz_nelem * sizeof(struct bz_bad)) / sizeof(long);
		if (zdp->bz_csn != getchksum((long *)zdp->bz_bad, size,
					(long)(zdp->bz_nelem ^ zdp->bz_nsnf))) {
			printf("zd%d: Checksum failed!\n", io->i_unit);
			/*
			 * If bad block list corrupted consider format
			 * to be invalid. Allow open but disallow read/write.
			 */
			zdbsd[ctlrunit] = NULL;
			ctlrp->zdc_drivecfg[ZDC_DRIVE(io->i_unit)] &= ~ZD_FORMATTED;
			return;
		}
#endif	BOOTXX
	}

	part = io->i_boff;
	/*
	 * Read the partition data.  io->i_boff is the partition number
	 * on entry.  The VTOC gets read, and io->i_boff gets replaced
	 * with the offset of this partition from the front of the disk.
	 */
#ifndef BOOTXX
	if (vtoc_setboff(io, 1 * dd->zdd_sectors * dd->zdd_tracks) < 0) {
		io->i_error = 0;
		if (part < NUMPARTS) {
			if (dd->zdd_drive_type >= nzdtypes) {
				/*
				 * Unknown type
				 */
				io->i_error =  ENXIO;
				return;
			}
			if (cyl_offsets[dd->zdd_drive_type][part] == -1) {
				/*
				 * Bad partition
				 */
				io->i_error = EUNIT;
				return;
			}
			io->i_boff = cyl_offsets[dd->zdd_drive_type][part]
					* dd->zdd_sectors * dd->zdd_tracks;
		}
	}
#else /* BOOTXX */
#ifdef VTOC
	(void) vtoc_setboff(io, 1 * dd->zdd_sectors * dd->zdd_tracks);
#else
	if (part < NUMPARTS) {
		if (dd->zdd_drive_type >= nzdtypes) {
			/*
			 * Unknown type
			 */
			io->i_error =  ENXIO;
			return;
		}
		if (cyl_offsets[dd->zdd_drive_type][part] == -1) {
			/*
			 * Bad partition
			 */
			io->i_error = EUNIT;
			return;
		}
		io->i_boff = cyl_offsets[dd->zdd_drive_type][part]
				* dd->zdd_sectors * dd->zdd_tracks;
	}
#endif /* VTOC */
#endif /* BOOTXX */
}

#ifndef	BOOTXX
/*
 * getchksum
 *	Calculate bad block list checksum
 */
static long
getchksum(lptr, nelem, seed)
	register long *lptr;
	register int nelem;
	long seed;
{
	register long sum;

	sum = seed;
	while (nelem-- > 0) {
		sum ^= *lptr;
		++lptr; 
	}
	return (sum);
}
#endif	BOOTXX

/*
 * zdstrategy - read / write interface
 */
zdstrategy(io, func)
	register struct iob *io;
	int func;
{
	register struct cb *cbp;
	register int sector;
	register struct zdcdd *dd;
	register struct zdc_ctlr *ctlrp;
	int nspc;				/* sectors per cylinder */
	struct cb rwcb;

	cbp = &rwcb;
	ctlrp = &zdctrlr[ZDC_CTRLR(io->i_unit)];
#ifndef	BOOTXX
	/*
	 * Error if not formatted or format doesn't match the
	 * rest of the channel.
	 */
	if ((ctlrp->zdc_drivecfg[ZDC_DRIVE(io->i_unit)] &
	     (ZD_FORMATTED | ZD_MATCH)) != (ZD_FORMATTED | ZD_MATCH)) {
		io->i_error = EIO;
		return (-1);
	}
	if (NOTALIGNED2((int)io->i_ma, ADDRALIGN)) {
		io->i_error = EIO;
		return (-1);
	}
#endif	BOOTXX
	dd = (ZDC_DRIVE(io->i_unit) & 1) ? &ctlrp->zdc_chanB
					 : &ctlrp->zdc_chanA;
	nspc = dd->zdd_sectors * dd->zdd_tracks;
	sector = io->i_bn;
	cbp->cb_cyl = sector / nspc;
	sector %= nspc;
	cbp->cb_head = sector / dd->zdd_sectors;
	cbp->cb_sect = sector % dd->zdd_sectors;
	cbp->cb_cmd = (func == READ) ? ZDC_READ : ZDC_WRITE;
	cbp->cb_mod = 0;
	/*
	 * Roundup in case applications ask for non DEV_BSIZE transfers.
	 */
	cbp->cb_count = (io->i_cc + (DEV_BSIZE-1)) & ~(DEV_BSIZE-1);
	cbp->cb_addr = (u_long)io->i_ma;
	cbp->cb_iovec = NULL;
	return (dozdcmd(io, cbp));
}

/*
 * dozdcmd
 *
 * Issue a command to the ZDC. 
 * Poll for completion.
 * Retry, if necessary.
 *
 * Called from zdstrategy(), zdioctl(), and init_zdc().
 *
 * Return transfer count
 * On error:
 *	- io->i_error will contain error code.
 *	- io->i_errblk will contain logical block # where error occurred.
 * If error, return -1.
 * If bytes transferred, return count transferred.
 */
dozdcmd(io, acbp)
	register struct iob *io;
	register struct cb *acbp;
{
	register struct cb *cbp;
	register int i;
	struct zdc_ctlr *ctlrp;
	int	saverrno;		/* saved errno before last reset */
	struct	cb lcb;			/* for ZDC_RESET during retries */

	ctlrp = &zdctrlr[ZDC_CTRLR(io->i_unit)];
	cbp = &zdcb[ZDC_DRIVE(io->i_unit) * NCBPERDRIVE];

	if (acbp->cb_cmd != ZDC_RESET)
		io->i_errcnt = 0;
	io->i_error = 0;

retry:
	/*
	 * Fill in appropriate CB
	 */
	*cbp = *acbp;
#ifndef	BOOTXX
	/*
	 * If severe burn-in, inhibit ZDC ECC correction.
	 */
	if (io->i_flgs & F_SEVRE)
		cbp->cb_mod |= ZDC_NOECC;
#endif	BOOTXX

restart:
	/*
	 * Signal ZDC to do command. Poll until completion or timeout.
	 */
	cbp->cb_psect = 0;		/* always 0 in stand-alone */
	cbp->cb_compcode = ZDC_BUSY;
#ifdef	DEBUG
#ifndef	BOOTXX
	if (io->i_debug & ZD_DUMPCBDEBUG)
		dumpcb(cbp);		/* dump cb contents */
#endif	BOOTXX
#endif	DEBUG
	if (mIntr(ctlrp->zdc_slicaddr, CBBIN, (int)(cbp - zdcb))) {
#ifndef	BOOTXX
		printf("zd%d: Cannot mIntr.\n", io->i_unit);
#endif	BOOTXX
		io->i_error = EHER;
		return (-1);
	}

	i = zdccmdtime * cpuspeed;
	while (cbp->cb_compcode == ZDC_BUSY) {
		if (--i == 0) {
			/*
			 * Timed out - check for controller error
			 */
#ifndef	BOOTXX
			printf("zd%d: Cmd %x timeout.\n", io->i_unit,
				cbp->cb_cmd);
			i = rdslave(ctlrp->zdc_slicaddr, SL_Z_STATUS);
			if (i >= 0 && ((i & SLB_ZPARERR) ||
					((i & ZDC_READY) != ZDC_READY))) {
				/*
				 * Found controller error.
				 */
				if (((i & ZDC_ERRMASK) == ZDC_OBCB) &&
						cbp->cb_cmd == ZDC_INIT) {
					/*
					 * If Out-of-Bounds CB and the command
					 * was a ZDC_INIT, then return
					 * an error. ZDC_INIT is the first
					 * ZDC command attempted on open so
					 * this error is probably a reference to
					 * a drive that cannot exist on OK-8.
					 * That is, drives 8-15.
					 */
					if (mIntr(ctlrp->zdc_slicaddr,
							CLRERRBIN, 0xbb)) {
						printf("zd%d: Cannot mIntr.\n",
							io->i_unit);
						io->i_error = EHER;
					} else
						io->i_error = EUNIT;
					return (-1);
				}
				printf("zd%d: Ctrlr error status 0x%x.\n",
					io->i_unit, i);
				if (zdctrl & ZDC_DUMPONPANIC)
					printf("zd%d: FW dump address = 0x%x.\n",
						io->i_unit,
						ctlrp->zdc_dumpaddr);
				/*
				 * controller bad - panic!!
				 */
				_stop("zd: bad ctlr");
			}
#endif	BOOTXX
			io->i_error = EHER;
			return (-1);
		}
	}
#ifdef	DEBUG
#ifndef	BOOTXX
	if (io->i_debug & ZD_DUMPCBDEBUG) {
		printf("Command Completed...\n");
		dumpcb(cbp);
	}
#endif	BOOTXX
#endif	DEBUG

	switch (cbp->cb_compcode) {

	case ZDC_SOFTECC:
	case ZDC_CORRECC:
		/*
		 * Corrected or Soft ECC error.
		 */
#ifndef	BOOTXX
		printf("zd%d: %s at (%d, %d, %d).\n", io->i_unit,
			zd_compcodes[cbp->cb_compcode],
			cbp->cb_cyl, cbp->cb_head, cbp->cb_sect);
		dumpstatus(cbp);
#endif	BOOTXX
		/* Fall into... */
	case ZDC_DONE:
		/*
		 * Normal completion
		 */
done:
		*acbp = *cbp;
		return (io->i_cc);

#ifndef	BOOTXX
	case ZDC_ECC:
		/*
		 * Hard ECC error
		 */
		io->i_error = EECC;
		goto hard;

	case ZDC_DRVPROT:
	case ZDC_DMA_TO:
	case ZDC_REVECT:
	case ZDC_ILLCMD:
	case ZDC_ILLMOD:
	case ZDC_ILLALIGN:
	case ZDC_ILLCNT:
	case ZDC_ILLIOV:
	case ZDC_ILLVECIO:
	case ZDC_ILLPGSZ:
	case ZDC_ILLDUMPADR:
	case ZDC_CH_RESET:
	case ZDC_DDC_STAT:
		/*
		 * Hard error.
		 */
		io->i_error = EHER;
		goto hard;

	case ZDC_ILLCHS:
		/*
		 * Bad input - access at past end of disk most likely.
		 */
		io->i_error = EIO;
		goto hard;

	case ZDC_BADDRV:
		/*
		 * Drive went away. ZDC_BADDRV is issued until it reappears.
		 * Stand-alone assumes that it reappears with the same
		 * characteristics as before. To see any changes a
		 * close/re-open must be done to effect re-probe.
		 * Formatter should not see as it does a close/re-open
		 * to probe drive status.
		 */
		io->i_error = EIO;
		goto hard;
#endif	BOOTXX

	case ZDC_CBREUSE:
		/*
		 * ZDC/driver in unknown state.
		 */
		_stop("CB re-used");

#ifndef	BOOTXX
	case ZDC_ACCERR:
		/*
		 * Most likely bad memory address input.
		 * Clear access error and notify firmware.
		 */
		io->i_error = EIO;
		i = rdslave(ctlrp->zdc_slicaddr,
			    (ZDC_DRIVE(io->i_unit) & 1) ? SL_G_ACCERR1
							: SL_G_ACCERR);
		printf("zd%d: Access error 0x%x on transfer at 0x%x.\n",
			io->i_unit, i, cbp->cb_addr);
		wrslave(ctlrp->zdc_slicaddr,
			(ZDC_DRIVE(io->i_unit) &1) ? SL_G_ACCERR1 : SL_G_ACCERR,
			(u_char)0xbb);
		goto hard;
#endif	BOOTXX

	case ZDC_NOCFG:
		*acbp = *cbp;
		if (cbp->cb_cmd == ZDC_GET_CHANCFG)
			return (0);
#ifndef BOOTXX
                if (cbp->cb_cmd == ZDC_READ_HDRS) {
                        io->i_error = EHER;
                        goto hard;
                }
#endif /* BOOTXX */
		return (-1);

	case ZDC_SNF:
#ifndef BOOTXX
                /*
                 * Short circuit ZDC_READ_HDRS failure
                 * so revector/retry is not attempted.
                 */
                if (cbp->cb_cmd == ZDC_READ_HDRS) {
                        io->i_error = EBSE;
                        goto hard;
                }
#endif /* BOOTXX */
		i = 0;
		if ((io->i_flgs & F_NBSF) == 0 && (i = zdrevector(io, cbp)) == 0) {
			/*
			 * Reset errcnt for retries.
			 * Update initial cb for second part (beyond revectored
			 * sector). This effectively breaks up initial
			 * request into multiple requests.
			 */
			io->i_errcnt = 0;
			if (cbp->cb_count == 0)
				goto done;
			*acbp = *cbp;
			goto restart;
		}
		/*
		 * If I/O on replacement sector failed, hard error.
		 * Otherwise, retry - since sector was not in bad block list.
		 */
		if (i < 0)
			goto hard;
		io->i_error = EBSE;
#ifndef	BOOTXX
		/*
		 * If SEVRE burn-in, no retry for SNF.
		 */
		if (io->i_flgs & F_SEVRE)		/* No retry */
			goto hard;
#endif	BOOTXX
		break;

	case ZDC_HDR_ECC:
#ifndef BOOTXX
                /*
                 * Short circuit ZDC_READ_HDRS failure
                 * so revector/retry is not attempted.
                 */
                if (cbp->cb_cmd == ZDC_READ_HDRS) {
                        io->i_error = EHDRECC;
                        goto hard;
                }
                /*
                 * Don't attempt to revector during a format
                 * or when bad sector forwarding is disabled.
                 * Otherwise, attempt to treat much like SNF.
                 */
                i = 0;
                if ((cbp->cb_mod & ZDC_NOECC) == 0
                &&  (io->i_flgs & F_NBSF) == 0
                &&  (i = zdrevector(io, cbp)) == 0) {
                        /*
                         * Reset errcnt for retries.
                         * Update initial cb for second part (beyond revectored
                         * sector). This effectively breaks up initial
                         * request into multiple requests.
                         */
                        io->i_errcnt = 0;
                        if (cbp->cb_count == 0)
                                goto done;
                        *acbp = *cbp;
                        goto restart;
                }
                /*
                 * If I/O on replacement sector failed, hard error.
                 * Otherwise, retry - since sector was not in bad block list.
                 */
                if (i < 0)
                        goto hard;
		io->i_error = EHDRECC;
		if (io->i_flgs & F_SEVRE)	 	/* No retry */
			goto hard;
		break;
#endif	BOOTXX
	case ZDC_SO:		/* indicates header problem */
#ifndef	BOOTXX
		io->i_error = ESO;
                /*
                 * No retry if ZDC_READ_HDRS failure
                 * or F_SEVRE mode set.
                 */
		if ((io->i_flgs & F_SEVRE) || cbp->cb_cmd == ZDC_READ_HDRS)
			goto hard;
		break;
#endif	BOOTXX
	case ZDC_NDS:
#ifndef	BOOTXX
                /*
                 * No retry if ZDC_READ_HDRS failure
                 * or F_SEVRE mode set.
                 */
		if ((io->i_flgs & F_SEVRE) || cbp->cb_cmd == ZDC_READ_HDRS) {
			io->i_error = EHER;
			goto hard;
		}
#endif	BOOTXX
		/* Fall into... */
	case ZDC_DRVFLT:
	case ZDC_SEEKERR:
	case ZDC_SEEK_TO:
	case ZDC_CH_TO:
	case ZDC_FDL:
		/*
		 * Retry these errors, unless ZDC_READ_HDRS failure.
		 */
		io->i_error = EHER;
		if (cbp->cb_cmd == ZDC_READ_HDRS)
			goto hard;

		break;

	default:
#ifndef	BOOTXX
		/*
		 * Unknown completion code - controller bad?
		 */
		printf("zd%d: Bad compcode 0x%x.\n", io->i_unit,
			cbp->cb_compcode);
		cbp->cb_compcode = NCOMPCODES - 1;	/* Get nice message */
#endif	BOOTXX
		io->i_error = EHER;
		goto hard;
	}

	if (cbp->cb_cmd == ZDC_RESET)
		return (-1);
#ifndef	BOOTXX
	printf("zd%d: Error (%s); cmd = 0x%x at (%d, %d, %d).\n",
		io->i_unit, zd_compcodes[cbp->cb_compcode], cbp->cb_cmd, 
		cbp->cb_cyl, cbp->cb_head, cbp->cb_sect);
	dumpstatus(cbp);
#endif	BOOTXX

	/*
	 * Retry. Reset the drive each time.
	 */
	if (io->i_errcnt++ < zdcretry) {
		lcb.cb_cmd = ZDC_RESET;
		if (dozdcmd(io, &lcb) < 0) {
#ifndef	BOOTXX
			printf("zd%d: RESET failed.\n", io->i_unit);
#endif	BOOTXX
			io->i_error = EHER;
			goto hard;
		}
		goto retry;
	}
#ifndef	BOOTXX
	else {
		/*
		 * save cb contents to return.
		 */
		*acbp = *cbp;
		saverrno = io->i_error;		/* save as ZDC_RESET clobbers */
		lcb.cb_cmd = ZDC_RESET;
		if (dozdcmd(io, &lcb) < 0)
			printf("zd%d: RESET failed.\n", io->i_unit);
		*cbp = *acbp;	/* restore return value */
		io->i_error = saverrno;
	}
#endif	BOOTXX

hard:
	/*
	 * Update argument cb and return
	 */
#ifndef	BOOTXX
	printf("zd%d: Hard error (%s); cmd = 0x%x at (%d, %d, %d).\n",
		io->i_unit, zd_compcodes[cbp->cb_compcode], cbp->cb_cmd, 
		cbp->cb_cyl, cbp->cb_head, cbp->cb_sect);
	dumpstatus(cbp);
	io->i_errblk = io->i_bn + ((io->i_cc - cbp->cb_count) >> DEV_BSHIFT);
	*acbp = *cbp;
#endif	BOOTXX
	return (-1);
}

/*
 * zdioctl(io, cmd, arg)
 *	struct iob *io;
 *	int cmd;		- ioctl
 *	struct cb *arg;		- ZDC cb is filled out by application
 *
 * The following commands are supported (setup in zdioctl()):
 *	SAIODEBUG - turn on/off debugging.
 *		  - ZD_BSFDEBUG bit defines Bad Sector Forwarding debug.
 *		  - ZD_DUMPCBDEBUG bit defines CB debugging.
 *	SAIOZSETBASE - Set base for I/O (i.e., i_boff)
 *	SAIOSETBBL - Set new bad block list. Replace bad block list with arg.
 *	SAIOZDCCMD - ZDC command. All ZDC commands are supported
 *		     except ZDC_READ, ZDC_WRITE.
 *
 * Others supported by driver but setup in ioctl():
 *
 *	SAIONOBAD  - inhibit bad sector forwarding.	(F_NBSF)
 *	SAIODOBAD  - enable bad sector forwarding.	(!F_NBSF)
 *	SAIOSEVRE  - severe burnin, no ECC, no retries.	(F_SEVRE)
 *	SAIONSEVRE - clear severe burnin.		(!F_SEVRE)
 *      SAIOFIRSTSECT - return first sector of usable space.
 *
 * Return 0 on successful completion.
 * Return -1 on error (io->i_error contains error code).
 */
zdioctl(io, cmd, arg)
	struct iob *io;
	int	cmd;
	register struct	cb *arg;
{
#ifndef	BOOTXX
	int flag;
	struct zdcdd *dd;
	struct zdc_ctlr *ctlrp;

	switch (cmd) {

	case SAIODEBUG:
		/*
		 * Turn on/off debug flag.
		 */
		flag = (int)arg;	
		if (flag > 0)
			io->i_debug |= flag;
		else
			io->i_debug &= ~flag;
		break;

	case SAIODEVDATA:
		printf("zd%d: unsupported ioctl 0x%x.\n", io->i_unit, cmd);
		io->i_error = ECMD;
		return (-1);

	case SAIOZDCCMD:
		io->i_cc = 0;

		switch (arg->cb_cmd) {

		/*
		 * The first set of ZDC commands will be used by the formatter
		 * and zdc debugging. All others will be unsupported.
		 */
		case ZDC_GET_CHANCFG:
		case ZDC_SET_CHANCFG:
			if (arg->cb_count != sizeof(struct zdcdd) ||
			    NOTALIGNED2((int)arg->cb_addr, ADDRALIGN) ||
			    arg->cb_iovec != 0) {
				io->i_error = EIO;
				return (-1);
			}
			return (dozdcmd(io, arg));

		case ZDC_WHDR_WDATA:
			if ((arg->cb_count != (DEV_BSIZE + CNTMULT)) ||
			    NOTALIGNED2((int)arg->cb_addr, ADDRALIGN) ||
			    arg->cb_iovec != 0) {
				io->i_error = EIO;
				return (-1);
			}
			return (dozdcmd(io, arg));

		case ZDC_FMTTRK:
		case ZDC_READ_HDRS:
			if (NOTMULT2(arg->cb_count, CNTMULT) ||
			    NOTALIGNED2((int)arg->cb_addr, ADDRALIGN) ||
			    arg->cb_iovec != 0) {
				io->i_error = EIO;
				return (-1);
			}
			return (dozdcmd(io, arg));

		case ZDC_READ_SS:
		case ZDC_WRITE_SS:
			if (arg->cb_count != ZDD_SS_SIZE ||
			    NOTALIGNED2((int)arg->cb_addr, ADDRALIGN) ||
			    arg->cb_cyl != ZDD_DDCYL || arg->cb_head != 0 ||
			    arg->cb_sect >= ZDD_NDDSECTORS ||
			    arg->cb_iovec != 0) {
				io->i_error = EIO;
				return (-1);
			}
			return (dozdcmd(io, arg));

		case ZDC_FMT_SS:
			if (arg->cb_count != CNTMULT ||
			    NOTALIGNED2((int)arg->cb_addr, ADDRALIGN) ||
			    arg->cb_cyl != ZDD_DDCYL || arg->cb_head != 0 ||
			    arg->cb_sect >= ZDD_NDDSECTORS ||
			    arg->cb_iovec != 0) {
				io->i_error = EIO;
				return (-1);
			}
			return (dozdcmd(io, arg));

		case ZDC_READ:
		case ZDC_WRITE:
		case ZDC_LONG_READ:
		case ZDC_LONG_WRITE:
		case ZDC_REC_DATA:
			ctlrp = &zdctrlr[ZDC_CTRLR(io->i_unit)];
			dd = (ZDC_DRIVE(io->i_unit) & 1) ? &ctlrp->zdc_chanB
							 : &ctlrp->zdc_chanA;
			if ((ctlrp->zdc_drivecfg[ZDC_DRIVE(io->i_unit)] & (ZD_FORMATTED|ZD_MATCH))
						!= (ZD_FORMATTED|ZD_MATCH)
			||  NOTMULT2(arg->cb_count, CNTMULT)
			||  NOTALIGNED2((int)arg->cb_addr, ADDRALIGN)
			||  arg->cb_cyl >= dd->zdd_cyls
			||  arg->cb_head >= dd->zdd_tracks
			||  arg->cb_sect >= (dd->zdd_sectors + dd->zdd_spare)
			||  arg->cb_iovec != 0) {
				io->i_error = EIO;
				return (-1);
			}
			io->i_cc = arg->cb_count;
			io->i_ma = (char *)arg->cb_addr;
			return (dozdcmd(io, arg));

		case ZDC_NOP:
		case ZDC_READ_LRAM:
		case ZDC_WRITE_LRAM:
		case ZDC_PROBE:
		case ZDC_PROBEDRIVE:
		case ZDC_RESET:
		case ZDC_SEEK:
		case ZDC_STAT:
			return (dozdcmd(io, arg));

		case ZDC_INIT:
			printf("zd%d: unsupported ioctl 0x%x.\n", io->i_unit,
				arg->cb_cmd);
			io->i_error = ECMD;
			return (-1);

		default:
			printf("zd%d: bad ioctl 0x%x.\n", io->i_unit,
				arg->cb_cmd);
			io->i_error = ECMD;
			return (-1);
		}
		break;

	case SAIOZSETBASE:
		/*
		 * Can be used to gain access to whole disk drive
		 * by setting i_boff to zero.
		 */
		io->i_boff = (daddr_t)arg;
		break;

	case SAIOSETBBL:
		/*
		 * set new bad block list
		 */
		if (arg == NULL) {
			io->i_error = ECMD;
			return (-1);
		}
		zdbsd[ZDC_CTRLR(io->i_unit) * ZDC_MAXDRIVES 
			+ ZDC_DRIVE(io->i_unit)] = (struct zdbad *)arg;
		break;

	case SAIOFIRSTSECT:
		/*
		 * return first sector of usable space.  For the zd driver,
		 * this is sector 0 of cylinder 1.
		 */
		ctlrp = &zdctrlr[ZDC_CTRLR(io->i_unit)];
		dd = (ZDC_DRIVE(io->i_unit) & 1) ? &ctlrp->zdc_chanB
						 : &ctlrp->zdc_chanA;
		*(int *)arg = dd->zdd_tracks * dd->zdd_sectors * 1;
		break;

	default:
		printf("zd%d: bad ioctl (('%c'<<8|%d).\n", io->i_unit,
			(u_char)(cmd >> 8), cmd & 0xff);
		io->i_error = ECMD;
		return (-1);
	}
	return (0);
#endif	BOOTXX
}

/*
 * findzdcs()
 *
 * Find all ZDCs and fill-in slic addresses in controller structure.
 * Also, if ZDC_DUMPONPANIC is set in the binary configurable "zdctrl"
 * variable, allocate an 8K dump area per ZDC for FW local RAM.
 * Only called on first open.
 */
static
findzdcs()
{
#ifdef BOOTXX
	register struct zdc_ctlr *ctlrp;
	register struct cfg_zdc *zdcfg;
	register struct cfg_boot *boot;

	boot = ((struct cfg_ptr *)CFG_PTR)->head_cfg;
	zdcfg = boot->b_cfg_zdc;
	for (ctlrp = zdctrlr; ctlrp < &zdctrlr[boot->b_num_zdc]; ctlrp++) {
		ctlrp->zdc_slicaddr = zdcfg->zdc_slic_addr;
		if (zdcfg->cfg_com.c_diag_flag & (CFG_FAIL|CFG_DECONF))
			ctlrp->zdc_state = ZDC_DEAD;
		else
			ctlrp->zdc_state = ZDC_ALIVE;
		zdcfg = (struct cfg_zdc *)zdcfg->cfg_com.c_next;
	}
#else	BOOTXX
	register struct zdc_ctlr *ctlrp;
	register struct config_desc *cd = CD_LOC;
	register struct ctlr_desc *cp;
	register int n;

	n = MIN(cd->c_toc[SLB_ZDCBOARD].ct_count, ZDC_MAXCTRLR);
	cp = &cd->c_ctlrs[cd->c_toc[SLB_ZDCBOARD].ct_start];
	for (ctlrp = zdctrlr; n-- > 0; ctlrp++,cp++) {
		ctlrp->zdc_slicaddr = cp->cd_slic;
		ctlrp->zdc_diagflag = cp->cd_diag_flag;
		if (cp->cd_diag_flag & (CFG_FAIL|CFG_DECONF))
			ctlrp->zdc_state = ZDC_DEAD;
		else
			ctlrp->zdc_state = ZDC_ALIVE;
		if (zdctrl & ZDC_DUMPONPANIC) {
			callocrnd(1024);	/* align at 1K boundary */
			ctlrp->zdc_dumpaddr = calloc(ZDC_LRAMSZ);
		}
	}
#endif
}

/*
 * init_zdc
 *
 *	- Initialize controller to where CBs live.
 *	- If ZDC_DUMPONPANIC, ZDC_INIT command will provide the
 *	  ZDC fw 8K of memory to dump its local RAM on controller panics.
 *	- Get channel configuration for channel with drive in question.
 *	- Issue ZDC_PROBE command to determine drive state.
 */
static
init_zdc(io)
	register struct iob *io;
{
	register int i;
	register struct zdc_ctlr *ctlrp;
	register struct cb *cbp;
	struct cb lcb;
	
	cbp = &lcb;
	ctlrp = &zdctrlr[ZDC_CTRLR(io->i_unit)];

#ifndef	BOOTXX
	{
		register int val;

		/*
		 * Check to see if controller FW is initialized.
		 * If, not wait until ZDC is initialized.
		 * If error, return EHER.
		 */
		i = zdcinitime * cpuspeed;
		for (;;) {
			val = rdslave(ctlrp->zdc_slicaddr, SL_Z_STATUS);
			if (val < 0) {
				/*
				 * Slic error
				 */
				io->i_error = EHER;
				return;
			}

			if ((val & SLB_ZPARERR) != SLB_ZPARERR) {
				if ((val & ZDC_READY) == ZDC_READY)
					break;
				if ((val & ZDC_ERRMASK) == ZDC_SINIT)  {
					/* Still initializing */
					if (--i)
						continue;
				}
			}

			printf("zd%d: Ctrlr status == 0x%x.\n", io->i_unit, val);
			io->i_error = EHER;
			return;
		}
	}
#endif	BOOTXX

	/*
	 * Tell the ZDC where the CB array is located.
	 * Done via Bin1-Bin4 SLIC interrupts, where Bin 1 is the
	 * least significant byte of the CB address and Bin 4 is
	 * the most significant byte.
	 */
	for (i=0; i < NBPW; i++) {
		if (mIntr(ctlrp->zdc_slicaddr, i+1, (int)zdcb >> (i * NBBY))) {
#ifndef	BOOTXX
			printf("zd%d: Cannot init mIntr.\n", io->i_unit);
#endif	BOOTXX
			io->i_error = EHER;
			return;
		}
	}

#ifndef	BOOTXX
	{
		register struct init_cb *icbp;	/* for init_cb overlay */

		/*
		 * Give FW a place to dump its LRAM.
		 */
		icbp = (struct init_cb *)cbp;
		bzero((char *)icbp, sizeof(struct init_cb));
		icbp->icb_cmd = ZDC_INIT;
		zdctrl &= ~ZDC_ENABLE_INTR;		/* paranoia */
		icbp->icb_ctrl = zdctrl;
		if (zdctrl & ZDC_DUMPONPANIC)
			icbp->icb_dumpaddr = ctlrp->zdc_dumpaddr;
		if (dozdcmd(io, (struct cb *)icbp) < 0) {
			printf("zd%d: Cannot init.\n", io->i_unit);
			return;
		}
	}
#endif	BOOTXX

	/*
	 * Probe devices on ZDC via ZDC_PROBEDRIVE.
	 * Store results in controller structure.
	 */
	bzero((char *)cbp, sizeof(struct cb));
	cbp->cb_cmd = ZDC_PROBEDRIVE;
	if (dozdcmd(io, cbp) < 0) {
#ifndef	BOOTXX
		printf("zd%d: Cannot probe.\n", io->i_unit);
#endif	BOOTXX
		return;
	}
	for (i=0; i < ZDC_MAXDRIVES; i++)  {
		ctlrp->zdc_drivecfg[i] =
				  ((struct probe_cb *)cbp)->pcb_drivecfg[i];
	}

	/*
	 * Get the channel information for channel on which this drive
	 * resides.
	 */
	cbp->cb_cmd = ZDC_GET_CHANCFG;		/* command */
	cbp->cb_addr = (u_long)chancfg;		/* where to put it */
	cbp->cb_count = sizeof(struct zdcdd);
	cbp->cb_iovec = NULL;
	if (dozdcmd(io, cbp) < 0) {
#ifndef	BOOTXX
		printf("zd%d: Cannot get channel cfg.\n", io->i_unit);
#endif	BOOTXX
		return;
	}
	if (cbp->cb_compcode == ZDC_NOCFG)
		return;
	if (ZDC_DRIVE(io->i_unit) & 1)
		ctlrp->zdc_chanB = *chancfg;
	else
		ctlrp->zdc_chanA = *chancfg;
}

/*
 * zdgetrpl()
 *	Search bad block list for replacement sector for bad sector (SNF).
 *
 * Return:
 *	pointer to replacement sector
 *	otherwise NULL pointer if no replacement sector found.
 *	
 */
static struct diskaddr *
zdgetrpl(badsect, zdbad)
	register struct diskaddr *badsect;
	struct zdbad *zdbad;
{
	register int i;
	register struct bz_bad *bb;

	bb = zdbad->bz_bad;
	for (i = 0; i < zdbad->bz_nelem; i++, bb++) {
		if (bb->bz_cyl < badsect->da_cyl)
			continue;
		if (bb->bz_cyl > badsect->da_cyl)
			break;
		/* cylinder matched */
		if (bb->bz_head != badsect->da_head)
			continue;
		/* head matched */
		if (bb->bz_sect == badsect->da_sect) {
			if (bb->bz_rtype == BZ_SNF)
				return (&bb->bz_rpladdr);
			continue;	/* could be BZ_PHYS */
		}
	}
	return (NULL);
}

/*
 * zdrevector
 *	- revector bad sectors - if possible.
 *
 * Find replacement sector for a bad block and initiate I/O
 * to the replacement sector.
 * 
 * Return values:
 *	-1 - revectoring attempted unsuccessfully.
 *	 0 - revectoring successful.
 *	 1 - No replacement sector in bad block list.
 */
static
zdrevector(io, acbp)
	register struct iob *io;
	register struct cb *acbp;
{
	register struct zdc_ctlr *ctlrp;
	register struct zdcdd *dd;
	register struct diskaddr *replcmnt;
	struct cb lcb;				/* cb for new request */
	struct cb scb;				/* saved cb to continue */

#ifndef	BOOTXX
	if (io->i_debug & ZD_BSFDEBUG)
		printf("zd%d: revectoring (%d, %d, %d)", io->i_unit,
			acbp->cb_cyl, acbp->cb_head, acbp->cb_sect);
#endif	BOOTXX
	/*
	 * Determine if bad block is in bad block list.
	 */
	replcmnt = zdgetrpl(&acbp->cb_diskaddr,
		zdbsd[ZDC_CTRLR(io->i_unit) * ZDC_MAXDRIVES
			 + ZDC_DRIVE(io->i_unit)] );
	if (replcmnt == NULL) {
		/*
		 * Not in bad block list. Caller should retry
		 * bad block.
		 */
#ifndef	BOOTXX
		if (io->i_debug & ZD_BSFDEBUG)
			printf(": not in table.\n");
#endif
		return (1);
	}

	/*
	 * Save state of current request, do new request for replacement.
	 */
	acbp->cb_addr = (u_long)io->i_ma
			+ ((io->i_cc + (DEV_BSIZE-1) - acbp->cb_count)
			    & ~(DEV_BSIZE-1));
	scb = *acbp;
	lcb = *acbp;

	ctlrp = &zdctrlr[ZDC_CTRLR(io->i_unit)];
	dd = (ZDC_DRIVE(io->i_unit) & 1) ? &ctlrp->zdc_chanB
					 : &ctlrp->zdc_chanA;
	lcb.cb_count = DEV_BSIZE;
	lcb.cb_diskaddr = *replcmnt;
#ifndef	BOOTXX
	if (io->i_debug & ZD_BSFDEBUG)
		printf(" to (%d, %d, %d).\n", replcmnt->da_cyl,
			replcmnt->da_head, replcmnt->da_sect);
#endif	BOOTXX

	if (dozdcmd(io, &lcb) < 0) {
		/*
		 * No can do. restore saved state.
		 */
#ifndef	BOOTXX
		if(io->i_debug & ZD_BSFDEBUG)
			printf("zd%d: Revectoring failed.\n", io->i_unit);
#endif	BOOTXX
		*acbp = scb;
		return (-1);
	}

	/*
	 * Revectoring completed successfully.
	 * If not last sector of transfer, restore cb from saved.
	 */
#ifndef	BOOTXX
	if (io->i_debug & ZD_BSFDEBUG)
		printf("zd%d: Revectoring succeeded.\n", io->i_unit);
#endif	BOOTXX
	if (scb.cb_count != DEV_BSIZE) {
		*acbp = scb;
		acbp->cb_addr += DEV_BSIZE;
		acbp->cb_count -= DEV_BSIZE;
		if (++acbp->cb_sect == dd->zdd_sectors) {
			acbp->cb_sect = 0;
			if (++acbp->cb_head == dd->zdd_tracks) {
				acbp->cb_head = 0;
				acbp->cb_cyl++;
			}
		}
	}
	return (0);
}

#ifndef	BOOTXX
/*
 * Print out status bytes when appropriate.
 */
static
dumpstatus(cbp)
	register struct cb *cbp;
{
	register int i;

	if (cbp->cb_cmd == ZDC_INIT || cbp->cb_cmd == ZDC_PROBE ||
	    cbp->cb_cmd == ZDC_PROBEDRIVE)
		return;

	switch(cbp->cb_compcode) {

	case ZDC_DRVPROT:
	case ZDC_DRVFLT:
	case ZDC_SEEKERR:
	case ZDC_HDR_ECC:
	case ZDC_SOFTECC:
	case ZDC_CORRECC:
	case ZDC_ECC:
	case ZDC_SNF:
	case ZDC_REVECT:
	case ZDC_SO:
	case ZDC_NDS:
	case ZDC_FDL:
	case ZDC_DDC_STAT:
		break;

	default:
		if (cbp->cb_cmd == ZDC_READ_LRAM || cbp->cb_cmd == ZDC_WRITE_LRAM)
			break;
		return;
	}
	printf("cb_status: ");
	for (i=0; i < NSTATBYTES; i++)
		printf("0x%x, ", cbp->cb_status[i]);
	printf("\n");
}
#endif	BOOTXX

#ifdef	DEBUG
#ifndef	BOOTXX
/*
 * Print out the contents of a cb.
 */
static
dumpcb(cbp)
	register struct cb *cbp;
{
	register struct init_cb *icbp;
	register int i;

	printf("cb at 0x%x, cb_cmd 0x%x, cb_compcode 0x%x\n",
		cbp, cbp->cb_cmd, cbp->cb_compcode);
	if (cbp->cb_cmd == ZDC_INIT) {
		icbp = (struct init_cb *)cbp;
		printf("icb_ctrl 0x%x, icb_pagesize 0x%x, icb_dumpaddr 0x%x\n",
			icbp->icb_ctrl, icbp->icb_pagesize, icbp->icb_dumpaddr);
		printf("icb_dest 0x%x, icb_bin 0x%x, icb_vecbase 0x%x\n",
			icbp->icb_dest, icbp->icb_bin, icbp->icb_vecbase);
		printf("icb_errdest 0x%x, icb_errbin 0x%x, icb_errvector 0x%x\n\n",
			icbp->icb_errdest, icbp->icb_errbin,
			icbp->icb_errvector);
		return;
	}
	if (cbp->cb_cmd == ZDC_PROBE || cbp->cb_cmd == ZDC_PROBEDRIVE) {
		printf("pcb_drivecfg:");
		for (i = 0; i < ZDC_MAXDRIVES; i++)
			printf(" 0x%x",
				 ((struct probe_cb *)cbp)->pcb_drivecfg[i]);
		printf("\n\n");
		return;
	}
	printf("cb_mod 0x%x, cb_diskaddr (%d, %d, %d), cb_addr 0x%x\n",
		cbp->cb_mod, cbp->cb_cyl, cbp->cb_head, cbp->cb_sect,
		cbp->cb_addr);
	printf("cb_count 0x%x, cb_iovec 0x%x, cb_reqstat 0x%x\n",
		cbp->cb_count, cbp->cb_iovec, cbp->cb_reqstat);
	dumpstatus(cbp);
	printf("\n");
}
#endif	BOOTXX
#endif	DEBUG
