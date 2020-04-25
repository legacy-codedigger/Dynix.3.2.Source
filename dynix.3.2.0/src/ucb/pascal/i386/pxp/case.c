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
static char rcsid[] = "$Id: case.c,v 1.1 88/09/02 11:44:15 ksb Exp $";
#endif lint

/*
 * pxp - Pascal execution profiler
 *
 * Bill Joy UCB
 * Version 1.2 January 1979
 */

#include "0.h"
#include "tree.h"

/*
 * Case statement
 *	r	[0]	T_CASE
 *		[1]	lineof "case"
 *		[2]	expression
 *		[3]	list of cased statements:
 *			cstat	[0]	T_CSTAT
 *				[1]	lineof ":"
 *				[2]	list of constant labels
 *				[3]	statement
 */
caseop(r)
	int *r;
{
	register *cl, *cs, i;
	struct pxcnt scnt;
#if defined(RMOTHERS)
	    int *othersp;		/* tree where others is, or NIL */
	    int hasothers;		/* 1 if others found, else 0 */
#endif RMOTHERS

#if defined(RMOTHERS)
	    if (rmothers) {
		hasothers = needscaseguard(r,&othersp);
		if (hasothers) {
		    precaseguard(r);
		}
	    }
#endif RMOTHERS
	savecnt(&scnt);
	ppkw("case");
	ppspac();
	rvalue(r[2], NIL);
	ppspac();
	ppkw("of");
	for (cl = r[3]; cl != NIL;) {
		cs = cl[1];
		if (cs == NIL)
			continue;
		baroff();
		ppgoin(DECL);
		setline(cs[1]);
		ppnl();
		indent();
		ppbra(NIL);
		cs = cs[2];
		if (cs != NIL) {
			i = 0;
			for (;;) {
				gconst(cs[1]);
				cs = cs[2];
				if (cs == NIL)
					break;
				i++;
				if (i == 7) {
					ppsep(",");
					ppitem();
					i = 0;
				} else
					ppsep(", ");
			}
		} else
			ppid("{case label list}");
		ppket(":");
		cs = cl[1];
		cs = cs[3];
		getcnt();
		ppgoin(STAT);
		if (cs != NIL && cs[0] == T_BLOCK) {
			ppnl();
			indent();
			baron();
			ppstbl1(cs, STAT);
			baroff();
			ppstbl2();
			baron();
		} else {
			baron();
			statement(cs);
		}
		ppgoout(STAT);
		ppgoout(DECL);
		cl = cl[2];
		if (cl == NIL)
			break;
		ppsep(";");
	}
	if (rescnt(&scnt))
		getcnt();
	ppnl();
	indent();
	ppkw("end");
#if defined(RMOTHERS)
	    if (rmothers) {
		if (hasothers) {
		    postcaseguard(othersp);
		}
	    }
#endif RMOTHERS
}
