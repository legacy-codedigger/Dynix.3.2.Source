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
static	char	rcsid[] = "$Header: pmap.c 2.0 86/01/28 $";
#endif

/*
 * pmap.c
 *	Programmatic interface to pmap driver ioctl's.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <machine/pmap.h>

usage(prog)
{
	fprintf(stderr, "Usage: %s file [paddr size [npmem|phys]].\n", prog);
	fprintf(stderr, "Paddr and size in hex.  Defaults to physical.\n", prog);
	exit(1);
}

main(argc, argv)
	int	argc;
	char	**argv;
{
	struct	pmap_ioc pioc;
	int	fd;
	int	flags = 0;

	switch(argc) {

	case 5:					/* set parameters, NPMEM */
		if (strcmp("npmem", argv[4]) == 0)
			flags = PMAP_NPMEM;
		else if (strcmp("phys", argv[4]) != 0)
			usage(argv[0]);
		/* fall into... */

	case 4:					/* set parameters */
		fd = open(argv[1], 2);
		if (fd < 0) {
			perror(argv[1]);
			exit(2);
		}
		(void) sscanf(argv[2], "%X", &pioc.pi_paddr);
		(void) sscanf(argv[3], "%X", &pioc.pi_size);
		pioc.pi_flags = flags;

		if (ioctl(fd, PMAPIOCSETP, &pioc) < 0) {
			perror(argv[1]);
			exit(1);
		}
		report(argv[1], fd);
		break;

	case 2:						/* report on unit */
		fd = open(argv[1], 2);
		if (fd < 0) {
			perror(argv[1]);
			exit(2);
		}
		report(argv[1], fd);
		break;

	default:
		usage(argv[0]);
	}

	exit(0);
}

report(file, fd)
	char	*file;
	int	fd;
{
	struct	pmap_ioc pioc;

	if (fd < 0 || ioctl(fd, PMAPIOCGETP, &pioc) < 0) {
		perror(file);
		exit(1);
	}
	printf("%s: paddr=0x%x, size=0x%x", file, pioc.pi_paddr, pioc.pi_size);
	if (pioc.pi_flags & PMAP_NPMEM)
		printf(", non-paged memory");
	else
		printf(", physical");
	if (pioc.pi_flags & PMAP_EXCL)
		printf(", exclusive");
	if (pioc.pi_flags & PMAP_MAPPED)
		printf(", mapped");
	printf(".\n");
}
