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
static char rcsid[] = "$Header: main.c 2.0 86/01/28 $";
#endif

#include <signal.h>
#include <stdio.h>
#include "1.defs.h"
#include "def.h"


char (*input)(), (*unput)();
FILE *outfd	= stdout;



main(argc,argv)
int argc;
char *argv[];
	{
	int anyoutput;
	int dexit();
	char *getargs();
	char input1(), unput1(), input2(), unput2();
	anyoutput = FALSE;
	getargs(argc,argv);
	if (debug == 2) debfd = stderr;
	else if (debug)
		debfd = fopen("debug1","w");

	if (signal(SIGINT, SIG_IGN) !=SIG_IGN)
		signal(SIGINT,dexit);
	prog_init();

	for (;;)
		{
		++routnum;
		routerr = 0;

		input = input1;
		unput = unput1;
		if (!mkgraph()) break;
		if (debug) prgraph();
		if (routerr) continue;

		if (progress)fprintf(stderr,"build:\n");
		build();
		if (debug) prtree();
		if (routerr) continue;

		if (progress)fprintf(stderr,"structure:\n");
		structure();
		if (debug) prtree();
		if (routerr) continue;
		input = input2;
		unput = unput2;

		if (progress)fprintf(stderr,"output:\n");
		output();
		if (routerr) continue;
		anyoutput = TRUE;
		freegraf();
		}
	if (anyoutput)
		exit(0);
	else
		exit(1);
	}


dexit()
	{
	exit(1);
	}
