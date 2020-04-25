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

/* $Header: yppasswd.h 1.1 86/12/11 $ */

/*	@(#)yppasswd.h 1.1 86/02/05 SMI */
/* @(#)yppasswd.h	2.1 86/04/14 NFSSRC */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#define YPPASSWDPROG 100009
#define YPPASSWDPROC_UPDATE 1
#define YPPASSWDVERS_ORIG 1
#define YPPASSWDVERS 1

struct yppasswd {
	char *oldpass;		/* old (unencrypted) password */
	struct passwd newpw;	/* new pw structure */
};

int xdr_yppasswd();
