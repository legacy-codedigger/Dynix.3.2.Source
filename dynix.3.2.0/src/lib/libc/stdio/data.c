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

/* $Header: data.c 2.3 1991/10/01 00:02:46 $ */
/* @(#)data.c	4.3 (Berkeley) 5/15/84 */

#include <stdio.h>
#include <errno.h>

extern int errno;

#define active(iop)	((iop)->_flag & (_IOREAD|_IOWRT|_IORW))

#define NSTATIC	20	/* stdin + stdout + stderr + the usual */

FILE _iob[NSTATIC] = {
	{ 0, NULL, NULL, 0, _IOREAD,		0 },	/* stdin  */
	{ 0, NULL, NULL, 0, _IOWRT,		1 },	/* stdout */
	{ 0, NULL, NULL, 0, _IOWRT|_IONBF,	2 },	/* stderr */
};

/*
 * The following is strictly for support of 3.1 release of C++ product.
 * NOTHING else should use _lastbuf as it is no longer supported.
 */
struct   _iobuf  *_lastbuf = { &_iob[NSTATIC] };

extern	char	*calloc();
extern	char	*realloc();

static	char sbuf[NSTATIC];
char	*_smallbuf = sbuf;
static	FILE	**iobglue;
static	FILE	**endglue;
static	int	oldnfiles = NSTATIC;

/*
 * Find a free FILE for fopen et al.
 * We have a fixed static array of entries, and in addition
 * may allocate additional entries dynamically, up to the kernel
 * limit on the number of open files.
 * At first just check for a free slot in the fixed static array.
 * If none are available, then we allocate a structure to glue together
 * the old and new FILE entries, which are then no longer contiguous.
 */
FILE *
_findiop()
{
	register FILE **iov, *iop;
	register FILE *fp;

	if (iobglue == 0) {
		for (iop = _iob; iop < _iob + NSTATIC; iop++)
			if (!active(iop))
				return (iop);

		if (_f_morefiles() == 0) {
			errno = ENOMEM;
			return (NULL);
		}
	}

retry:
	iov = iobglue;
	while (*iov != NULL && active(*iov))
		if (++iov >= endglue) {
			if (_f_even_morefiles() == 0) {
				errno = EMFILE;
				return (NULL);
			} else {
				goto retry;
			}
		}

	if (*iov == NULL)
		*iov = (FILE *)calloc(1, sizeof **iov);

	return (*iov);
}

_f_morefiles()
{
	register FILE **iov;
	register FILE *fp;
	register char *cp;
	int nfiles;

	nfiles = getdtablesize();

	iobglue = (FILE **)calloc(nfiles, sizeof *iobglue);
	if (iobglue == NULL)
		return (0);

	endglue = iobglue + nfiles;

	for (fp = _iob, iov = iobglue; fp < &_iob[NSTATIC]; /* void */)
		*iov++ = fp++;

	_smallbuf = calloc(nfiles, sizeof(*_smallbuf));
	oldnfiles = nfiles;
	return (1);
}

f_prealloc()
{
	register FILE **iov;
	register FILE *fp;

	if (iobglue == NULL && _f_morefiles() == 0)
		return;

	for (iov = iobglue; iov < endglue; iov++)
		if (*iov == NULL)
			*iov = (FILE *)calloc(1, sizeof **iov);
}

_fwalk(function)
	register int (*function)();
{
	register FILE **iov;
	register FILE *fp;

	if (iobglue == NULL) {
		for (fp = _iob; fp < &_iob[NSTATIC]; fp++)
			if (active(fp))
				(*function)(fp);
	} else {
		for (iov = iobglue; iov < endglue; iov++)
			if (*iov && active(*iov))
				(*function)(*iov);
	}
}

_f_even_morefiles()
{
	register FILE **iov;
	register FILE **oldiov;
	register FILE **iobptr;
	register char *fp;
	register char *smbptr;
	int nfiles;

	nfiles = getdtablesize();

	if (nfiles <= oldnfiles) {
		return(0);
	}

	oldiov = iobglue;

	iobptr = (FILE **)realloc(iobglue, nfiles *(sizeof *iobglue));
	if (iobptr == NULL) {
		return (0);
	} else {
		iobglue = iobptr;
	}

	endglue = iobglue + nfiles;

	smbptr = realloc(_smallbuf,nfiles*sizeof(*_smallbuf));
	if (smbptr != NULL) {
		_smallbuf = smbptr;
	}
	oldnfiles = nfiles;
	return (1);
}
