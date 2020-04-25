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
 * $Header: kernel.h 2.1 86/04/03 $
 *
 * kernel.h
 *	Global variables for the kernel
 */

/* $Log:	kernel.h,v $
 */

long	rmalloc();

/* 1.1 */
extern	long	hostid;
extern	char	hostname[32];
extern	int	hostnamelen;
extern	char	domainname[32];
extern	int	domainnamelen;

/* 1.2 */
extern	struct	timeval boottime;
extern	struct	timeval time;
extern	struct	timezone tz;
int	realitexpire();
extern	int	hz;
extern	int	tick;
extern	sema_t	lbolt;				/* awoken once a second */

extern	long	avenrun[3];			/* load average */

extern	struct	pte	zeropg[];		/* source of RO zeroes */
