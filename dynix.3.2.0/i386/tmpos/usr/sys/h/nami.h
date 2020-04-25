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
 * $Header: nami.h 2.0 86/01/28 $
 */

/* $Log:	nami.h,v $
 */

struct namidata {
	int		ni_offset;
	int		ni_count;
	struct inode	*ni_pdir;
	struct direct	ni_dent;
};

enum nami_op { NAMI_LOOKUP, NAMI_CREATE, NAMI_DELETE };

/* this is temporary until the namei interface changes */
#define	LOOKUP		0	/* perform name lookup only */
#define	CREATE		1	/* setup for file creation */
#define	DELETE		2	/* setup for file deletion */
#define	LOCKPARENT	0x10	/* see the top of namei */
