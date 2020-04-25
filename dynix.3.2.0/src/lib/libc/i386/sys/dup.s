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

/* $Header: dup.s 2.3 87/06/22 $
 *
 * $Log:	dup.s,v $
 */

/*
 * Handles case where caller passes two args to dup(), with 1st having
 * "DUPTO" bit set -- old interface.  Kernel doesn't support "dup2" in
 * dup(), rather insists we call dup2() if that's what we want.
 */

#include "SYS.h"

#define	DUPTOBIT	6

ENTRY(dup)
	btrl	$DUPTOBIT, SPARG0		# is dup2?
	jc	isdup2
	SVC1(dup)
	jc	err
	ret
isdup2:
	SVC2(dup2)
	jc	err
	ret
CERROR
