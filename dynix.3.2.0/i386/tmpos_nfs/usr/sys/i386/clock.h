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
 * $Header: clock.h 2.0 86/01/28 $
 */

/* $Log:	clock.h,v $
 */

#define	HZ	100

#define TODFREQ		1000 / HZ			/* 10 milliseconds */
#define	SECDAY		((unsigned)(24*60*60))		/* seconds per day */
#define	SECYR		((unsigned)(365*SECDAY))	/* per common year */

#define	YRREF		1970
#define	LEAPYEAR(year)	((year)%4==0)	/* good till time becomes negative */

/*
 * Local and tod clock bin and vector.  The vector is defined by SLIC as 0x00.
 */

#define	LCLKBIN		7		/* highest priority */
#define	LCLKVEC		0x00		/* according to SLIC */

#define TODCLKBIN	7		/* highest priority */
#define	TODCLKVEC	0x01
