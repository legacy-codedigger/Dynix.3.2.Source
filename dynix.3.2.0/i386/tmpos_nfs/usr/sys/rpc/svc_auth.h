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
 * $Header: svc_auth.h 1.1 86/11/02 $
 *
 * svc_auth.h
 *	Service side of rpc authentication.
 * 
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

/* $Log:	svc_auth.h,v $
 */

/*
 * Server side authenticator
 */
extern enum auth_stat _authenticate();
