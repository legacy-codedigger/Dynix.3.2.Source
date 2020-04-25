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

/* $Header: getgroup.h 1.1 86/12/18 $ */


/*	@(#)getgroup.h 1.1 86/02/05 (C) 1985 Sun Microsystems, Inc.	*/
/* @(#)getgroup.h	2.1 86/04/16 NFSSRC */ 

struct grouplist {		
	char *gl_machine;
	char *gl_name;
	char *gl_domain;
	struct grouplist *gl_nxt;
};

struct grouplist *my_getgroup();

			
