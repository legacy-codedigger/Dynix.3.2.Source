/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
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
static char *rcsid = "$Header: sh.char.c 1.1 1991/07/26 00:43:47 $";
#endif

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley Software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * static char *sccsid = "@(#)sh.char.c	5.3 (Berkeley) 3/29/86";
 */

#include "sh.char.h"

unsigned short _cmap[256] = {
/*	nul		soh		stx		etx	*/
	0,		0,		0,		0,

/*	eot		enq		ack		bel	*/
	0,		0,		0,		0,

/*	bs		ht		nl		vt	*/
	0,		_SP|_META,	_NL|_META,	0,

/*	np		cr		so		si	*/
	0,		0,		0,		0,

/*	dle		dc1		dc2		dc3	*/
	0,		0,		0,		0,

/*	dc4		nak		syn		etb	*/
	0,		0,		0,		0,

/*	can		em		sub		esc	*/
	0,		0,		0,		0,

/*	fs		gs		rs		us	*/
	0,		0,		0,		0,

/*	sp		!		"		#	*/
	_SP|_META,	0,		_Q,		_META,

/*	$		%		&		'	*/
	_DOL,		0,		_META,		_Q,

/*	(		)		*		+	*/
	_META,		_META,		_GLOB,		0,

/*	,		-		.		/	*/
	0,		0,		0,		0,

/*	0		1		2		3	*/
	_DIG,		_DIG,		_DIG,		_DIG,

/*	4		5		6		7	*/
	_DIG,		_DIG,		_DIG,		_DIG,

/*	8		9		:		;	*/
	_DIG,		_DIG,		0,		_META,

/*	<		=		>		?	*/
	_META,		0,		_META,		_GLOB,

/*	@		A		B		C	*/
	0,		_LET,		_LET,		_LET,

/*	D		E		F		G	*/
	_LET,		_LET,		_LET,		_LET,

/*	H		I		J		K	*/
	_LET,		_LET,		_LET,		_LET,

/*	L		M		N		O	*/
	_LET,		_LET,		_LET,		_LET,

/*	P		Q		R		S	*/
	_LET,		_LET,		_LET,		_LET,

/*	T		U		V		W	*/
	_LET,		_LET,		_LET,		_LET,

/*	X		Y		Z		[	*/
	_LET,		_LET,		_LET,		_GLOB,

/*	\		]		^		_	*/
	_ESC,		0,		0,		_LET,

/*	`		a		b		c	*/
	_Q1|_GLOB,	_LET,		_LET,		_LET,

/*	d		e		f		g	*/
	_LET,		_LET,		_LET,		_LET,

/*	h		i		j		k	*/
	_LET,		_LET,		_LET,		_LET,

/*	l		m		n		o	*/
	_LET,		_LET,		_LET,		_LET,

/*	p		q		r		s	*/
	_LET,		_LET,		_LET,		_LET,

/*	t		u		v		w	*/
	_LET,		_LET,		_LET,		_LET,

/*	x		y		z		{	*/
	_LET,		_LET,		_LET,		_GLOB,

/*	|		}		~		del	*/
	_META,		0,		0,		0,
};
