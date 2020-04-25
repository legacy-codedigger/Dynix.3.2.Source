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

/*
 * call.c: version 1.2 of 11/30/82
 * 
 */
# ifndef lint
static char rcsid[] = "$Header: call.c 2.0 86/01/28 $";
# endif


/* perform a call to a user function */

/*
 * host defines
 */
#include <ctype.h>
#include <signal.h>

/* 
 * target defines
 */
#include "main.h";
#include "sym.h";
#include "parse.h";
#include "display.h";
#include "bpt.h";
#include "machine.h";

extern int	lastsig;

push(sptr,value)
int *sptr,value;
{
	*sptr -= 4;
	setdouble(*sptr,value);
}

docall()
{
	int callpc, pcreturn, fakesp, savesp, off;
	char *symatch;
	int args, arg, i = 0;

	args = opcnt - 1;
	if (typeops[0] < 0)
		callpc = getreg(typeops[0]);
	else
		callpc = ops[0];
	symatch = lookbyval(callpc, &off);
	if ((symatch == (char *)0) || (off != 0)) {
		printf("\r\ncannot call address\r\n");
		processabort();
	}
	savesp = fakesp = getreg(SSP);
	for (i = args; i > 0; i--) {
	    if (typeops[i] < 0)
		    arg = getreg(typeops[i]);
	    else
		    arg = ops[i];
	    push(&fakesp, arg);
	}
	pcreturn = getreg(SPC);
	maketemp(pcreturn);		/* set break at pc return */
	push(&fakesp, pcreturn);
	setreg(SPC, callpc);
	/* setreg(SMOD, modf); */
	setreg(SSP, fakesp);
	putinbpts(TEMP);
	proceed(GOGO,FALSE);
	outbpts(TEMP);
	if (pcreturn == getreg(SPC)) {
		if (lastsig == SIGBPT)
			lastsig = SIGTRAP;
		setreg(SSP,savesp);
	}
	showspot();
}
