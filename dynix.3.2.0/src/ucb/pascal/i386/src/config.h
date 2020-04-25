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

/* $Id: config.h,v 1.1 88/09/02 11:47:56 ksb Exp $ */
    /*
     *	external declarations of things from 
     *		CONFIG.c
     *
     */

    /*
     *	the version of translator
     */
extern char	*version;

    /*
     *	the location of the error strings
     *	and the length of the path to it
     *	(in case of execution of pc0 as a.out)
     */
extern char	*err_file;
extern int	err_pathlen;

    /*
     *	the location of the short explanation
     *	and the length of the path to it
     *	the null at the end is so pix can change it to pi'x' from pi.
     */
extern char	*how_file;
extern int	how_pathlen;
extern char	*px_header;
extern char	*pi_comp;
extern char	*px_intrp;
extern char	*px_debug;
