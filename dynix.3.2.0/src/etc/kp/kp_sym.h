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
 * $Header: kp_sym.h 1.1 86/10/07 $
 */

/* $Log:	kp_sym.h,v $
 */

/*
 * Structures for internal symbol tables
 */
struct	sym { 
	char *sym_name; 		/* pointer into string table */
	unsigned long sym_value; 	/* value of symbol itself */
	unsigned char sym_type;		/* the type */
};

#define	MAXSTRLEN	64

extern getsymboltable();
