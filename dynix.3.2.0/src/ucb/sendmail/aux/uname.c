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

/* $Header: uname.c 2.0 86/01/28 $ */

# include <stdio.h>
# include <sysexits.h>

/*
**  UNAME -- print UNIX system name (fake version)
**
**	For UNIX 3.0 compatiblity.
*/

main(argc, argv)
	int argc;
	char **argv;
{
	char buf[40];

	gethostname(buf, sizeof buf);
	printf("%s\n", buf);
	exit(EX_OK);
}