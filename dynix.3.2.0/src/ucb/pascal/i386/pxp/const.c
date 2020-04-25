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
static char rcsid[] = "$Id: const.c,v 1.1 88/09/02 11:44:16 ksb Exp $";
#endif lint

/*
 * pxp - Pascal execution profiler
 *
 * Bill Joy UCB
 * Version 1.2 January 1979
 */

#include "0.h"
#include "tree.h"

STATIC	int constcnt = -1;

/*
 * The const declaration part
 */
constbeg(l, cline)
	int l, cline;
{

	line = l;
	if (nodecl)
		printoff();
	puthedr();
	putcm();
	ppnl();
	indent();
	ppkw("const");
	ppgoin(DECL);
	constcnt = 0;
	setline(cline);
}

const(cline, cid, cdecl)
	int cline;
	char *cid;
	int *cdecl;
{

	if (constcnt)
		putcm();
	setline(cline);
	ppitem();
	ppid(cid);
	ppsep(" = ");
	gconst(cdecl);
	ppsep(";");
	constcnt++;
	setinfo(cline);
	putcml();
}

constend()
{

	if (constcnt == -1)
		return;
	if (nodecl)
		return;
	if (constcnt == 0)
		ppid("{const decls}");
	ppgoout(DECL);
	constcnt = -1;
}

/*
 * A constant in an expression
 * or a declaration.
 */
gconst(r)
	int *r;
{
	register *cn;

	cn = r;
loop:
	if (cn == NIL) {
		ppid("{constant}");
		return;
	}
	switch (cn[0]) {
		default:
			panic("gconst");
		case T_PLUSC:
			ppop("+");
			cn = cn[1];
			goto loop;
		case T_MINUSC:
			ppop("-");
			cn = cn[1];
			goto loop;
		case T_ID:
			ppid(cn[1]);
			return;
		case T_CBINT:
		case T_CINT:
		case T_CFINT:
			ppnumb(cn[1]);
			if (cn[0] == T_CBINT)
				ppsep("b");
			return;
		case T_CSTRNG:
			ppstr(cn[1]);
			return;
	}
}
