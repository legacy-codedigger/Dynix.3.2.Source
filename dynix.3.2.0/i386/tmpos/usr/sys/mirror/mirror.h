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
 * $Header: mirror.h 1.1 89/08/17 $
 */

/* $Log:	mirror.h,v $
 */

#define MIRROR_SPL	SPL5

/* For ioctls--the dev_t block works like so: */
#define U0RAW	0			  /* Unit 0 character device */
#define U0BLOCK	1			  /* Unit 0 block device */
#define U1RAW	2			  /* Unit 1 character device */
#define U1BLOCK 3			  /* Unit 1 block device */

#define UPM 2				  /* units per mirror MUST BE TWO */

#define MIRROR_N_DEV_T  (2*UPM)

#define MIOCON		_IOW(M,0, dev_t [MIRROR_N_DEV_T])
#define MIOCOFF		_IO(M,1)
#define MIOCINFO	_IOR(M,2,struct mioc)
#define MIOCOINFO	_IOWR(M,3,struct mioc)
#define MIOCVRFY	_IOW(M,4, dev_t [MIRROR_N_DEV_T])
	
struct mirror_info {
	/* core_info must be first. */
#define core_info 	u_long	open, active, nunits, last_read
	core_info;
#ifdef KERNEL
	lock_t	lock;
	u_char	dolabel;		  /* do labels when not wip. */
	/*
	 * start_slic is MAX_NUM_SLIC when not in use.  It is used by
	 * mirror_start to record the slic id of the processor that is holding
	 * the lock for this mirror.  If the interrupt routine is entered for
	 * this mirror by the same processor, it's not by a true interrupt but
	 * a call from biodone done by the host disk driver's strategy routine.
	 */
	u_char	start_slic;		  /* when valid, the locker */
	u_long	blk_min, blk_toobig, blk_offset;
	struct mirror_unit	*unit;
	struct buf queue;
#endif /* KERNEL */
};

struct mioc_info {
	core_info;
};

#undef core_info
/* (struct mirror_info *)->active */
#define INACTIVE	0		  /* not mirroring */
#define CHANGING	1		  /* transition between */
#define ACTIVE		2		  /* in use--mirroring */
#define SHUTDOWN	3		  /* shutting down mirroring */
/* (struct mirror_unit *)->ok */
#define  TRUE 1
#define FALSE 0

/* (struct mirror_label *)->bmap? */
#define MRU_STATE	0x80		  /* top  bit of char */
#define MRU_IDLE	0		  /* Not currently in use */
#define MRU_INUSE	0x80		  /* in an active mirror */
#define MRU_MODE	0x60		  /* next two bits of char */
#define MRU_D3		0		  /* Dynix/3 doesn't have modes */
#define MRU_SYNC	0x20		  /* in sync */
#define MRU_CONV	0x40		  /* converging */
#define MRU_DIV		0x60		  /* diverging */

struct mirror_unit {
	/* core_unit must be first */
#define core_unit	\
	dev_t	block, raw;	\
	bool_t	ok;	\
	bool_t  wip			  /* flag for work in progress */
	core_unit;
#ifdef KERNEL
	struct mirror_label *label;
	struct buf ubuf;
#endif /* KERNEL */
};

struct mioc_unit {
	core_unit;
};
#undef core_unit

struct mirror_label {
	u_long	mirror_magic;		/* always MIRROR_MAGIC */
	time_t	label_time;		/* time of last labelling */
	u_short	mminor;			/* minor dev # of this mirror */
	u_short	unit;			/* unit number, 0 or 1 on 3.0 */
	u_long	label_size;		/* label_size, in blocks, 1 in 3.0 */
	u_long	first_blk;		/* # of 1st blk, 0 or 1 in 3.0 */
	u_long	blk_break;		/* # of last block + 1 */
	/* in mirror 3.0, the next quantity is always 0xffffffff */
	u_long	bmap_density;		/* # of blocks per bit in bmap */
	u_short	nunits;			/* # of units, always 2 in 3.0 */
	/* Following are the block maps for each unit.  The first two
	 * bits indicate unit state, the next if the unit is up to
	 * date or not (there is only one bit at maximum density).  In
	 * order have the same meaning as the PSX mirrors, 0=unit ok
	 * and up-to-date, 0xff=unit bad, all blocks out of date.
	 */
	u_char	bmap0;
	u_char	bmap1;
};

#define mioc_label mirror_label

#define ML_SIZE (sizeof (struct mirror_label)) /* size of mirror label */
#define ML_BLKS ((ML_SIZE + DEV_BSIZE - 1)/DEV_BSIZE)
#define ML_BSIZE (DEV_BSIZE * ML_BLKS)
#define ML_BLOCK ((daddr_t)0)			  /* mirror at block 0 */
#define ML_ADDRALIGN 16			  /* required alignment of buffer */
#define MIRROR_MAGIC ((unsigned)0xcd69e24f) 	/* M-M i M-r O */
#define M_BMAP_DENSITY ((unsigned)0xffffffff) 	/* Dynix 3.0 only */

#define b_sortkey b_resid		  /* sort key for disksort */

struct mioc {
	unsigned		limit;	  /* number of mirrors */
	struct mioc_info	mirror;
	struct mioc_unit	unit[UPM];
	struct mioc_label	label;
};
