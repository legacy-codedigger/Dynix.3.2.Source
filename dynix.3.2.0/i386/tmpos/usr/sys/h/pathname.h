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
 * $Header: pathname.h 2.2 87/06/30 $
 *
 * pathname.h
 *	Path-name structure definitions.
 */

/* $Log:	pathname.h,v $
 */

/*
 * Pathname structure.
 *
 * System calls which operate on path names gather the
 * pathname from system call into this structure and reduce
 * it by peeling off translated components.  If a symbolic
 * link is encountered the new pathname to be translated
 * is also assembled in this structure.
 *
 * The struct includes enough space for normal sized paths.
 * A larger buffer is obtained if we exceed this.
 */

#define PN_BUFSIZE	60

struct	pathname {
	char	*pn_buf;		/* underlying storage */
	char	*pn_path;		/* remaining pathname */
	int	pn_pathlen;		/* remaining length */
	union {
		char	pun_data[PN_BUFSIZE];	/* prealloc for normal paths */
		struct	buf	*pun_bp;	/* when can't fit locally */
	}	pn_pun;
};

#define	pn_data	pn_pun.pun_data
#define	pn_bp	pn_pun.pun_bp

#define	pn_peekchar(PNP)	((PNP)->pn_pathlen>0?*((PNP)->pn_path):0)
#define	pn_pathleft(PNP)	((PNP)->pn_pathlen)

extern	int	pn_alloc();		/* allocat buffer for pathname */
extern	int	pn_get();		/* allocate buf and copy path into it */
extern	int	pn_set();		/* set pathname to string */
extern	int	pn_combine();		/* combine to pathnames (for symlink) */
extern	int	pn_getcomponent();	/* get next component of pathname */
extern	void	pn_skipslash();		/* skip over slashes */
extern	void	pn_free();		/* free pathname buffer */
