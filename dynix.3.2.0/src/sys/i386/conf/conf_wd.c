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
static	char	rcsid[] = "$Header: conf_wd.c 1.4 91/03/13 $";
#endif

/*
 * conf_wd.c - SSM scsi disk device driver configuration file
 */

/* $Log:	conf_wd.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/uio.h"
#include "../h/systm.h"
#include "../balance/cntrlblock.h"
#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../h/scsi.h"
#include "../ssm/ssm_scsi.h"		/* scsi common data structures */
#include "../machine/ioconf.h"		/* IO Configuration Definitions */
#include "../ssm/wd.h"			/* driver local structures */
#include "../h/vtoc.h"

/*
 * Partition tables. 
 * NOTE: Should be cleanly merged with the standalone.
 *       These partitions that go to the end of the disk are grossly
 *       exaggerated so that varying disk sizes can be used.
 * NOTE: The newfs utility ASSUMES that the 'c' partition starts at the
 *	 beginning of the disk when writing the bootstrap program.
 *	 A bootstrap program is written when a root filesystem is created.
 *	 The newfs utility ASSUMES that the 'a' partition is the root
 *	 filesystem. However, by writing the bootstrap to partition 'c' the
 *	 'a' partition may be moved to the middle of the disk to reduce
 *	 seek latency.
 *	 For SSM derivations it is desirable to avoid putting the old
 *	 style of bootstrap at the beginning of the disk, where 2nd
 *	 level SSM firmware resides.  Therefore, be sure to use the
 *	 "-n" option to newfs. 
 * NOTE: Disk partitions that extend to the end of the disk are sized
 *	 as WD_END which allows several different sized drives to work with
 *	 the same partition table.  DYNIX adjusts to the actual size of the
 *	 drive.
 */

struct cmptsize wd_scsi_part[NUMPARTS] = {
/*	start,			length,			         */
	2415,			33810,		/* minor 0 ('a') */
	2415+33810,		33810,		/* minor 1 ('b') */
	0,			WD_END,	 	/* minor 2 ('c') */
	2415+(2*33810),		33810,		/* minor 3 ('d') */
	2415+(3*33810),		168084,		/* minor 4 ('e') */
	2415+(3*33810)+168084,	WD_END,		/* minor 5 ('f') */
	2415+(2*33810),		WD_END,		/* minor 6 ('g') */
	0,			2415,		/* minor 7 ('h') */
};

#define WD_CB_IOVEC	(howmany(WD_MAX_XFER * DEV_BSIZE, CLBYTES) + 1)

/*
 * Configure the device's tuning parameters.
 *
 * The commented number to the far right in the table below will be the 
 * unit number portion of the devices major/minor pair.
 *
 * The structure of the minor number is
 * bits 0-2 are the partition table index; 3-7 is the index into
 * the binary configuration table.
 *
 * partab:	partition table entry for each unit.
 * num_iat:	number of iat's which are allocated for operation.
 * 		This is a per command block number, the SSM has 
 * 		NCB_SCSI_DEV command blocks per disk.
 *		This parameter must be large enough to handle 
 * 		the maximum device transfer request. Where
 *		each iat maps CLBYTES.
 *
 * NOTE: The partition table entry(below) must contain a valid partition
 *	 table that has the proper number of entries(above).
 *	 UNPREDICTABLE DRIVER ACTION AND RESULTS WILL OCCUR OTHERWISE.
 */
struct wd_bconf wdbconf[] = { 
/*	partab,		num_iovecs, 	blocks/seconds     */
	{wd_scsi_part,	WD_CB_IOVEC,	60*17 },	/*0*/
	{wd_scsi_part,	WD_CB_IOVEC,	60*17 },	/*1*/
	{wd_scsi_part,	WD_CB_IOVEC,	60*17 },	/*2*/
	{wd_scsi_part,	WD_CB_IOVEC,	60*17 },	/*3*/
	{wd_scsi_part,	WD_CB_IOVEC,	60*17 },	/*4*/
	{wd_scsi_part,	WD_CB_IOVEC,	60*17 },	/*5*/
	{wd_scsi_part,	WD_CB_IOVEC,	60*17 },	/*6*/
	{wd_scsi_part,	WD_CB_IOVEC,	60*17 }		/*7*/
};

int	wdretrys = 4;	/* Number of retrys before allowing a hard error   */
gate_t	wdgate = 58;	/* gate for this device driver */

/*
 * NOTE, supported disks must have the following characterisitics:
 * Byte 3 of an INQUIRY command must 1 to indicate embedded scsi
 * drives.
 * Byte 0 of an INQUIRY command must be 0 on all recognized SCSI disks.
 */

int	wdmaxminor = sizeof(wdbconf)/sizeof(struct wd_bconf);
