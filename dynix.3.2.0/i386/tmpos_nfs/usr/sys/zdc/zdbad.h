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
 * $Header: zdbad.h 1.1 86/04/16 $
 *
 * zdbad.h
 *	ZDC bad block list and MFG defect list structure definitions
 */

/* $Log:	zdbad.h,v $
 */

#define	BZ_NBADCOPY	5		/* number of bad block list copies */

/*
 * structure of bad block list
 */

struct zdbad {
	long	bz_csn;		/* bad block list checksum */
	u_short	bz_nelem;	/* nbr of elements in list */
	u_short	bz_nsnf;	/* nbr of BZ_SNF entries in list */
	struct	bz_bad {
		struct	bz_diskaddr {
			u_char	bd_sect;	/* sector */
			u_char	bd_head;	/* head */
			u_short	bd_cyl   : 13,	/* cylinder */
				bd_rtype :  2,	/* replacement type */
				bd_ftype :  1;	/* failure type */
		} bz_badaddr;			/* Bad disk address */
		struct diskaddr bz_rpladdr;	/* Replacement disk address */
	} bz_bad[1];		/* size is disk type dependent */
};

#define bz_cyl		bz_badaddr.bd_cyl
#define bz_head		bz_badaddr.bd_head
#define bz_sect		bz_badaddr.bd_sect
#define bz_rtype	bz_badaddr.bd_rtype
#define bz_ftype	bz_badaddr.bd_ftype

/*
 * Replacement types
 */
#define BZ_PHYS		0x0		/* Physical disk address */
#define BZ_AUTOREVECT	0x1		/* Autorevector for replacement */
#define BZ_SNF		0x2		/* SNF - driver must revector */ 

/*
 * Failure types
 */
#define BZ_BADHEAD	0x0
#define BZ_BADDATA	0x1

/*
 * Structure of manufacturer's defect list.
 */

struct bad_mfg {
	long	bm_csn;		/* mfg's defect list checksum */
	u_short	bm_nelem;	/* number of entries in bm_mfgbad */

	/* defect location */
	struct bm_mfgbad {
		u_short	bm_cyl;		/* cylinder */
		u_short	bm_len;		/* length of defect */
		u_int	bm_pos  : 24,	/* bytes from index */
			bm_head :  8;	/* head */
	} bm_mfgbad[1];		/* size is disk type dependent */
};
