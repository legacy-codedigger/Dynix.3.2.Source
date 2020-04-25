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

/* $Header: ex_re.h 2.0 86/01/28 $ */

/* sccs id:	@(#)ex_re.h	7.2	7/26/81  */
/* Copyright (c) 1981 Regents of the University of California */
/*
 * Regular expression definitions.
 * The regular expressions in ex are similar to those in ed,
 * with the addition of the word boundaries from Toronto ed
 * and allowing character classes to have [a-b] as in the shell.
 * The numbers for the nodes below are spaced further apart then
 * necessary because I at one time partially put in + and | (one or
 * more and alternation.)
 */
struct	regexp {
	char	Expbuf[ESIZE + 2];
	bool	Circfl;
	short	Nbra;
};

/*
 * There are three regular expressions here, the previous (in re),
 * the previous substitute (in subre) and the previous scanning (in scanre).
 * It would be possible to get rid of "re" by making it a stack parameter
 * to the appropriate routines.
 */
var struct	regexp re;		/* Last re */
var struct	regexp scanre;		/* Last scanning re */
var struct	regexp subre;		/* Last substitute re */

/*
 * Defining circfl and expbuf like this saves us from having to change
 * old code in the ex_re.c stuff.
 */
#define	expbuf	re.Expbuf
#define	circfl	re.Circfl
#define	nbra	re.Nbra

/*
 * Since the phototypesetter v7-epsilon
 * C compiler doesn't have structure assignment...
 */
#define	savere(a)	copy(&a, &re, sizeof (struct regexp))
#define	resre(a)	copy(&re, &a, sizeof (struct regexp))

/*
 * Definitions for substitute
 */
var char	*braslist[NBRA];	/* Starts of \(\)'ed text in lhs */
var char	*braelist[NBRA];	/* Ends... */
var char	rhsbuf[RHSSIZE];	/* Rhs of last substitute */

/*
 * Definitions of codes for the compiled re's.
 * The re algorithm is described in a paper
 * by K. Thompson in the CACM about 10 years ago
 * and is the same as in ed.
 */
#define	STAR	1

#define	CBRA	1
#define	CDOT	4
#define	CCL	8
#define	NCCL	12
#define	CDOL	16
#define	CEOFC	17
#define	CKET	18
#define	CCHR	20
#define	CBRC	24
#define	CLET	25