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
#ifndef _SYS_UN_INCLUDED
#define _SYS_UN_INCLUDED

/*
 * $Header: un.h 2.2 90/05/25 $
 */

/* $Log:	un.h,v $
 */

/*
 * Definitions for UNIX IPC domain.
 */
struct	sockaddr_un {
	short	sun_family;		/* AF_UNIX */
	char	sun_path[108];		/* path name (gag) */
};

#ifdef KERNEL
extern	int	unp_sendspace;
extern	int	unp_recvspace;
#endif
#endif	/* _SYS_UN_INCLUDED */
