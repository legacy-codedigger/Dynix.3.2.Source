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

/* $Id: ndu.h,v 2.8 88/09/02 11:46:22 ksb Exp $ */
/*
 *	Berkeley Pascal Compiler	(ndu.h)
 */

/* This file defines the basic tree node data structure for the PCC */

typedef union ndu {
	struct {		/* interior node */
		int	op;
		int	rall;
		TWORD	type;
		int	su;
#if !defined(FLEXNAMES)
		char	name[NCHNAM];
#else
		char	*name;
		int	stalign;
#endif
		NODE	*left;
		NODE	*right;
	} in;
	struct {		/* terminal node */
		int	op;
		int	rall;
		TWORD	type;
		int	su;
#if !defined(FLEXNAMES)
		char	name[NCHNAM];
#else
		char	*name;
		int	stalign;
#endif
		CONSZ	lval;
		int	rval;
	} tn;
	struct {		/* branch node */
		int	op;
		int	rall;
		TWORD	type;
		int	su;
		int	label;		/* for use with branching */
	} bn;
	struct {		/* structure node */
		int	op;
		int	rall;
		TWORD	type;
		int	su;
		int	stsize;		/* sizes of structure objects */
		int	stalign;	/* alignment of structure objects */
	} stn;
	struct {		/* front node */
		int	op;
		int	cdim;
		TWORD	type;
		int	csiz;
	} fn;
	/*
	 * This structure is used when a double precision
	 * floating point constant is being computed
	 */
	struct {			/* DCON node */
		int	op;
		TWORD	type;
		int	cdim;
		int	csiz;
		double	dval;
	} dpn;
	/*
	 * This structure is used when a single precision
	 * floating point constant is being computed (never! ZZ )
	 */
#if 0
	struct {			/* FCON node */
		int	op;
		TWORD	type;
		int	cdim;
		int	csiz;
		float	fval;
	} fpn;
#endif	/* ksb sayts it never happend in Pascal -- F77 remnant */
} NDU;
