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

#ifndef lint
static char rcsid[]= "$Header: usclk.c 1.3 87/08/06 $";
#endif

/*
 *	usclk_init:  Allocates storage for and maps the microsecond
 *	 clock device into virtual memory.  Also probes the device to ensure
 *	 its presence.
 */

/* $Log:	usclk.c,v $
 */
#include <sys/types.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <errno.h>
#include <usclkc.h>

#define USCLK_DEV	"/dev/usclk"
#define ADDR_RND	(2*(4*1024))
#define PGRND(x)	(char *) (((int)(x) + _pgoff) & ~_pgoff)

static char usclkmap[(ADDR_RND + ADDR_RND - 1)];
static int _pgoff;
static int usclk_fd;	/* file descriptor for usclk device */

#if defined(ns32000) || defined(KXX)
static
#endif defined(i386) && !defined(KXX)
usclk_t *usclk_base;

#if defined(ns32000) || defined(KXX)
typedef unsigned short	usclk16_t;
usclk16_t	*usclk_p;		/* Points to low address of clock */

#define USCLK_HI2	0
#define USCLK_LO	1
#define USCLK_HI1	2
#else
usclk_t	*usclk_p;

#endif defined(ns32000) || defined(KXX)

void
usclk_init()				/* C interface */
{
	usclk_fd = open(USCLK_DEV, O_RDONLY, 0);
	if (usclk_fd < 0) {
		perror("error opening microsecond clock");
		exit(1);
	}

	_pgoff = getpagesize() - 1;
	if (_pgoff > ADDR_RND) {
		errno = EINVAL;
		perror("binary page size too old");
		exit(1);
	}

	usclk_base = (usclk_t *)PGRND(usclkmap);
	if (mmap(usclk_base, getpagesize(), PROT_READ, MAP_SHARED, usclk_fd, 0) < 0) {
		perror("error mapping usclk device");
		exit(1);
	}

#if defined(ns32000) || defined(KXX)
	usclk_p = (usclk16_t *)usclk_base + USCLK_LO;
#else
	usclk_p = usclk_base;
#endif defined(i386) && !defined(KXX)
}

#if defined(i386) && !defined(KXX)
usclk_t
getusclk()
{
	return(*usclk_base);
}
#endif defined(i386) && !defined(KXX)


/*
 *  Make routine addressable by same name from both Fortran and C.
 *	This macro definition should be replaced with one from a
 *	/usr/include header file, once that becomes available.
 */
#define	ENTRY(x,y)	asm("	.text"); \
			asm("	.globl x"); \
			asm("	.set x,y"); \
			asm("	.data")


ENTRY(usclk_init, _usclk_init);		/* -e style F and P */
ENTRY(_usclk_init_, _usclk_init);	/* V2.9.1 F */

#if defined(i386) && !defined(KXX)
ENTRY(getusclk, _getusclk);		/* -e style F and P */
ENTRY(_getusclk_, _getusclk);		/* V2.9.1 F */
#endif defined(i386) && !defined(KXX)
