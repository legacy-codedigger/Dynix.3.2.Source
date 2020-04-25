/* 
 * $Copyright:	$
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
static	char	rcsid[] = "$Header: cust_panics.c 2.0 86/01/28 $";
#endif

/*
 * cust_panics.c
 *	Custom Panic Handling Routines.
 *
 * This file contains a table of procedure addresses.  These procedures
 * are called if the system panics, after disks are sync'd.
 */

/* 
 * $Log:	cust_panics.c,v $
 */

/*
 * To add an entry, define the procedure somewhere, add an extern
 * declaration for the procedure in this module, and add the name of the
 * procedure to the cust_panics[] table.
 */

int	(*cust_panics[])() = {
	/* add entries here */
	(int (*)()) 0				/* terminate list */
};
