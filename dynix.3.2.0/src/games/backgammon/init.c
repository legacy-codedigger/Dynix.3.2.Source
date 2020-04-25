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

/* $Header: init.c 2.0 86/01/28 $ */

static char sccsid[] = "	init.c	4.1	82/05/11	";

#include <sgtty.h>

/*
 * variable initialization.
 */

				/* name of executable object programs */
char	EXEC[] = "/usr/games/backgammon";
char	TEACH[] = "/usr/games/teachgammon";

int	pnum	= 2;		/* color of player:
					-1 = white
					 1 = red
					 0 = both
					 2 = not yet init'ed */
int	acnt	= 0;		/* length of args */
int	aflag	= 1;		/* flag to ask for rules or instructions */
int	bflag	= 0;		/* flag for automatic board printing */
int	cflag	= 0;		/* case conversion flag */
int	hflag	= 1;		/* flag for cleaning screen */
int	mflag	= 0;		/* backgammon flag */
int	raflag	= 0;		/* 'roll again' flag for recovered game */
int	rflag	= 0;		/* recovered game flag */
int	tflag	= 0;		/* cursor addressing flag */
int	iroll	= 0;		/* special flag for inputting rolls */
int	rfl	= 0;

char	*color[] = {"White","Red","white","red"};
