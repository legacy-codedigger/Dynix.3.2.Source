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
static char rcsid[] = "$Header: mrinval.c 1.1 89/09/29 $";
#endif

#include <stdio.h>
#include <strings.h>
#include <sys/time.h>

char *raw_dev(), *malloc(), *arg0;

main(argc, argv)
	char **argv;
{
	int i, errors = 0;
	
	arg0 = argv[0];

	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s file ...\n", arg0);
		exit( 1 );
	}

	for (i = 1; i < argc; i++)
		if ( utimes( raw_dev(argv[i]), (struct timeval *)0) )
		{
			fprintf( stderr, "%s:  ", arg0 );
			perror( argv[i] );
			errors++;
		}

	exit( errors );
}

char *
raw_dev( dev )
	char *dev;
{
	char *result, *r, last, next;

	/*
	 * alloc memory for new string, which may be one longer than the old.
	 * Remember that strlen doesn't count the null at the end, but we must.
	 * Thus, we allocate a chunk that is two larger than the string length.
	 */
	if ( (result = malloc((unsigned)(strlen(dev) + 2))) == NULL )
	{
		fprintf(stderr, "%s: out of memory\n", arg0);
		exit(1);
	}
	(void)strcpy(result, dev);

	if ( (r=rindex(result, '/')) == NULL )
		r = result-1;
	r++;

	if ( *r != 'r' )
	{
		for ( last='r', next= *r; last; *r=last, last=next, next= *++r)
			;
		*r = last;
	}
	return result;
}
