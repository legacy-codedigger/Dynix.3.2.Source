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

/* $Header: playgame.c 2.0 86/01/28 $ */

# include	"hangman.h"

/*
 * playgame:
 *	play a game
 */
playgame()
{
	register bool	*bp;

	getword();
	Errors = 0;
	bp = Guessed;
	while (bp < &Guessed[26])
		*bp++ = FALSE;
	while (Errors < MAXERRS && index(Known, '-') != NULL) {
		prword();
		prdata();
		prman();
		getguess();
	}
	endgame();
}
