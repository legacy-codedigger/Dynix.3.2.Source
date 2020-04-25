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
static char rcsid[]= "$Header: conf_zd.c 1.23 90/09/02 $";
#endif

/*
 * Binary configuration information for standalone ZDC disk controller.
 */
#include <sys/types.h>
#include <sys/vtoc.h>
#include <zdc/zdc.h>
#include "zdc.h"

#ifndef NUMPARTS
#define NUMPARTS 8
#endif 

#ifndef VTOC
/*
 * Cylinder offsets for ZDC disk partitions
 *
 * Indexed by drive type.
 *
 * Note that this list is order dependent. New entries *must*
 * correspond with the drive type.
 */
short	cyl_offsets[][NUMPARTS] = {
	{ 335, 360, 1, 1, 411, 1, 462, 1}, 	/* 0 - Fujitsu M2333K */
	{ 366, 384, 1, 1, 420, 1, 457, 1}, 	/* 1 - Fujitsu M2351A (eagle) */
	{ 282, 292, 1, 1, 311, 1, 330, 1}, 	/* 2 - Fujitsu M2344K */
	{ 348, 356, 1, 1, 372, 1, 388, 1}, 	/* 3 - Fujitsu M2382K */
	{ 640, 657, 1, 1, 690, 1, 723, 1}, 	/* 4 - CDC 9720-850 */
	{ 931, 943, 1, 1, 957, 1, 984, 1}, 	/* 5 - Fujitsu M2392K */
	{ -1, -1, -1, -1, -1, -1, -1, -1}, 	/* 6 - Reserved by Sequent */
	{ -1, -1, -1, -1, -1, -1, -1, -1}, 	/* 7 - Reserved by Sequent */
	{ -1, -1, -1, -1, -1, -1, -1, -1}, 	/* 8 - Reserved by Sequent */
	{ -1, -1, -1, -1, -1, -1, -1, -1}, 	/* 9 - Reserved by Sequent */
	{ -1, -1, -1, -1, -1, -1, -1, -1}, 	/* 10 - Reserved by Sequent */
	{ -1, -1, -1, -1, -1, -1, -1, -1}, 	/* 11 - Reserved by Sequent */
	{ -1, -1, -1, -1, -1, -1, -1, -1}, 	/* 12 - Reserved by Sequent */
	{ -1, -1, -1, -1, -1, -1, -1, -1}, 	/* 13 - Reserved by Sequent */
	{ -1, -1, -1, -1, -1, -1, -1, -1}, 	/* 14 - Reserved by Sequent */
	{ -1, -1, -1, -1, -1, -1, -1, -1}, 	/* 15 - Reserved by Sequent */
	{ 292, 313, 1, 1, 355, 1, 396, 1}, 	/* 16 - CDC 9715-340 */
	{ 474, 486, 1, 1, 512, 1, 536, 1}, 	/* 17 - CDC 9771-800 */
	{ -1, -1, -1, -1, -1, -1, -1, -1}, 	/* 18 - Reserved by Sequent */
	{ -1, -1, -1, -1, -1, -1, -1, -1}, 	/* 19 - Reserved by Sequent */
        { 769, 785, 1, 1, 817, 1, 840, 1},      /* 20 - CDC 9720-1230 */
        { 769, 785, 1, 1, 817, 1, 840, 1},      /* 21 - SABRE5-1230 */
};
#endif

#ifndef BOOTXX
/*
 * zdinfo table for drives.
 * 1 entry per drive type.
 */
struct zdinfo zdinfo[] = {
	{ "M2333K",				/* 0 - Fujitsu M2333K */
	  {0x6b62776e,			((1 << 7) | 6), 1, 66, 10,
	   823, ZDT_M2333K, 20,		 90, ((112 << 8) + 44),
	    0,  1, 4, 20,		1, 37,  610,
	   11, 34, 0,  0,		0,
	   0,				0,
		{    0,    0, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF, 0xFF,
		  0xEE, 0xCF, 0xAF, 0xEB,	0xE7, 0xE7, 0x9B,    6,
		     0,    2,    0,    0,	0xC9,    0,    0,    0,
		     0,    0,       0,		 	    0,
		     3,   21,    0,    1,	0x01, 0x01, 0x01, 0x01,
		  0x01, 0x05,    0,    0, 	   1,   16,    0,    1,
		  0x00, 0x00,    0, 0x19,	  34, 0xDC, 0x01, 0x04,
		      512,    0x00, 0x5A,	0x00, 0x00,    0, 0x19
		},
	  }
	},
	{ "M2351A",				/* 1 - Fujitsu M2351A (Eagle) */
	  {0x6b62776e,			((1 << 7) | 4), 1, 46, 20,
	   842, ZDT_M2351A, 15,		101, ((112 << 8) + 44),
	   15,  1, 5, 16,		1, 37,  597,
	   11, 23, 0,  0,		0,
	   0,				0,
		{    0,    0, 0xFF, 0xFF,	0x00, 0x00, 0xFF, 0xFF,
		  0xBA, 0xFB, 0xFF, 0xFF,	0xF5, 0xEB, 0x95,    6,
		     0,    2,    0,    0,	0xC9,    0,    0,    0,
		     0,    0,       0,		 	    0,
		     3,   22,    0,    1,	0x01, 0x01, 0x01, 0x01,
		  0x01, 0x05,    0,    0, 	   3,   15,    0,    1,
		  0x00, 0x00,    0, 0x19,	  23, 0x9C, 0x01, 0x04,
		      512,    0x00, 0x5A,	0x00, 0x00,    0, 0x19
		},
	  }
	},
	{ "M2344K",				/* 2 - Fujitsu M2344K */
	  {0x6b62776e,			((1 << 7) | 6), 1, 66, 27,
	   624, ZDT_M2344K, 20,		 90, ((112 << 8) + 44),
	    0,  1, 4, 16,		1, 37,  610,
	   11, 34, 0,  0,		0,
	   0,				0,
		{    0,    0, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF, 0xFF,
		  0xEE, 0xCF, 0xAF, 0xEB,	0xE7, 0xE7, 0x9B,    6,
		     0,    2,    0,    0,	0xC9,    0,    0,    0,
		     0,    0,       0,		 	    0,
		     3,   21,    0,    1,	0x01, 0x01, 0x01, 0x01,
		  0x01, 0x05,    0,    0, 	   1,   16,    0,    1,
		  0x00, 0x00,    0, 0x19,	  34, 0xDC, 0x01, 0x04,
		      512,    0x00, 0x5A,	0x00, 0x00,    0, 0x19
		},
	  }
	},
	{ "M2382K",				/* 3 - Fujitsu M2382K */
	  {0x6b62776e,			((1 << 7) | 6), 1, 81, 27,
	   745, ZDT_M2382K, 24,	 200, ((112 << 8) + 0),
	    0,  2, 2, 20,		1, 31,  604,
	    5, 34, 0,  0,		0,
	   0,				0,
		{    0,    0, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF, 0xFF,
		  0xEE, 0xCF, 0xAF, 0xEB,	0xE7, 0xE7, 0x9B,    6,
		     0,    2,    0,    0,	0xC9,    0,    0,    0,
		     0,    0,       0,		 	    0,
		     3,   17,    0,    1,	0x01, 0x01, 0x01, 0x01,
		  0x01, 0x05,    0,    0, 	   1,   18,    0,    1,
		  0x00, 0x00,    0, 0x19,	  35, 0xDC, 0x01, 0x04,
		      512,    0x00, 0x5A,	0x00, 0x00,    0, 0x19
		},
	  }
	},
	{ "CDC9720-850",			/* 4 - CDC 9720, 850 MB */
	  {0x6b62776e,			  ((1 << 7) | 6), 1, 68, 15,
	   1381, ZDT_CDC9720_850, 20,	 102, ((64 << 8) + 40),
	    0,  2, 2, 21,		1, 35,  594,
	    9, 27, 0,  0,		0,
	   0,				0,
		{    0,    0, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF, 0xFF,
		  0xEE, 0xCF, 0xAF, 0xEB,	0xE7, 0xE7, 0x9B,    6,
		     0,    2,    0,    0,	0xC9,    0,    0,    0,
		     0,    0,       0,		 	    0,
		     1,   21,    0,    1,	0x01, 0x01, 0x01, 0x01,
		  0x01, 0x05,    0,    0, 	   1,   13,    0,    1,
		  0x00, 0x00,    0, 0x19,	  28, 0xDC, 0x01, 0x04,
		      512,    0x00, 0x5A,	0x00, 0x00,    0, 0x19
		},
	  }
	},
	{ "M2392K",				/* 5 - Fujitsu M2392K */
	  {0x6b62776e,			((1 << 7) | 6), 1, 81, 21,
	   1916, ZDT_M2392K, 24,	 216, ((112 << 8) + 0),
	    0,  4, 4, 10,		1, 31,  612,
	    0, 39, 0,  0,		0,
	   0,				0,
		{    0,    0, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF, 0xFF,
		  0xEE, 0xCF, 0xAF, 0xEB,	0xE7, 0xE7, 0x9B,    6,
		     0,    2,    0,    0,	0xC9,    0,    0,    0,
		     0,    0,       0,		 	    0,
		     3,   17,    0,    1,	0x01, 0x01, 0x01, 0x01,
		  0x01, 0x05,    0,    0, 	   2,   19,    0,    1,
		  0x00, 0x00,    0, 0x19,	  40, 0xDC, 0x01, 0x04,
		      512,    0x00, 0x5A,	0x00, 0x00,    0, 0x19
		},
	  }
	},
	{ "Reserved",				/* 6 - Reserved by Sequent */
	  { 0 }
	},
	{ "Reserved",				/* 7 - Reserved by Sequent */
	  { 0 }
	},
	{ "Reserved",				/* 8 - Reserved by Sequent */
	  { 0 }
	},
	{ "Reserved",				/* 9 - Reserved by Sequent */
	  { 0 }
	},
	{ "Reserved",				/* 10 - Reserved by Sequent */
	  { 0 }
	},
	{ "Reserved",				/* 11 - Reserved by Sequent */
	  { 0 }
	},
	{ "Reserved",				/* 12 - Reserved by Sequent */
	  { 0 }
	},
	{ "Reserved",				/* 13 - Reserved by Sequent */
	  { 0 }
	},
	{ "Reserved",				/* 14 - Reserved by Sequent */
	  { 0 }
	},
	{ "Reserved",				/* 15 - Reserved by Sequent */
	  { 0 }
	},
	{ "CDC9715-340",			/* 16 - CDC 9715, 340 MB */
	  {0x6b62776e,			  6, 1, 34, 24,
	   711, ZDT_CDC9715_340, 10,	 0, ((88 << 8) + 48),
	    8,  1, 4, 15,		1, 40,  576,
	   14, 9, 0,  0,		0,
	   0,				0,
		{    0,    0, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF, 0xFF,
		  0xEE, 0xCF, 0xAF, 0xEB,	0xE7, 0xE7, 0x9B,    6,
		     0,    2,    0,    0,	0xC9,    0,    0,    0,
		     0,    0,       0,		 	    0,
		     1,   25,    0,    1,	0x01, 0x01, 0x01, 0x01,
		  0x01, 0x05,    0,    0, 	   1,   12,    0,    1,
		  0x00, 0x00,    0, 0x19,	  10, 0xDC, 0x01, 0x04,
		      512,    0x00, 0x5A,	0x00, 0x00,    0, 0x19
		},
	  }
	},
	{ "CDC9771-800",			/* 17 - CDC 9771, 800 MB */
	  {0x6b62776e,			((1 << 7) | 6), 1, 85, 16,
	   1024, ZDT_CDC9771_800, 15,	 90, ((88 << 8) + 48),
	   19,  1, 4, 16,		1, 40,  585,
	   14, 17, 0,  0,		0,
	   0,				0,
		{    0,    0, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF, 0xFF,
		  0xEE, 0xCF, 0xAF, 0xEB,	0xE7, 0xE7, 0x9B,    6,
		     0,    2,    0,    0,	0xC9,    0,    0,    0,
		     0,    0,       0,		 	    0,
		     1,   25,    0,    1,	0x01, 0x01, 0x01, 0x01,
		  0x01, 0x05,    0,    0, 	   1,   12,    0,    1,
		  0x00, 0x00,    0, 0x19,	  19, 0xDC, 0x01, 0x04,
		      512,    0x00, 0x5A,	0x00, 0x00,    0, 0x19
		},
	  }
	},
	{ "Reserved",				/* 18 - Reserved by Sequent */
	  { 0 }
	},
	{ "Reserved",				/* 19 - Reserved by Sequent */
	  { 0 }
	},
	{ "CDC9720-1230",			/* 20 - CDC 9720, 1230 MB */
	  {0x6b62776e,			((1 << 7) | 6), 1, 83, 15,
	   1635, ZDT_CDC9720_1230, 24,	 252, ((64 << 8) + 52),
	   0,  2, 5, 26,		1, 40,  597,
	   9, 30, 0,  0,		0,
	   0,				0,
		{    0,    0, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF, 0xFF,
		  0xEE, 0xCF, 0xAF, 0xEB,	0xE7, 0xE7, 0x9B,    6,
		     0,    2,    0,    0,	0xC9,    0,    0,    0,
		     0,    0,       0,		 	    0,
		     1,   21,    0,    1,	0x01, 0x01, 0x01, 0x01,
		  0x01, 0x05,    0,    0, 	   1,   10,    0,    1,
		  0x00, 0x00,    0, 0x19,	  31, 0xDC, 0x01, 0x04,
		      512,    0x00, 0x5A,	0x00, 0x00,    0, 0x19
		},
	  }
	},
	{ "SABRE5-1230",			/* 21 - SABRE5, 1230 MB new */
	  {0x6b62776e,			((1 << 7) | 6), 1, 83, 15,
	   1635, ZDT_SABRE5_1230, 24,	 252, ((64 << 8) + 40),
	   0,  2, 2, 26,		1, 35,  597,
	   9, 31, 0,  0,		0,
	   0,				0,
		{    0,    0, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF, 0xFF,
		  0xEE, 0xCF, 0xAF, 0xEB,	0xE7, 0xE7, 0x9B,    6,
		     0,    2,    0,    0,	0xC9,    0,    0,    0,
		     0,    0,       0,		 	    0,
		     1,   21,    0,    1,	0x01, 0x01, 0x01, 0x01,
		  0x01, 0x05,    0,    0, 	   1,   13,    0,    1,
		  0x00, 0x00,    0, 0x19,	  31, 0xDC, 0x01, 0x04,
		      512,    0x00, 0x5A,	0x00, 0x00,    0, 0x19
		},
	  }
	},
};
#endif

#ifdef VTOC
int	nzdtypes = 0;
#else
/* number of valid drive types */
int	nzdtypes = sizeof(cyl_offsets) / sizeof(cyl_offsets[0]);
#endif /* VTOC */

#ifndef BOOTXX
/*
 * When creating a VTOC from the formatter, a single partition is made
 * available to load the miniroot on.  These are suggestions for
 * what that partition should be, indexed by drive type.  Note that
 * this will be used to create partition "1".
 */

struct partition zd_proto[] = {
	{	270600,	 67320, V_RAW, 8192, 1024 },	/* 0 - m2333k */
	{	386400,	192280, V_RAW, 8192, 1024 },	/* 1 - m2351a */
	{	552420, 204930, V_RAW, 8192, 1024 },	/* 2 - m2344k */
	{	811377, 205578, V_RAW, 8192, 1024 },	/* 3 - m2382k */
	{       626280, 205020, V_RAW, 8192, 1024 }, 	/* 4 - CDC 9720-850 */
	{	1551312,207522, V_RAW, 8192, 1024 },	/* 5 - m2392k */
	{	     0,      0, V_RAW, 8192, 1024 },	/* 6 - Reserved */
	{	     0,      0, V_RAW, 8192, 1024 },	/* 7 - Reserved */
	{	     0,      0, V_RAW, 8192, 1024 },	/* 8 - Reserved */
	{	     0,      0, V_RAW, 8192, 1024 },	/* 9 - Reserved */
	{	     0,      0, V_RAW, 8192, 1024 },	/* 10 - Reserved */
	{	     0,      0, V_RAW, 8192, 1024 },	/* 11 - Reserved */
	{	     0,      0, V_RAW, 8192, 1024 },	/* 12 - Reserved */
	{	     0,      0, V_RAW, 8192, 1024 },	/* 13 - Reserved */
	{	     0,      0, V_RAW, 8192, 1024 },	/* 14 - Reserved */
	{	     0,      0, V_RAW, 8192, 1024 },	/* 15 - Reserved */
	{	     0,      0, V_RAW, 8192, 1024 },	/* 16 - Reserved */
	{	     0,      0, V_RAW, 8192, 1024 },	/* 17 - Reserved */
	{	     0,      0, V_RAW, 8192, 1024 },	/* 18 - Reserved */
	{	     0,      0, V_RAW, 8192, 1024 },	/* 19 - Reserved */
        {       939975, 205425, V_RAW, 8192, 1024 },    /* 20 - CDC 9720-1230 */
        {       939975, 205425, V_RAW, 8192, 1024 },    /* 21 - SABRE5-1230 */
};
#endif

/*
 * Time to wait for ZDC fw to initialize.
 * Includes time to powerup drives.  Should be >= 60 secs.
 */
#ifdef	i386
int	zdcinitime	= 2400000;
#endif	i386
#ifdef	ns32000
int	zdcinitime	= 1000000;
#endif	ns32000

/*
 * polled command timeout, should be approx. 30 secs.
 */
#ifdef	i386
int	zdccmdtime	= 30000000;
#endif	i386
#ifdef	ns32000
int	zdccmdtime	= 6250000;
#endif	ns32000

int	zdcretry = 10;			/* error retry count */
u_char	zdctrl = ZDC_DUMPONPANIC;		/* ZDC_INIT control bits */
