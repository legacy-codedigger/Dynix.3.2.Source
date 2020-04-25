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

#ifndef	lint
static	char	rcsid[] = "$Header: emon_test.c 2.3 87/04/11 $";
#endif

/*
 * $Log:	emon_test.c,v $
 */

#include "emon.h"

extern u_char trapE[];

do_do_test()
{
	printf("THIS IS A TEST of the Ethernet Broadcasting System\n");
	printf("To install a new command - write it as do_do_test in\n");
	printf("\t\temon_test.c\n");
	printf("NOW TESTING addname\n");
	do_do_addname();
}

