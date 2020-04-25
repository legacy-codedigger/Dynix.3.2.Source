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

/* $Header: table.c 2.0 86/01/28 $ */

# define	DEBUG

/*
 * @(#)table.c	1.1 (Berkeley) 4/1/82
 */

# include	"mille.h"

main() {

	reg int	i, j, count;

	printf("   %16s -> %5s %5s %4s %s\n", "Card", "cards", "count", "need", "opposite");
	for (i = 0; i < NUM_CARDS - 1; i++) {
		for (j = 0, count = 0; j < DECK_SZ; j++)
			if (Deck[j] == i)
				count++;
		printf("%2d %16s -> %5d %5d %4d %s\n", i, C_name[i], Numcards[i], count, Numneed[i], C_name[opposite(i)]);
	}
}
