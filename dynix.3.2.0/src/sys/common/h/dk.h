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

/*
 * $Header: dk.h 2.2 87/01/13 $
 */

/* $Log:	dk.h,v $
 */

/*
 * Instrumentation per drive.
 *
 * We'll assume that each drive has a soft state associated with it,
 * with at least the following fields:
 *	struct soft_state {
 *		struct timeval	starttime;	/* xfer start time * /
 *		struct dk	*mydkp;		/* pointer to dk stats * /
 *		int		myunit;		/* my logical unit number * /
 *	} *softp;
 *
 * dk_ndrives
 *	This is a binary configurable parameter; the maximum
 *	possible number of drives for which we can record
 *	accounting statistics.  dk_ndrives will probably be a
 *	small number, around 6 or 8, it is constant for the
 *	life of the kernel.
 *
 * dk
 *	This is a pointer to an array of dk_ndrives struct
 *	dk's.  dk[0] will record the info for the 0th drive;
 *	dk[1] for the 1th drive; etc.  This array may either be
 *	allocated dynamically at autoconf time or allocated
 *	statically in some file like param.c.
 *
 * dk_nxdrive
 *	Initially: 0.
 *
 *	During autoconf calls to the boot routine: This is the
 *	current number of drives described in the array dk[].
 *	Drives are placed in the dk[] array by each disk
 *	driver's boot routine, by code that might look
 *	something like:
 *
 *		if (dk_nxdrive < dk_ndrives) {
 *			softp->mydkp = &dk[dk_nxdrive++];
 *			bcopy("sdX", dkp->dk_name, 4);
 *			softp->dkp->dk_name[2] = '0' + softp->myunit;
 *			softp->mydkp->dk_bps = 60*17;
 *		}
 *
 *	After autoconf: This is the number of drives being
 *	monitored.  It will be <= dk_ndrives.
 *
 *
 * The Fields in struct dk:
 *
 * dk_name
 *	Up-to-10-character null-terminated name of the drive.
 *	Set up at boot time when the dk entry is initialized.
 *
 * dk_time
 *	Amount of time we think the device is busy.  This can
 *	be calculated by recording the time you start the
 *	device and the time you get the interrupt from the
 *	device and subtracting the first from the second, ala:
 *
 *	sdstart(...)
 *	{	...
 *		if (device is starting now) {
 *			softp->starttime = time;
 *		} ...
 *	}
 *
 *	sdintr(...)
 *	{	...
 *		if (device is idling now) {
 *			struct timeval elapsed = time;
 *
 *			timevalsub(&elapsed, &softp->starttime);
 *			timevaladd(&softp->mydkp->dk_time, &elapsed);
 *		} ...
 *	}
 *
 * dk_seek
 *	Number of seeks completed for the drive.  For SCSI
 *	drives, it may be necessary to approximate this number,
 *	incremented appropriately at each disk transfer (in the
 *	driver's interrupt routine?).
 *
 * dk_xfer
 *	Number of completed disk transfers for the drive,
 *	incremented at each disk transfer (in the driver's
 *	interrupt routine?).
 *
 * dk_blks
 *	This is the number of completed full (and partial) disk
 *	blocks transferred by the drive.  Presumably, dk_blks >=
 *	dk_xfer (bumped in the driver's interrupt routine?).
 *
 * dk_bps
 *	This is the raw transfer rate supported by the drive in
 *	blocks per second.  For SCSI disks right now, we are
 *	assuming that the transfer rate is the revolution rate
 *	times the number of sectors per track (60*17 blocks/sec).
 *	This number is set up at boot time, along with the disk's
 *	name.  The number should be binary configurable in order
 *	to support multiple drive types.
 */
struct dk {
	struct timeval	dk_time;	/* time the disk is busy */
	long	dk_seek;	/* number of seeks on drive */
	long	dk_xfer;	/* number of transfers on drive */
	long	dk_blks;	/* number of disk sectors transferred */
	long	dk_bps;		/* transfer rate in sectors/second */
	char	dk_name[10];	/* disk drive name */
};

#ifdef KERNEL
extern	int dk_ndrives;		/* max number of drives recordable */
extern	int dk_nxdrive;		/* number of drives recorded */
extern	struct dk dk[];		/* array of entries */
#endif KERNEL
