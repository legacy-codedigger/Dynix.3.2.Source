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

#ifndef lint
static char rcsid[] = "$Header: clear.c 2.1 90/03/06 $";
#endif

/* load me with -ltermlib */
/* #include <retrofit.h> on version 6 */
/*
 * clear - clear the screen
 */

#include <stdio.h>
#include <sgtty.h>

char	*getenv();
char	*tgetstr();
char	PC;
short	ospeed;
#undef	putchar
int	putchar();

main()
{
	char *cp = getenv("TERM");
	char clbuf[20];
	char pcbuf[20];
	char *clbp = clbuf;
	char *pcbp = pcbuf;
	char *clear;
	char buf[1024];
	char *pc;
	struct sgttyb tty;

	gtty(1, &tty);
	ospeed = tty.sg_ospeed;
	if (cp == (char *) 0)
		exit(1);
	if (tgetent(buf, cp) != 1)
		exit(1);
	pc = tgetstr("pc", &pcbp);
	if (pc)
		PC = *pc;
	clear = tgetstr("cl", &clbp);
	if (clear)
		tputs(clear, tgetnum("li"), putchar);
	exit (clear == (char *) 0);
}
