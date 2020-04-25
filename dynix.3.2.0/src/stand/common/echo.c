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

#ifdef RCS
static char rcsid[] = "$Header: echo.c 1.1 90/02/07 $";
#endif

main(argc,argv,envp)
int	argc;
char	**argv;
char	*envp;
{
	int	i;

	printf("argc=%d argv=0x%x envp=0x%x\n", argc, argv, envp);

	for (i=0; i<argc; i++ ) {
		printf("argc:%d *(0x%x:0x%x)argv=\"%s\"\n", i, &argv[i], 
				argv[i], argv[i]);
	}
	exit(0);
}
