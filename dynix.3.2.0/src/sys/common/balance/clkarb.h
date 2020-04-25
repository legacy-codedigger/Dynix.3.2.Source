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
 * $Header: clkarb.h 2.3 87/01/23 $
 */

/*
 * Clock / Arbiter board definitions
 */

/* $Log:	clkarb.h,v $
 */

#define	CLKARB_SLIC	0
#define	CA_FOL		32		/* 1st slot optionally low priority */
#define	CA_LOL		41		/* last slot optionally low priority */

#define	FP_NLIGHTS	48
#define	FP_LIGHTOFF(i)	(wrAddr(CLKARB_SLIC, (fp_lightmap[(i)])))
#define	FP_LIGHTON(i)	(wrAddr(CLKARB_SLIC, (fp_lightmap[(i)] ^ 1)))

#define	FP_IO_INACTIVE	(wrAddr(CLKARB_SLIC, SL_C_IO_ACTIVE - 1))
#define	FP_IO_ACTIVE	(wrAddr(CLKARB_SLIC, SL_C_IO_ACTIVE))

#define	FP_IO_OFFLINE	(wrAddr(CLKARB_SLIC, SL_C_IO_ONLINE - 1))
#define	FP_IO_ONLINE	(wrAddr(CLKARB_SLIC, SL_C_IO_ONLINE))

#define	FP_IO_NOERROR	(wrAddr(CLKARB_SLIC, SL_C_IO_ERROR - 1))
#define	FP_IO_ERROR	(wrAddr(CLKARB_SLIC, SL_C_IO_ERROR))

extern	short	fp_lights;
extern	u_char	fp_lightmap[FP_NLIGHTS];

#ifdef	i386
/*
 * The data-mover on the B21k clock-arbiter board is not used by the SGS kernel
 * since the processor board cache is just as effective at using the bus.
 */
#endif	i386

#ifdef	ns32000
/*
 * Structures and definitions for dealing with data-mover (032 systems only).
 */

struct	cadm_table	{
	u_short		ct_count;	/* ~(8-byte count) */
	u_char		ct_pgszcmd;	/* page-size and command */
	u_char		ct_status;	/* status byte */
	long		ct_swrsvd;	/* reserved for SW use */
	struct	{
		caddr_t	ct_src;		/* source address */
		caddr_t	ct_dest;	/* destination address */
	}		ct_addr[1];	/* open-ended list of addresses */
};

#define	CADM_ALIGN	0x1000		/* tables must be 4k aligned */
#define	CADM_TABLESZ	0x0800		/* each table is 2k */
#define	CADM_DATALIGN	0x7		/* xfr's, counts must be mult of 8 */
#define	CADM_COUNT(c)	~((c) >> 3)	/* count == 1's comp 8-byte count */
#define	CADM_PGSZ	(CLBYTES/2048)	/* page size encoding */
#define	CADM_MOVE	0x00		/* "move" command */
#define	CADM_FILL	0x10		/* "fill" command */
#define	CADM_STATCLR	0x00		/* clear status before start move/fill*/
#define	CADM_STATOK	0xFF		/* good completion */
#define	CADM_STATERR	0xFE		/* access error during move/fill */

#define	CADM_GATE	63		/* gate for coordinating datamover */
#endif	ns32000
