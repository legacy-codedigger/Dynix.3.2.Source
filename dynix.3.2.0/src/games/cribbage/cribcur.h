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

/* $Header: cribcur.h 2.0 86/01/28 $ */

/* @(#)cribcur.h	1.5 (Berkeley) 5/19/83 */

# define	PLAY_Y		15	/* size of player's hand window */
# define	PLAY_X		12
# define	TABLE_Y		21	/* size of table window */
# define	TABLE_X		14
# define	COMP_Y		15	/* size of computer's hand window */
# define	COMP_X		12
# define	Y_SCORE_SZ	9	/* Y size of score board */
# define	X_SCORE_SZ	41	/* X size of score board */
# define	SCORE_Y		0	/* starting position of scoring board */
# define	SCORE_X	 	(PLAY_X + TABLE_X + COMP_X)
# define	CRIB_Y		17	/* position of crib (cut card) */
# define	CRIB_X		(PLAY_X + TABLE_X)
# define	MSG_Y		(LINES - (Y_SCORE_SZ + 1))
# define	MSG_X		(COLS - SCORE_X - 1)
# define	Y_MSG_START	(Y_SCORE_SZ + 1)

# define	PEG	'*'	/* what a peg looks like on the board */

extern	WINDOW		*Compwin;		/* computer's hand window */
extern	WINDOW		*Msgwin;		/* message window */
extern	WINDOW		*Playwin;		/* player's hand window */
extern	WINDOW		*Tablewin;		/* table window */
