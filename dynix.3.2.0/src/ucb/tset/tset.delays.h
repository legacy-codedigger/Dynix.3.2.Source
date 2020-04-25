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

/* $Header: tset.delays.h 2.0 86/01/28 $ */

/*
**  SYSTEM DEPENDENT TERMINAL DELAY TABLES
**
**	Evans Hall VAX
**
**	This file maintains the correspondence between the delays
**	defined in /etc/termcap and the delay algorithms on a
**	particular system.  For each type of delay, the bits used
**	for that delay must be specified (in XXbits) and a table
**	must be defined giving correspondences between delays and
**	algorithms.  Algorithms which are not fixed delays (such
**	as dependent on current column or line number) must be
**	cludged in some way at this time.
*/



/*
**  Carriage Return delays
*/

int	CRbits = CRDELAY;
struct delay	CRdelay[] =
{
	0,	CR0,
	9,	CR3,
	80,	CR1,
	160,	CR2,
	-1
};

/*
**  New Line delays
*/

int	NLbits = NLDELAY;
struct delay	NLdelay[] =
{
	0,	NL0,
	66,	NL1,		/* special M37 delay */
	100,	NL2,
	-1
};


/*
**  Back Space delays
*/

int	BSbits = BSDELAY;
struct delay	BSdelay[] =
{
	0,	BS0,
	-1
};


/*
**  TaB delays
*/

int	TBbits = TBDELAY;
struct delay	TBdelay[] =
{
	0,	TAB0,
	11,	TAB1,		/* special M37 delay */
	-1
};


/*
**  Form Feed delays
*/

int	FFbits = VTDELAY;
struct delay	FFdelay[] =
{
	0,	FF0,
	2000,	FF1,
	-1
};

#ifdef CBVIRTTERM
/*
 * Map from the universal tables in termcap to the particular numbers
 * this system uses.  The lack of standardization of terminal numbers
 * is a botch but such is life.
 */
struct vt_map {
	char stdnum;
	char localnum;
} vt_map[] = {
#ifdef	TERM_TEC
	1, TERM_TEC,
#endif
#ifdef	TERM_V61
	2, TERM_V61,
#endif
#ifdef	TERM_V10
	3, TERM_V10,
#endif
#ifdef	TERM_TEX
	4, TERM_TEX,
#endif
#ifdef	TERM_D40
	5, TERM_D40,
#endif
#ifdef	TERM_H45
	6, TERM_H45,
#endif
#ifdef	TERM_D42
	7, TERM_D42,
#endif
#ifdef TERM_C100
	8, TERM_C100,
#endif
#ifdef TERM_MIME
	9, TERM_MIME,
#endif
	0,0
};
#endif
