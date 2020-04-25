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

/* $Header: _ppinit.c 1.3 86/11/25 $
 *
 * _ppinit()
 *	Stub for non-parallel programs.
 *
 * Called by /lib/crt0.o to allow run-time setup.
 */

/*ARGSUSED*/
_ppinit(argc, argv)
	int	argc;
	char	**argv;
{
	/* NOP */
}
