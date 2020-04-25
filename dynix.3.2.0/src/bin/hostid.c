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
static char rcsid[] = "$Header: hostid.c 2.0 86/01/28 $";
#endif

main(argc, argv)
	int argc;
	char **argv;
{

	if (argc > 1) {
		int hostid;
		sscanf(argv[1], "%x", &hostid);
		if (sethostid(hostid) < 0)
			perror("hostid");
	} else
		printf("%x\n", gethostid());
}
