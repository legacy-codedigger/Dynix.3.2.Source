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

/* $Header: varpush.c 2.0 86/01/28 $ */

# include	"mille.h"

/*
 * @(#)varpush.c	1.1 (Berkeley) 4/1/82
 */

int	read(), write();

/*
 *	push variables around via the routine func() on the file
 * channel file.  func() is either read or write.
 */
varpush(file, func)
reg int	file;
reg int	(*func)(); {

	int	temp;

	(*func)(file, &Debug, sizeof Debug);
	(*func)(file, &Finished, sizeof Finished);
	(*func)(file, &Order, sizeof Order);
	(*func)(file, &End, sizeof End);
	(*func)(file, &On_exit, sizeof On_exit);
	(*func)(file, &Handstart, sizeof Handstart);
	(*func)(file, &Numgos, sizeof Numgos);
	(*func)(file,  Numseen, sizeof Numseen);
	(*func)(file, &Play, sizeof Play);
	(*func)(file, &Window, sizeof Window);
	(*func)(file,  Deck, sizeof Deck);
	(*func)(file, &Discard, sizeof Discard);
	(*func)(file,  Player, sizeof Player);
	if (func == read) {
		read(file, &temp, sizeof temp);
		Topcard = &Deck[temp];
		if (Debug) {
			char	buf[80];
over:
			printf("Debug file:");
			gets(buf);
			if ((outf = fopen(buf, "w")) == NULL) {
				perror(buf);
				goto over;
			}
			if (strcmp(buf, "/dev/null") != 0)
				setbuf(outf, 0);
		}
	}
	else {
		temp = Topcard - Deck;
		write(file, &temp, sizeof temp);
	}
}
