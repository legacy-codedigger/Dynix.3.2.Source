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

/* $Id: macdefs.h,v 2.8 88/09/02 11:46:16 ksb Exp $ */
/*
 *	Berkeley Pascal Compiler	(macdefs.h)
 */

#if !defined(_MACDEFS_)
#define	_MACDEFS_

#define AUTOINIT	0 

/*
 * Storage space requirements
 */
#define SZCHAR		8
#define SZINT		32
#define SZFLOAT		32		/* ??ZZ */
#define SZDOUBLE	64
#define SZLONG		32
#define SZSHORT		16
#define SZPOINT		32

/*
 * Alignment constraints
 */
#define ALCHAR		8
#define ALINT		32
#define ALFLOAT		32
#define ALDOUBLE	32
#define ALLONG		32
#define ALSHORT		16
#define ALPOINT		32
#define ALSTRUCT	8
#define ALSTACK		32

typedef	long	CONSZ;		/* size in which constants are converted */
typedef	long	OFFSZ;		/* size in which offsets are kept */

#define CONFMT	"%ld"		/* format for printing constants */

#define BACKTEMP		/* stack grows negatively for temporaries */
#define RTOLBYTES		/* bytes are numbered from right to left */

#endif	/* no multiple includes of this file */
