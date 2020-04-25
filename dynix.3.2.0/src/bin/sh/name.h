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

/* $Header: name.h 2.0 86/01/28 $ */

#
/*
 *	UNIX shell
 *
 *	S. R. Bourne
 *	Bell Telephone Laboratories
 *
 */


#define N_RDONLY 0100000
#define N_EXPORT 0040000
#define N_ENVNAM 0020000
#define N_ENVPOS 0007777

#define N_DEFAULT 0

struct namnod {
	NAMPTR	namlft;
	NAMPTR	namrgt;
	STRING	namid;
	STRING	namval;
	STRING	namenv;
	INT	namflg;
};
