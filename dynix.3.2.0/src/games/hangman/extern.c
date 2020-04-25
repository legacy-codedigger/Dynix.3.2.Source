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

/* $Header: extern.c 2.0 86/01/28 $ */

# include	"hangman.h"

bool	Guessed[26];

char	Word[BUFSIZ],
	Known[BUFSIZ],
	*Noose_pict[] = {
		"     ______",
		"     |    |",
		"     |",
		"     |",
		"     |",
		"     |",
		"   __|_____",
		"   |      |___",
		"   |_________|",
		NULL
	};

int	Errors,
	Wordnum = 0;

double	Average = 0.0;

ERR_POS	Err_pos[MAXERRS] = {
	{  2, 10, 'O' },
	{  3, 10, '|' },
	{  4, 10, '|' },
	{  5,  9, '/' },
	{  3,  9, '/' },
	{  3, 11, '\\' },
	{  5, 11, '\\' }
};

FILE	*Dict = NULL;

off_t	Dict_size;
