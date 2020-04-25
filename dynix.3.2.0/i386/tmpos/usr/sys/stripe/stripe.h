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

/*	$CHeader: stripe.h 1.4 87/07/06 19:25:18 $	*/
/*	Copyright 1986 Convex Computer Corporation	*/

/*
 * $Header: stripe.h 1.4 1991/07/09 21:33:13 $
 */

/*
 * stripe.h
 *	stripe driver defintions.
 */

/* $Log: stripe.h,v $
 *
 *
 */

#define STRIPECAP "/etc/stripecap"        /* Path to stripe database file */

/* 
 * Major device numbers of interest to utilities 
 * and stripe driver.  These must match what is
 * in devices.balance.
 */
#define STRIPECHAR 	30	/* Major device number of raw stripe-device */
#define STRIPEBLOCK	11	/* Major device number of block stripe-device */
#define ZDBLOCK		7	/* Major device number of block zd-device */
#define SDBLOCK		1	/* Major device number of block sd-device */

/*
 * Note that the use of this definition 
 * may change in the future.  Currently 
 * the disk drivers all use the same minor 
 * device defintion, with bits 0-3 being 
 * partition number and the rest as the 
 * driver unit number.
 */
#define N_PARTITIONS	8

/*
 * MAX_STRIPE_PARTITIONS is the cap on the number of partitions which can
 * make up any given stripe.
 *
 * MAX_STRIPE_DEVICES is the cap on the number of /dev/rds* entries that
 * can exist.  The number of psuedo devices actually configured may not
 * exceed this value or it will be truncated.  Configuring less saves memory.
 *
 * MIN_STRIPE_BUFS is the cap on the number of buf headers that requests
 * to a given stripe can be broken into.  The variable "stripe_maxbufs"
 * initialized in conf_stripe.c, must be greater than this value or it
 * will be assumed to be MIN_STRIPE_BUFS.
 */
#define	MAX_STRIPE_PARTITIONS	32	/* arbitrary		        */
#define	MAX_STRIPE_DEVICES	256	/* max. 256 due to minor(dev)   */
#define	MIN_STRIPE_BUFS		2

/*
 * The following definition is the maximum
 * number of blocks in a device.  One more
 * than this times DEV_BSIZE and it looks
 * like a negative number in physck(), since
 * type 'off_t' is a 32 bit signed integer.
 */
#define	MAX_DEV_BLKS		0x003fffff

/* 
 * The following defines a the minimum size for
 * a stripe section's stripe block size.  In addition
 * stripe block sizes must be a multiple of it.  
 * This restriction allows striping to work with 
 * optimizations that the kernel uses when dealing
 * with filesystems, paging, and swapping.
 */
#define MIN_SBLK		(MAXBSIZE / DEV_BSIZE)

/*
 * Definition for stripe configuration
 * which must be loaded to use the stripe.
 *
 * The stripe is spread across multiple
 * devices, which must be listed in order
 * according to which contributes the most
 * physical blocks to the stripe.  Since
 * contributions may differ, the stripe
 * is broken into sections spread accross
 * the first devices listed in which each
 * device contributes an equal number of
 * real blocks to the section.  
 *
 * Each stripe section description indicates
 * its physical block offset into the overall
 * stripe, how many of the first 'n' devices 
 * contribute to that section and how many 
 * blocks they contribute.  It also contains
 * an interleave factor - the number of 
 * sequential blocks from one device before
 * interleaving with the next device in the
 * section.  
 *
 */
struct	stripe_dev	{
	dev_t	st_dev[MAX_STRIPE_PARTITIONS]; /* Major/minor striped devices */
	struct	{
		/* Description for a stripe section */
		long	st_start;	/* Starting block offset into stripe */
		long	st_size;	/* # blocks from each device */
		int	st_ndev;	/* # of devices in this section */
		int	st_block;	/* Interleave - stripe block size */
	} st_s[MAX_STRIPE_PARTITIONS];
	long	st_total_size;	/* Total size in device blocks of this stripe */
	};

typedef struct stripe_dev stripe_t;

/*
 * Information for managing access
 * to the stripes, its configuration,
 * and statistics.
 */
struct stripe_info {
	int n_opens;			/* Number of active opens on device */
	bool_t exclusive_open;		/* True if open for exclusive access */
	sema_t user_sync;		/* Synchronization of access for 
					 * configuration integrity.  Pertains
					 * to open/close/ioctl  */
	lock_t buf_lock;		/* Synchronization for active requests.
					 * For data integrity when multiple
					 * parallel requests terminate. */
	stripe_t config;		/* This unit's stripe configuration */
	unsigned qlen;			/* # of requests currently queued */
	unsigned rcount;		/* Total requests to this device */ 
	unsigned bcount;		/* Total blocks xferred w/ device */ 
};

typedef struct stripe_info stripeinfo_t;

#ifdef	STRIPEMACS
/*	
 * Macros for manipulating a stripe segment.
 * 'x' is expected to be a "stripe_t *"
 * 'i' is the partition index within that stripe.
 */
#define	ssize(x,i)	((x)->st_s[i].st_size)
#define	sstart(x,i)	((x)->st_s[i].st_start)
#define	sndev(x,i)	((x)->st_s[i].st_ndev)
#define	sblock(x,i)	((x)->st_s[i].st_block)
#endif

/* Ioctl definitions 	*/
#define STPUTTABLE	_IO(S,0)/* Load the stripe table for the device */
#define STGETTABLE	_IO(S,1)/* Retreive the stripe table for the device */

#ifdef	KERNEL

/*
 * Binary configurable parameters declared
 * and initialized in conf_stripe.c.
 */
extern int nstripebufs;			/* # of bufs in the driver local pool */
extern int stripe_maxbufs;		/* Maximum number of bufs a request may
                                         * have at once from the local pool */
extern gate_t stripegate;

#endif

#ifndef	KERNEL
struct vtctodisktab * tryvtoc();
#endif

struct part	{
	daddr_t p_start;	/* start sector no of partition*/
	long	p_size;		/* # of blocks in partition*/
	int	p_type;		/* type of this partition */
	short	p_bsize;	/* block size in bytes */
	short	p_fsize;	/* frag size in bytes */
};

/*
 * Transfer structure for vtoc to disktab
 */
struct vtctodisktab {
	char	*v_disktype;	/* type of disk this is */
	int	v_secsize;	/* sector size in bytes */
	int	v_ntracks;	/* # tracks/cylinder */
	int	v_nsectors;	/* # sectors/track */
	int	v_ncylinders;	/* # cylinders */
	int	v_rpm;		/* revolutions/minute */
	struct part v_part[8];
};
