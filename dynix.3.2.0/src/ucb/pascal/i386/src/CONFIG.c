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

#if !defined(lint)
static char rcsid[] = "$Id: CONFIG.c,v 1.1 88/09/02 11:47:51 ksb Exp $";
#endif lint


    /*
     *	make sure you are editing
     *		CONFIG.c
     *	editing config.c won't work
     */

    /*
     *	the version of translator
     */
char	*version = "VERSION (DATE)";

    /*
     *	the location of the error strings
     *	and the length of the path to it
     *	(in case of execution as a.something)
     */
char	*err_file = "LIBDIR/ERRORSTRINGS";
int	err_pathlen = sizeof("LIBDIR/")-1;

    /*
     *	the location of the short explanation
     *	and the length of the path to it
     *	the null at the end is so pix can change it to pi'x' from pi.
     */
char	*how_file = "LIBDIR/HOWFILE\0";
int	how_pathlen = sizeof("LIBDIR/")-1;
    
    /*
     *	things about the interpreter.
     *	these are not used by the compiler.
     */
#if !defined(PC)
char	*px_header = "LIBDIR/px_header";	/* px_header's name */
#endif

#if !defined(PXP)
char	*pi_comp = "INSTALLDIR/pi";		/* the compiler's name */
char	*px_intrp = "INSTALLDIR/px";		/* the interpreter's name */
char	*px_debug = "INSTALLDIR/pdx";		/* the debugger's name */
#endif
