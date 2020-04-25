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
 * $Header: types.h 1.3 89/07/17 $
 */

/*
 * $Log:	types.h,v $
 */

typedef	int	logical_t;
typedef	int	grp_t;

/* SMB specific */
typedef	unsigned char	BYTE;
typedef	unsigned short	WORD;

/* Logicals */
#define	TRUE	1
#define	FALSE	0
#define	OK	TRUE
#define	FAIL	FALSE

/* Data Identifer Bytes */
#define	DIB_DBLK	1
#define	DIB_DLCT	2
#define	DIB_PATH	3
#define	DIB_ASCII	4
#define	DIB_VBLK	5

/* command dib_str() argument */

#define DIBmkdir	1
#define DIBrmdir	1
#define DIBopen		1
#define DIBcreate	1
#define DIBclose	0
#define DIBflush	0
#define DIBunlink	1
#define DIBmv		2
#define DIBgetatr	1
#define DIBsetatr	1
#define DIBread		0
#define DIBwrite	0
#define DIBlock		0
#define DIBunlock	0
#define DIBctemp	1
#define DIBmknew	1
#define DIBchkpth	1
#define DIBexit		0
#define DIBlseek	0
#define DIBtcon		3
#define DIBtdis		0
#define DIBnegprot	16	/* variable */
#define DIBdskattr	0
#define DIBsearch	2
#define DIBsplopen	1
#define DIBsplwr	1
#define DIBsplclose	0
#define DIBsplretq	0

#ifdef	lint
extern WORD mtohw();
extern WORD htomw();
#else
/* 
 *	Macros to convert WORDS
 *	from message to host byte order (and back)
 */
# if defined(ns32000) || defined(i386) || defined(xyz_processor)
#   define mtohw(x)	(x)
#   define htomw(x)	(x)
# else
    ERROR: machine type not specified!
# endif
#endif	lint
