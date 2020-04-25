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

/* $Header: slic_rdslave.c 2.2 86/05/01 $ */

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <machine/hwparam.h>
#include <machine/slic.h>
#include "saio.h"

/*
 * This calls wrAddr and rdData to read a slave data register.
 *
 * Returns value of slave register in low order 8 bits.
 * Error occurred if full 32-bit return value is negative.
 */

int
rdslave(destination, reg)
	unsigned char destination, reg;
{
	register int error = 0;
	int value;

	error = wrAddr(destination, reg);
	if (error)
		return(error);
	value = rdData(destination);
	return(value);
}
