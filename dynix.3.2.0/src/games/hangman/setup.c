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

/* $Header: setup.c 2.0 86/01/28 $ */

# include	"hangman.h"

/*
 * setup:
 *	Set up the strings on the screen.
 */
setup()
{
	register char		**sp;
	static struct stat	sbuf;

	noecho();
	crmode();

	mvaddstr(PROMPTY, PROMPTX, "Guess:");
	mvaddstr(GUESSY, GUESSX, "Guessed:");
	mvaddstr(NUMBERY, NUMBERX, "Word #:");
	mvaddstr(AVGY, AVGX, "Current Average:");
	mvaddstr(AVGY + 1, AVGX, "Overall Average:");
	mvaddstr(KNOWNY, KNOWNX, "Word: ");

	for (sp = Noose_pict; *sp != NULL; sp++) {
		move(sp - Noose_pict, 0);
		addstr(*sp);
	}

	srand(time(NULL) + getpid());
	if ((Dict = fopen(DICT, "r")) == NULL) {
		perror(DICT);
		endwin();
		exit(1);
	}
	fstat(fileno(Dict), &sbuf);
	Dict_size = sbuf.st_size;
}
