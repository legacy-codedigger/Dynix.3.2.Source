/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

/* $Log: brk.c,v $
 *
 */

#ifndef lint
static char rcsid[] = "$Header: brk.c 1.2 1991/04/26 16:26:56 $";
#endif

/*
 * Sys V Includes.
 */

#include <sys/types.h>
#include <errno.h>

#define	u_long	unsigned long
#define	u_short unsigned short
#define	u_char	unsigned char

#define NULL	0

#include "mman.h"	/* DYNIX include file */

/* Defines for DYNIX support */
#define	flock	_flock
#define	getpagesize _getpagesize
#define	mmap	__mmmap
#define lseek	_lseek
#define fchmod	_fchmod
#define BLOCK	oldmask=_sigblock(0xffffffff)
#define UNBLOCK	_sigsetmask(oldmask)
int	_pgoff;			/* getpagesize() - 1 */
char *_shmat_start = NULL;
#define	PGRND(x)	(((int)(x) + _pgoff) & ~_pgoff)
#define	PG_ALIGN(x)	(char *) (((int)(x)) & ~_pgoff)

/*
 * brk
 *	basic private memory allocator.
 *
 * Replaces libc's brk()
 *
 */

/*
 * ZFOD_MMAP() is another short-hand for calling mmap().
 */

#define	ZFOD_MMAP(va,sz)	\
		mmap(va, sz, PROT_READ|PROT_WRITE, MAP_ZEROFILL, 0, 0)

extern	char	end;

char	*_curbrk = (char *)&end;	/* used by brk() */
char	*_minbrk = (char *)&end;	/* used by brk() */

#define BLOCK	oldmask=_sigblock(0xffffffff)
#define UNBLOCK	_sigsetmask(oldmask)
int oldmask;

/*
 * brk()
 *	Add private address space.
 */

brk(newbrk)
	register char	*newbrk;
{
	register char	*cur;			/* actual brk (rounded) */
	register char	*new;			/* requested brk (rounded) */

	if (_pgoff == 0)
		_pgoff = getpagesize() - 1;

	if (newbrk < _minbrk)			/* keep original data */
		newbrk = _minbrk;

	BLOCK;
	if ((_shmat_start) && (newbrk > _shmat_start)) {
		UNBLOCK;
		errno = ENOMEM;
		return(-1);
	}

	cur = (char *)PGRND(_curbrk);
	new = (char *)PGRND(newbrk);

	/*
	 * Three cases: grow, shrink, or no actual change.
	 */

	if (new > cur) {					/* grow */
		if (new_space(cur, new-cur) < 0) {
			UNBLOCK;
			return(-1);
		}
	} else if (new < cur) {					/* shrink */
		if (loose_space(new, cur-new) < 0) {
			UNBLOCK;
			return(-1);
		}
	}

	_curbrk = newbrk;
	UNBLOCK;
	return(0);
}

/* 
 * new_space()
 *	Create new private space.  Simply ask mmap for it.
 *
 * Arguments are system page-size aligned.
 */

static
new_space(vaddr, size)
	char	*vaddr;
	int	size;
{
	if (ZFOD_MMAP(vaddr, size) < 0) {
		errno = ENOMEM;
		return(-1);
	}
	return(0);
}

/*
 * loose_space()
 *	Loose some address space.  Munmap() does wonders.
 *
 * Arguments are system page-size aligned.
 */

static
loose_space(vaddr, size)
	char	*vaddr;
	int	size;
{
	return(_munmap(vaddr, size));
}

