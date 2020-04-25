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

/* $Header: ex_argv.h 2.0 86/01/28 $ */

/* sccs id:	@(#)ex_argv.h	7.2	7/26/81  */
/* Copyright (c) 1981 Regents of the University of California */
/*
 * The current implementation of the argument list is poor,
 * using an argv even for internally done "next" commands.
 * It is not hard to see that this is restrictive and a waste of
 * space.  The statically allocated glob structure could be replaced
 * by a dynamically allocated argument area space.
 */
var char	**argv;
var char	**argv0;
var char	*args;
var char	*args0;
var short	argc;
var short	argc0;
var short	morargc;		/* Used with "More files to edit..." */

var int	firstln;		/* From +lineno */
var char	*firstpat;		/* From +/pat	*/

/* Yech... */
struct	glob {
	short	argc;			/* Index of current file in argv */
	short	argc0;			/* Number of arguments in argv */
	char	*argv[NARGS + 1];	/* WHAT A WASTE! */
	char	argspac[NCARGS + sizeof (int)];
};
var struct	glob frob;
