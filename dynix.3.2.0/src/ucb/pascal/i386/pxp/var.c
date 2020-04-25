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

#if !defined(lint)
static char rcsid[] = "$Id: var.c,v 1.1 88/09/02 11:44:25 ksb Exp $";
#endif lint

/*
 * pxp - Pascal execution profiler
 *
 * Bill Joy UCB
 * Version 1.2 January 1979
 */

#include "0.h"
#include "tree.h"

STATIC	int varcnt = -1;
/*
 * Var declaration part
 */
varbeg(l, vline)
	int l, vline;
{

	line = l;
	if (nodecl)
		printoff();
	puthedr();
	putcm();
	ppnl();
	indent();
	ppkw("var");
	ppgoin(DECL);
	varcnt = 0;
	setline(vline);
}

var(vline, vidl, vtype)
	int vline;
	register int *vidl;
	int *vtype;
{

	if (varcnt)
		putcm();
	setline(vline);
	ppitem();
	if (vidl != NIL)
		for (;;) {
			ppid(vidl[1]);
			vidl = vidl[2];
			if (vidl == NIL)
				break;
			ppsep(", ");
		}
	else
		ppid("{identifier list}");
	ppsep(":");
	gtype(vtype);
	ppsep(";");
	setinfo(vline);
	putcml();
	varcnt++;
}

varend()
{

	if (varcnt == -1)
		return;
	if (varcnt == 0)
		ppid("{variable decls}");
	ppgoout(DECL);
	varcnt = -1;
}
