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
static char rcsid[] = "$Header: 1.init.c 2.0 86/01/28 $";
#endif

#include <stdio.h>
#include "1.defs.h"
#include  "1.incl.h"
#include "def.h"


prog_init()
	{
	endline = endcom = 0;	endchar = -1;
	comchar = -1;
	graph = challoc(sizeof(*graph) * maxnode);
	}

routinit()
	{
	graf_init();
	progtype = !sub;
	routbeg = endline + 1;
	rtnbeg = endchar + 1;
	nameline = 0;
	stflag = UNDEFINED;
	}
line_init()
	{
	struct lablist *makelab();
	freelabs();
	newlab = linelabs = makelab(0L);
	flag = counter = nlabs = lswnum = swptr = p1 = 0;
	p3 = 5;
	endcom = endline;
	comchar = endchar;
	begline = endline + 1;	begchar = endchar + 1;
	reflab = endlab = errlab = 0;
	r1 = r2 = 0;
	}
graf_init()
	{
	int arctype[3];  long arclab[3];
	nodenum = 0;
	doptr = UNDEFINED;
	retvert = stopvert = UNDEFINED;
	ENTLST = FMTLST = 0;

	
	arctype[0] = -2;  arclab[0] = implicit;
	START = makenode(DUMVX,FALSE,FALSE,implicit,1,arctype,arclab);
	}

