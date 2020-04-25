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

/* $Header: mbad.h 2.3 87/02/13 $ */

/*
 * definitions for stand-alone access to multibus adaptor
 */

/*
 * Machine dependent constants
 */
#ifdef i386
# ifdef KXX
#define MB_IOSPACE 0x01CC0000		/* 28M + 3/4M */
# else SGS_HW
#define MB_IOSPACE 0x81CC0000		/* 2G + 28M + 3/4M */
# endif KXX
#endif i386
#ifdef ns32000
#define	MB_IOSPACE 0x008C0000		/* 8M + 3/4M */
#endif ns32000
#define	MB_IODELTA 0x00100000		/* difference between MBAd's */
#define	MB_RAMBASE 0x000C0000

#define	K	1024
struct mb_ios {
	unsigned char	mb_io	[64*K];		/* Multi-bus IO addresses */
	unsigned char	mb_res1 [64*K];		/* Reserved */
	unsigned int 	mb_map	[256];		/* DMA Mapping registers */
	unsigned int 	mb_res2 [(64*K/4)-256];	/* Reserved */
	unsigned char	mb_ctl	[1];		/* MBif control register */
	unsigned char	mb_res3 [64*K-1];	/* Reserved */
};
