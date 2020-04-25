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
static	char	rcsid[] = "$Header: BmapCache.c 1.3 90/07/31 $";
#endif

#include <syscall.h>
#include "dbsupport.h"

BmapCache(fd, flavor)
	int	fd;
	int	flavor;
{
	return syscall(SYS_dbsupport, DB_BMAPCACHE, fd, flavor);
}
