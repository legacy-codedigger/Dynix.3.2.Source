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

/* $Header: slic_error.c 2.2 86/05/01 $ */

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <machine/hwparam.h>
#include <machine/slic.h>
#include "saio.h"

/*
 * Handle and print slic error messages
 */

int
slicerror(type, com, dest, val, stat)
unsigned char com, dest, val, stat;
{
	printf("\nSLIC ");
	switch (type) {
	case SL_PERROR:
		printf("parity");
		break;
	case SL_DERROR:
		printf("destination");
		break;
	case SL_NOTOK:
		printf("command");
		break;
	}
	printf(" err: com %x dest %x val %x stat %x\n",
		com, dest, val, stat);
}
