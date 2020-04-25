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

/* $Header: deck.h 2.0 86/01/28 $ */

/* @(#)deck.h	1.3 (Berkeley) 5/19/83 */

/*
 * define structure of a deck of cards and other related things
 */


#define		CARDS		52		/* number cards in deck */
#define		RANKS		13		/* number ranks in deck */
#define		SUITS		4		/* number suits in deck */

#define		CINHAND		4		/* # cards in cribbage hand */
#define		FULLHAND	6		/* # cards in dealt hand */

#define		LGAME		121		/* number points in a game */
#define		SGAME		61		/* # points in a short game */

#define		SPADES		0		/* value of each suit */
#define		HEARTS		1
#define		DIAMONDS	2
#define		CLUBS		3

#define		ACE		0		/* value of each rank */
#define		TWO		1
#define		THREE		2
#define		FOUR		3
#define		FIVE		4
#define		SIX		5
#define		SEVEN		6
#define		EIGHT		7
#define		NINE		8
#define		TEN		9
#define		JACK		10
#define		QUEEN		11
#define		KING		12
#define		EMPTY		13

#define		VAL(c)		( (c) < 9 ? (c)+1 : 10 )    /* val of rank */


#ifndef TRUE
#	define		TRUE		1
#	define		FALSE		0
#endif

typedef		struct  {
			int		rank;
			int		suit;
		}		CARD;

typedef		char		BOOLEAN;

