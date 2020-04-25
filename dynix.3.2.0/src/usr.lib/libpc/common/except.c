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

/* $Header: except.c 1.1 89/03/12 $ */
#include <signal.h>

static char *sbpchError[] = {
	"Integer division by zero\n",	/* 0x0	integer  DIVide by zero */
	"Real underflow\n",		/* 0x1	floating UNDerflow */
	"Real overflow\n",		/* 0x2	floating OVerFlow */
	"Real division by zero\n",	/* 0x3	floating DIVide by zero */
	"Illegal instruction\n",	/* 0x4	floating ILLegal instruction */
	"Invalid real operation\n",	/* 0x5	floating INValid operation */
	"Inexact real result\n",	/* 0x6	floating INexact Result */
	"Reserved instruction trap\n",	/* 0x7	floating ReSerVed for future */
	"Panic: Computational error in interpreter\n"
};

/*
 * catch runtime arithmetic errors
 */
EXCEPT(signum, type)
int signum, type;
{
	if (type < 0 || type > 7)
		type = 8;
	ERROR(sbpchError[type], 0);
	PCEXIT(1);
}
