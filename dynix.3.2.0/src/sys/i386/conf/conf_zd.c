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
static	char	rcsid[] = "$Header: conf_zd.c 1.17 90/09/02 $";
#endif

/*
 * configuration of disks on the ZDC
 *
 * WARNING: If the tables are partially filled in unpredictable results
 *	    *WILL* occur.
 */

/* $Log:	conf_zd.c,v $
 */
#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/buf.h"
#include "../h/time.h"
#include "../h/vtoc.h"
#include "../zdc/zdc.h"

/*
 *	Part A.	Partition tables.
 *
 * On all drives, the first cylinder is reserved for disk description data
 * and the last two cylinders are reserved for diagnostics. No partition
 * which will contain a filesystem should include any of these cylinders.
 *
 * N.B.: The stand-alone driver knows these offsets.
 *
 * The zdparts table is indexed by drive type.
 * Note that this list is order dependent. New entries *must*
 * correspond with the drive type.
 *
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
 */

struct cmptsize m2333k[NUMPARTS] = {	/* Fujitsu M2333K (swallow) */
	335*66*10,	 25*66*10,	/* A=cyl 335 thru 359 */
	360*66*10,	102*66*10,	/* B=cyl 360 thru 461 */
	  1*66*10,	820*66*10,	/* C=cyl   1 thru 820 */
	  1*66*10,	410*66*10,	/* D=cyl   1 thru 410 */
	411*66*10,	410*66*10,	/* E=cyl 411 thru 820 */
	  1*66*10,	359*66*10,	/* F=cyl   1 thru 359 */
	462*66*10,	359*66*10,	/* G=cyl 462 thru 820 */
	  1*66*10,	334*66*10,	/* H=cyl   1 thru 334 */
};

struct cmptsize m2351a[NUMPARTS] = {	/* Fujitsu M2351A (Eagle) */
	366*46*20,	  18*46*20,	/* A=cyl 366 thru 383 */
	384*46*20,	  73*46*20,	/* B=cyl 384 thru 456 */
	  1*46*20,	 839*46*20,	/* C=cyl   1 thru 839 */
	  1*46*20,	 419*46*20,	/* D=cyl   1 thru 419 */
	420*46*20,	 419*46*20,	/* E=cyl 420 thru 838 */
	  1*46*20,	 383*46*20,	/* F=cyl   1 thru 383 */
	457*46*20,	 383*46*20,	/* G=cyl 457 thru 839 */
	  1*46*20,	 365*46*20,	/* H=cyl   1 thru 365 */
};

struct cmptsize m2344k[NUMPARTS] = {	/* Fujitsu M2344K (swallow 4) */
	282*66*27, 	 10*66*27,	/* A=cyl 282 thru 291 */
	292*66*27,	 38*66*27,	/* B=cyl 292 thru 329 */
	  1*66*27,	621*66*27,	/* C=cyl   1 thru 621 */
	  1*66*27,	310*66*27,	/* D=cyl   1 thru 310 */
	311*66*27,	311*66*27,	/* E=cyl 311 thru 621 */
	  1*66*27,	291*66*27,	/* F=cyl   1 thru 291 */
	330*66*27,	292*66*27,	/* G=cyl 330 thru 621 */
	  1*66*27,	281*66*27,	/* H=cyl   1 thru 281 */
};

struct cmptsize m2382k[NUMPARTS] = {	/* Fujitsu M2382K (swallow 5) */
	348*81*27,	  8*81*27,	/* A=cyl 348 thru 355 */
	356*81*27,	 32*81*27,	/* B=cyl 356 thru 387 */
	  1*81*27,	742*81*27,	/* C=cyl   1 thru 742 */
	  1*81*27,	371*81*27,	/* D=cyl   1 thru 371 */
	372*81*27,	371*81*27,	/* E=cyl 372 thru 742 */
	  1*81*27,	355*81*27,	/* F=cyl   1 thru 355 */
	388*81*27,	355*81*27,	/* G=cyl 388 thru 742 */
	  1*81*27,	347*81*27,	/* H=cyl   1 thru 347 */
};

struct cmptsize m2392k[NUMPARTS] = {	/* Fujitsu M2392K (swallow 6) */
         931*81*21,	12*81*21,	/* A=cyl 931 thru 942 */
         943*81*21,	41*81*21,	/* B=cyl 943 thru 983 */
         1*81*21,	1913*81*21,	/* C=cyl   1 thru 1913 */
         1*81*21, 	956*81*21,	/* D=cyl   1 thru 956 */
         957*81*21,	957*81*21,	/* E=cyl 957 thru 1913 */
         1*81*21,	942*81*21,	/* F=cyl   1 thru 942 */
         984*81*21,	930*81*21,	/* G=cyl 984 thru 1913 */
         1*81*21,	930*81*21,	/* H=cyl   1 thru 930 */
};

struct cmptsize cdc9715_340[NUMPARTS] = {	/* CDC 9715-340 (FSD) */
	292*34*24,	 21*34*24,	/* A=cyl 292 thru 312 */
	313*34*24,	 83*34*24,	/* B=cyl 313 thru 395 */
	  1*34*24,	708*34*24,	/* C=cyl   1 thru 708 */
	  1*34*24,	354*34*24,	/* D=cyl   1 thru 354 */
	355*34*24,	354*34*24,	/* E=cyl 355 thru 708 */
	  1*34*24,	312*34*24,	/* F=cyl   1 thru 312 */
	396*34*24,	313*34*24,	/* G=cyl 396 thru 708 */
	  1*34*24,	291*34*24,	/* H=cyl   1 thru 291 */
};

struct cmptsize cdc9771_800[NUMPARTS] = {	/* CDC 9771-800 (XMD) */
	474*85*16,	 12*85*16,	/* A=cyl 474 thru 485 */
	486*85*16,	 50*85*16,	/* B=cyl 486 thru 535 */
	  1*85*16,     1021*85*16,	/* C=cyl   1 thru 1021 */
	  1*85*16,	511*85*16,	/* D=cyl   1 thru 511 */
	512*85*16,	510*85*16,	/* E=cyl 512 thru 1021 */
	  1*85*16,	485*85*16,	/* F=cyl   1 thru 485 */
	536*85*16,	486*85*16,	/* G=cyl 536 thru 1021 */
	  1*85*16,	473*85*16,	/* H=cyl   1 thru 473 */
};

struct cmptsize cdc9720_850[NUMPARTS] = {	/* CDC 9720-850 (EMD) */
	640*68*15,	 17*68*15,	/* A=cyl 640 thru 656 */
	657*68*15,	 66*68*15,	/* B=cyl 657 thru 722 */
	  1*68*15,     1378*68*15,	/* C=cyl   1 thru 1378 */
	  1*68*15,	689*68*15,	/* D=cyl   1 thru 689 */
	690*68*15,	689*68*15,	/* E=cyl 690 thru 1378 */
	  1*68*15,	656*68*15,	/* F=cyl   1 thru 656 */
	723*68*15, 	656*68*15,	/* G=cyl 723 thru 1378 */
	  1*68*15,	639*68*15,	/* H=cyl   1 thru 639 */
};

struct cmptsize cdc9720_1230[NUMPARTS] = { /* CDC 9720-1230 (EMD) */
	769*83*15,	  16*83*15,	/* A=cyl 769 thru 784 */
 	785*83*15,	  55*83*15,	/* B=cyl 785 thru 839 */
          1*83*15,	1632*83*15,	/* C=cyl   1 thru 1632 */
 	  1*83*15,	 816*83*15,	/* D=cyl   1 thru 816 */
 	817*83*15,	 816*83*15,	/* E=cyl 817 thru 1632 */
 	  1*83*15,	 784*83*15,	/* F=cyl   1 thru 784 */
 	840*83*15,	 793*83*15,	/* G=cyl 840 thru 1632 */
 	  1*83*15,	 768*83*15,	/* H=cyl   1 thru 768 */
};

struct cmptsize sabre5_1230[NUMPARTS] = { /* SABRE5-1230 (EMD) new format */
	769*83*15,	  16*83*15,	/* A=cyl 769 thru 784 */
 	785*83*15,	  55*83*15,	/* B=cyl 785 thru 839 */
          1*83*15,	1632*83*15,	/* C=cyl   1 thru 1632 */
 	  1*83*15,	 816*83*15,	/* D=cyl   1 thru 816 */
 	817*83*15,	 816*83*15,	/* E=cyl 817 thru 1632 */
 	  1*83*15,	 784*83*15,	/* F=cyl   1 thru 784 */
 	840*83*15,	 793*83*15,	/* G=cyl 840 thru 1632 */
 	  1*83*15,	 768*83*15,	/* H=cyl   1 thru 768 */
};

/*
 * A null cmptsize array for Reserved drive types
 * Done this way since driver already checks for a partition
 * length of zero.  Otherwise additonal code to check
 * for NULL pointer in zdparts[] would be required.
 */
struct cmptsize zd_nulltype[NUMPARTS] = {	/* Used for reserved types */
	0,	0,
	0,	0,
	0,	0,
	0,	0,
	0,	0,
	0,	0,
	0,	0,
	0,	0,
};

struct cmptsize *zdparts[] = {
	m2333k,			/* 0 - Fujitsu M2333K (swallow) */
	m2351a,			/* 1 - Fujitsu M2351A (Eagle) */
	m2344k,			/* 2 - Fujitsu M2344K (swallow 4) */
	m2382k,			/* 3 - Fujitsu M2382K (swallow 5) */
	cdc9720_850,		/* 4 - CDC 9720-850 (EMD) */
	m2392k,			/* 5 - Fujitsu M2392K (swallow 6) */
	zd_nulltype,		/* 6 - Reserved */
	zd_nulltype,		/* 7 - Reserved */
	zd_nulltype,		/* 8 - Reserved */
	zd_nulltype,		/* 9 - Reserved */
	zd_nulltype,		/* 10 - Reserved */
	zd_nulltype,		/* 11 - Reserved */
	zd_nulltype,		/* 12 - Reserved */
	zd_nulltype,		/* 13 - Reserved */
	zd_nulltype,		/* 14 - Reserved */
	zd_nulltype,		/* 15 - Reserved */
	cdc9715_340,		/* 16 - CDC 9715-340 MB (FSD) */
	cdc9771_800,		/* 17 - CDC 9771-800 MB (XMD) */
	zd_nulltype,		/* 18 - Reserved */
	zd_nulltype,		/* 19 - Reserved */
	cdc9720_1230,           /* 20 - CDC 9720-1230 MB (EMD) */
	sabre5_1230,            /* 21 - SABRE5-1230 MB (EMD) new format */
};
int	zdntypes	= sizeof (zdparts) / sizeof(struct cmptsize *);

/*
 *	Part B. Global Information.
 */

/*
 * If the number of zdc_iovpercb is 0 then the driver will automatically
 * allocate enough iovecs to handle max_RAW_IO (see physio()).
 * If the number of zdc_iovpercb is non-zero, then the value specified
 * is allocated if it is less than max_RAW_IO. If the specified value is
 * greater than max_RAW_IO the number is reduced to handle max_RAW_IO.
 */
int	zdc_iovpercb	= 0;			/* no of iovecs per cb */

int	zdc_err_bin	= 7;			/* Error interrupt bin */
int	zdc_cb_bin	= 5;			/* CB interrupt bin */
gate_t	zdcgate		= 62;			/* gate for zdc locks/semas */
/*
 * polled command timeout, should be approx. 30 secs.
 */
#ifdef i386
unsigned int	zdccmdtime	= 30000000;
#else
unsigned int	zdccmdtime	=  6250000;
#endif
/*
 * controller ready timeout, should be >= 60 secs.
 */
#ifdef i386
unsigned int	zdcinitime	= 2400000;
#else
unsigned int	zdcinitime	= 1000000;
#endif
short	zdcretry	= 10;			/* retry count */
int	zdc_AB_throttle	= 2;			/* Channel A&B DMA throttle */
int	zdc_C_throttle	= 2;			/* Channel C DMA throttle */
u_char	zdctrl = ZDC_DUMPONPANIC;		/* ZDC_INIT control bits */
