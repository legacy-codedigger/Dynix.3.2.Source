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

/* $Id: manifest.h,v 2.8 88/09/02 11:46:17 ksb Exp $ */
/*
 *
 *	Berkeley Pascal Compiler	(manifest.h)
 */

#if !defined(_MANIFEST_)
#define	_MANIFEST_

#include <stdio.h>
#include "pcclocal.h"
#include "config.h"

#define DSIZE	(MAXOP+1)	/* DSIZE is the size of the dope array */

/*
 * Node types
 */
#define LTYPE	00002		/* leaf */
#define UTYPE	00004		/* unary */
#define BITYPE	00010		/* binary */

/*
 * Bogus type values
 */
#define TNULL	PTR		/* pointer to UNDEF */
#define TVOID	FTN		/* function returning UNDEF (for void) */

/*
 * Type packing constants
 */
#define TMASK	00060		/* mask for 1st component of compound type */
#define TMASK1	00300		/* mask for 2nd component of compound type */
#define TMASK2	00360		/* mask for 3rd component of compound type */
#define BTMASK	00017		/* basic type mask */
#define BTSHIFT	4		/* basic type shift */
#define TSHIFT	2		/* shift count to get next type component */

/*
 * Type manipulation macros
 */
#define MODTYPE(x,y)	x = ((x)&(~BTMASK))|(y)	/* set basic type of x to y */
#define BTYPE(x)	((x)&BTMASK)		/* basic type of x */
#define ISUNSIGNED(x)	((x)<=ULONG&&(x)>=UCHAR)
#define UNSIGNABLE(x)	((x)<=LONG&&(x)>=CHAR)
#define ENUNSIGN(x)	((x)+(UNSIGNED-INT))
#define DEUNSIGN(x)	((x)+(INT-UNSIGNED))
#define ISPTR(x)	(((x)&TMASK)==PTR)
#define ISFTN(x)	(((x)&TMASK)==FTN)	/* is x a function type */
#define ISARY(x)	(((x)&TMASK)==ARY)	/* is x an array type */
#define INCREF(x)	((((x)&~BTMASK)<<TSHIFT)|PTR|((x)&BTMASK))
#define DECREF(x)	((((x)>>TSHIFT)&~BTMASK)|( (x)&BTMASK))
/* advance x to a multiple of y */
#define SETOFF(x,y)	if ((x)%(y) != 0) (x) = (((x)/(y) + 1) * (y))
/* can y bits be added to x without overflowing z */
#define NOFIT(x,y,z)	(((x)%(z) + (y)) > (z))

/*
 * Pack and unpack field descriptors (size and offset)
 */
#define PKFIELD(s,o)	(((o)<<6)| (s))
#define UPKFSZ(v)	((v) &077)
#define UPKFOFF(v)	((v)>>6)

/*
 * Operator information
 */
#define TYFLG	 000016
#define ASGFLG	 000001
#define LOGFLG	 000020
#define SIMPFLG	 000040
#define COMMFLG	 000100
#define DIVFLG	 000200
#define FLOFLG	 000400
#define LTYFLG	 001000
#define CALLFLG	 002000
#define MULFLG	 004000
#define SHFFLG	 010000
#define ASGOPFLG 020000
#define SPFLG	 040000

#define optype(o)	(dope[o]&TYFLG)
#define asgop(o)	(dope[o]&ASGFLG)
#define logop(o)	(dope[o]&LOGFLG)
#define callop(o)	(dope[o]&CALLFLG)

/*
 * External declarations, typedefs and the like
 */
#if defined(FLEXNAMES)
char	*hash();
char	*tstr();
extern	int tstrused;
extern	char *tstrbuf[];
extern	char **curtstr;
#define	freetstr()	curtstr = tstrbuf, tstrused = 0
#endif

extern	int nerrors;		/* number of errors seen so far */
extern	int dope[];		/* a vector containing operator information */
extern	char *opst[];		/* a vector containing names for ops */

typedef	union ndu NODE;
typedef	unsigned int TWORD;
#define NIL	(NODE *)0

#if !defined(EXPR)
#define EXPR '.'
#endif
#if !defined(BBEG)
#define BBEG '['
#endif
#if !defined(BEND)
#define BEND ']'
#endif

#endif /* no multiple includes of this file */
