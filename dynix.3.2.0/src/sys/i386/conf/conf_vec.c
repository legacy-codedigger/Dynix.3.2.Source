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

#ifndef lint
static char rcsid[] = "$Header: conf_vec.c 2.3 89/08/18 $";
#endif lint

/*
 * vec_conf.c
 *
 * This file contains the definition of software
 * interrupt and trap vectors.
 */

/* $Log:	conf_vec.c,v $
 */

/*
 * Software Interrupt Vectors.
 */

extern	int	undef(), netintr(), softclock(), resched();
extern	int	FlushTLBIntr();

int	(*softvec[8])() = {
	netintr,    	/* 0 */
	FlushTLBIntr,	/* 1 */
	undef,   	/* 2 */
	undef,		/* 3 */
	softclock, 	/* 4 */
	undef, 		/* 5 */
	undef, 		/* 6 */
	resched,	/* 7 */
};

/*
 * Software Trap Vectors.
 */

extern	int	swt_undef(), vpffintr(), profswt();

extern	int	swt_fpu_pgxbug();

int	(*swtrapvec[8])() = {
	vpffintr,    	/* 0 */
	profswt,	/* 1 */
	swt_undef,   	/* 2 */
	swt_undef,	/* 3 */
	swt_undef, 	/* 4 */
	swt_undef, 	/* 5 */
	swt_undef, 	/* 6 */
	swt_fpu_pgxbug,	/* 7 */
};
