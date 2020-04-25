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

/* $Header: v7.local.h 2.1 87/04/02 $ */

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)v7.local.h	5.2 (Berkeley) 9/19/85
 */

/*
 * Declarations and constants specific to an installation.
 *
 * Vax/Unix version 7.
 */

#define	GETHOST				/* System has gethostname syscall */
#ifdef	GETHOST
#define	LOCAL		EMPTYID		/* Dynamically determined local host */
#else
#define	LOCAL		'j'		/* Local host id */
#endif	GETHOST

#define	MAIL		"/bin/mail"	/* Name of mail sender */
#define SENDMAIL	"/usr/lib/sendmail"
					/* Name of classy mail deliverer */
#define	EDITOR		"/usr/ucb/ex"	/* Name of text editor */
#define	VISUAL		"/usr/ucb/vi"	/* Name of display editor */
#define	SHELL		"/bin/csh"	/* Standard shell */
#define	MORE		"/usr/ucb/more"	/* Standard output pager */
#define	HELPFILE	"/usr/lib/Mail.help"
					/* Name of casual help file */
#define	THELPFILE	"/usr/lib/Mail.tildehelp"
#define	POSTAGE		"/usr/adm/maillog"
					/* Where to audit mail sending */
					/* Name of casual tilde help */
#define	UIDMASK		0177777		/* Significant uid bits */
#define	MASTER		"/usr/lib/Mail.rc"
#define	APPEND				/* New mail goes to end of mailbox */
#define CANLOCK				/* Locking protocol actually works */
#define	UTIME				/* System implements utime(2) */

#ifndef VMUNIX
#include "sigretro.h"			/* Retrofit signal defs */
#endif VMUNIX
