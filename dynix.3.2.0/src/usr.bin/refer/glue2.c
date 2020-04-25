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

#ifndef lint
static char rcsid[] = "$Header: glue2.c 2.1 1991/05/20 00:15:18 $";
#endif

#include "pathnames.h"
char refdir[50];

savedir()
{
	if (refdir[0]==0)
		corout ("", refdir, _PATH_PWD, "", 50);
	trimnl(refdir);
}

restodir()
{
	chdir(refdir);
}
