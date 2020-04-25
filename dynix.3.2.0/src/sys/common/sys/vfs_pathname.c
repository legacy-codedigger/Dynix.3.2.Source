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

#ifndef	lint
static	char	rcsid[] = "$Header: vfs_pathname.c 2.2 87/06/30 $";
#endif

/*
 * vfs_pathname.c
 *	Virtual File-System path name manipulation utilities.
 *
 * In translating file names we copy each argument file name into a
 * pathname structure where we operate on it.  Each pathname structure
 * can hold MAXPATHLEN characters, and operations here support allocating
 * and freeing pathname structures, fetching strings from user space,
 * getting the next character from a pathname, combining two pathnames
 * (used in symbolic link processing), and peeling off the next
 * component of a pathname.
 *
 * Reasonably sized pathnames fit into pn_data in the structure itself.
 * Larger ones are placed in dynamically allocated space.
 */

/* $Log:	vfs_pathname.c,v $
 */

#include "../h/param.h"
#include "../h/dir.h"
#include "../h/uio.h"
#include "../h/buf.h"
#include "../h/errno.h"
#include "../h/pathname.h"

/*
 * pn_alloc()
 *	Allocate contents of pathname structure.
 *
 * Structure itself is typically automatic variable in calling
 * routine for convenience.  This allocates a full sized path
 * if path doesnt fit in pn_data.
 *
 * Note: callers all re-copy the 1st PN_BUFSIZE bytes; could get smart
 * and avoid the copy.  Careful: pn_bp overlays pn_data[].
 */

pn_alloc(pnp)
	register struct pathname *pnp;
{
	pnp->pn_bp = geteblk(MAXPATHLEN);
	pnp->pn_path = pnp->pn_buf = pnp->pn_bp->b_un.b_addr;
	pnp->pn_pathlen = 0;
}
 
/*
 * pn_get()
 *	Pull a pathname from user or kernel space
 */

pn_get(str, seg, pnp)
	register struct pathname *pnp;
	int seg;
	register char *str;
{
	int error;

	pnp->pn_path = pnp->pn_buf = pnp->pn_data;

	if (seg == UIOSEG_USER)
		error=copyinstr(str,pnp->pn_path, PN_BUFSIZE, &pnp->pn_pathlen);
	else
		error=copystr(str, pnp->pn_path, PN_BUFSIZE, &pnp->pn_pathlen);

	if (error == ENOENT) {
		/*
		 * The path won't fit in our ample stack buffer.
		 * Get max size path and try again.
		 */
		pn_alloc(pnp);
		if (seg == UIOSEG_USER)
			error = copyinstr(str, pnp->pn_path, MAXPATHLEN,
						 &pnp->pn_pathlen);
		else
			error = copystr(str, pnp->pn_path, MAXPATHLEN,
						&pnp->pn_pathlen);
	}

	--pnp->pn_pathlen;			/* copy*str() counts the NULL */
	if (error)
		pn_free(pnp);

	return(error);
}


/*
 * pn_set()
 *	Set pathname to argument string.
 *
 * pnp must point to an initialized pathanme struct.
 */

pn_set(pnp, path)
	register struct pathname *pnp;
	register char *path;
{
	register len;
	int error;

	pnp->pn_path = pnp->pn_buf;
	len = (pnp->pn_buf == pnp->pn_data ? PN_BUFSIZE : MAXPATHLEN);
	error = copystr(path, pnp->pn_path, len, &pnp->pn_pathlen);
	if (error == ENOENT && len == PN_BUFSIZE)  {
		pn_alloc(pnp);
		error = copystr(path, pnp->pn_path,MAXPATHLEN,&pnp->pn_pathlen);
	}
	--pnp->pn_pathlen;			/* copystr() counts the NULL */
	return (error);
}

/*
 * pn_combine()
 *	Combine two argument pathnames by putting second argument
 *	before first in first's buffer.
 *
 * Does not pn_free() the 2nd argument; caller must do this.
 *
 * This isn't very general: it is designed specifically
 * for symbolic link processing.
 *
 * We further assume that pnp was allocated to MAXPATHLEN by pn_alloc.
 */

pn_combine(pnp, sympnp)
	register struct pathname *pnp;
	register struct pathname *sympnp;
{
	register int	 totlen = pnp->pn_pathlen + sympnp->pn_pathlen;
	struct	buf	*bp = NULL;
	
	if (totlen >= MAXPATHLEN)
		return (ENAMETOOLONG);

	/*
	 * If name will grow beyond PN_BUFSIZE, alloc full size path.
	 * Code mimics pn_alloc().
	 */

	if (totlen >= PN_BUFSIZE && pnp->pn_buf == pnp->pn_data) {
		bp = geteblk(MAXPATHLEN);
		pnp->pn_buf = bp->b_un.b_addr;
	}
	
	ovbcopy(pnp->pn_path, pnp->pn_buf + sympnp->pn_pathlen,
						(unsigned)pnp->pn_pathlen);
	bcopy(sympnp->pn_path, pnp->pn_buf, (unsigned)sympnp->pn_pathlen);

	pnp->pn_pathlen += sympnp->pn_pathlen;
	pnp->pn_path = pnp->pn_buf;
	if (bp)					/* alloc'd new name buffer */
		pnp->pn_bp = bp;

	return (0);
}

/*
 * pn_getcomponent()
 *	Strip next component off a pathname.
 */

pn_getcomponent(pnp, component, n)
	register struct pathname *pnp;
	register char *component;
	register int n;				/* max strlen(component) */
{
	register char *cp;
	register int l;

	cp = pnp->pn_path;
	l = pnp->pn_pathlen;
	while (l > 0 && *cp != '/') {
		if (*cp & 0x80)
			return(EPERM);
		if (--n < 0)
			return(ENAMETOOLONG);
		*component++ = *cp++;
		--l;
	}
	pnp->pn_path = cp;
	pnp->pn_pathlen = l;
	*component = 0;
	return (0);
}

/*
 * pn_skipslash()
 *	skip over consecutive slashes in the pathname
 */

void
pn_skipslash(pnp)
	register struct pathname *pnp;
{
	while ((pnp->pn_pathlen > 0) && (*pnp->pn_path == '/')) {
		pnp->pn_path++;
		pnp->pn_pathlen--;
	}
}

/*
 * pn_free()
 *	Free pathname resources.
 */

void
pn_free(pnp)
	register struct pathname *pnp;
{
	if (pnp->pn_buf != pnp->pn_data)
		brelse(pnp->pn_bp);
	pnp->pn_buf = 0;				/* redundant? */
}
