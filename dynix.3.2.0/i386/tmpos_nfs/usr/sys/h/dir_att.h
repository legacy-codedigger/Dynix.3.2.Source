
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

/*
 * $Header: dir_att.h 2.0 86/01/28 $
 */

/*
 * $Log:	dir_att.h,v $
 */

/*
 * att directory structure, 
 *	renamed here to not collide with ucb directory structure
 *	NOTE that DIRSIZ also had to be renamed due to collision with
 * 	the ucb DIRSIZ macro!
 */

#ifndef	DIRSIZ_ATT
#define	DIRSIZ_ATT	255
#endif
struct	direct_att
{
	ino_t	d_ino;
	char	d_name[DIRSIZ_ATT + 1];	/* for assured null termination */
};
