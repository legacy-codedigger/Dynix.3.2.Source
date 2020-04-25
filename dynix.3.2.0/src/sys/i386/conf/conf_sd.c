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

#ifndef	lint
static	char	rcsid[] = "$Header: conf_sd.c 2.8 90/04/05 $";
#endif

/*
 * conf_sd.c - scsi disk device driver configuration file
 */

/* $Log:	conf_sd.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/uio.h"
#include "../h/systm.h"
#include "../h/vtoc.h"
#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../sec/sec.h"			/* scsi common data structures */
#include "../machine/ioconf.h"		/* IO Configuration Definitions */
#include "../sec/sd.h"			/* driver local structures */

/*
 * Partition tables. 
 * NOTE: Should be cleanly merged with the standalone.
 *       These partitions that go to the end of the disk are grossly
 *       exaggerated so that varying disk sizes can be used.
 * NOTE: The newfs utility ASSUMES that the 'c' partition starts at the
 *	 beginning of the disk when writing the bootstrap program.
 *	 The bootstrap program is written when a root filesystem is created.
 *	 The newfs utility ASSUMES that the 'a' partition is the root
 *	 filesystem. However, by writing the bootstrap to partition 'c' the
 *	 'a' partition may be moved to the middle of the disk to reduce
 *	 seek latency.
 *	 If the 'c' partition is changed so that it does not include the
 *	 start of the disk, then be sure to use the "-n" option to newfs
 *	 and use /stand/installboot to write the bootstrap program (at least
 *	 1 partition must start at the beginning of the disk).
 * NOTE: Disk partitions that extend to the end of the disk are sized
 *	 as SD_END which allows several different sized drives to work with
 *	 the same partition table.  DYNIX adjusts to the actual size of the
 *	 drive.
 */

/* Partition size defines */
#define	A_PART	15884
#define	B_PART	33440
#define	D_PART	15884
#define	F_PART	300575

struct cmptsize scsi_part[NUMPARTS] = {	/* scsi disk */
/*
start,			length,		*/
0,			A_PART,		/* minor 0 ('a') */
A_PART,			B_PART,		/* minor 1 ('b') */
0,			SD_END,		/* minor 2 ('c') */
A_PART+B_PART,		D_PART,		/* minor 3 ('d') */
A_PART+B_PART+D_PART,	SD_END,		/* minor 4 ('e') */
A_PART+B_PART,		F_PART,		/* minor 5 ('f') */
A_PART+B_PART,		SD_END,		/* minor 6 ('g') */
A_PART+B_PART+F_PART,	SD_END,		/* minor 7 ('h') */
};

/*
 * Configure the device's tuning parameters.
 *
 * The number to the far right in the table below will be the 
 * unit number portion of the devices major/minor pair.
 *
 * The structure of the minor number is
 * bits 0-2 are the partition table index; 3-7 is the index into
 * the binary configuration table.
 *
 * buf_sz:	currently used to handle ioctl return information.
 * partab:	partition table entry for each unit.
 * num_iat:	number of iat's which are calloc'd for operation.
 *		This parameter must be large enough to handle all device
 *		programs allocated to allow low and thresh to work properly and
 *		should be set to (num_device_progs*(CLSIZE>=7?7:CLSIZE))
 *		Where the maximum raw transfer size will be constrained to
 *		a minimum of ((num_iat-1)*CLBYTES).
 * low:		once all device programs have been filled out the interrupt
 *		procedure will not queue any more until the queue has drained
 *		off to below low. Minimum value is 2.
 * thresh:	number of device programs to place in the queue maximum on
 *		each strategy or interrupt call to the start procedure. This
 *		allow the queue to be filled up at a controlled rate.
 *
 * NOTE: The partition table entry(below) must contain a valid partition
 *	 table that has the proper number of entries(above).
 *	 UNPREDICTABLE DRIVER ACTION AND RESULTS WILL OCCUR OTHERWISE.
 */
struct sd_bconf sdbconf[] = {				/*
buf_sz,	partab,		num_iat, low,	thresh, bps		   */
{128,	scsi_part,	129,	5,	2,	60*17 },	/*0*/
{128,	scsi_part,	129,	5,	2,	60*17 },	/*1*/
{128,	scsi_part,	129,	5,	2,	60*17 },	/*2*/
{128,	scsi_part,	129,	5,	2,	60*17 },	/*3*/
{128,	scsi_part,	129,	5,	2,	60*17 },	/*4*/
{128,	scsi_part,	129,	5,	2,	60*17 },	/*5*/
{128,	scsi_part,	129,	5,	2,	60*17 },	/*6*/
{128,	scsi_part,	129,	5,	2,	60*17 },	/*7*/
};

int	sdretrys = 4;	/* Number of retrys before allowing a hard error   */
gate_t	sdgate = 58;	/* gate for this device driver */

#ifdef SDDEBUG
/*
 * spin time for async timeouts which should never occur in a
 * production system unless there is bad hardware (target adapter)
 * which never comes back or timeouts.
 */
int	sdspintime = 2000000;
#endif SDDEBUG

/*
 * bit patterns expected in results from INQUIRY command.  To identify
 * units on a target adaptor, byte 3 must be sdinq_targformat; CCS
 * disks will have byte 3 = sdinq_ccsformat.  Byte 0 must be sddevtype
 * on all recognized SCSI disks.
 */

u_char	sddevtype = 0;
u_char	sdinq_targformat = 0x0;
u_char	sdinq_ccsformat = 0x1;

/*
 * DON'T CHANGE ANY THING BELOW THIS LINE OR ALL BETS ARE OFF!
 */
int	sdmaxminor = sizeof(sdbconf)/sizeof(struct sd_bconf);
int	sdsensebuf_sz = 32 * sizeof(char);
