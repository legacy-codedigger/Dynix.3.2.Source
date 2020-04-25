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
 * $Header: ranlib.h 2.1 90/11/13 $
 */

/* $Log:	ranlib.h,v $
 */

/*	ranlib.h	4.2	87/04/06	*/

#define	RANLIBMAG	"__.SYMDEF"

/*
 * Structure of the __.SYMDEF table of contents for an archive.
 * __.SYMDEF begins with a word giving the number of ranlib structures
 * which immediately follow, and then continues with a string
 * table consisting of a word giving the number of bytes of strings
 * which follow and then the strings themselves.
 * The ran_strx fields index the string table whose first byte is numbered 0.
 */
struct	ranlib {
	union {
		off_t	ran_strx;	/* string table index of */
		char	*ran_name;	/* symbol defined by */
	} ran_un;
	off_t	ran_off;		/* library member at this offset */
};
