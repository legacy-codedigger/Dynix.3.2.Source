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

/* $Id: fort.h,v 2.8 88/09/02 11:46:11 ksb Exp $ */
/*
 *	Berkeley Pascal Compiler	(fort.h)
 */

/* machine dependent file */

#define label(n) printf("L%d:\n", n)
#define tlabel() (lccopy(2), printf(":\n"))
