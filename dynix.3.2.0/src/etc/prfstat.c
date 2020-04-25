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
char	rcsid[] = "$Header: prfstat.c 2.1 86/04/07 $";
#endif	lint

/*
 * prfstat.c
 *	kernel profiling: turn profiling on/off, or report state.
 */

#include "stdio.h"
#include "errno.h"
#include "a.out.h"

#define	PRF_ON	 1

main(argc, argv)
	int argc;
	char	**argv;
{

	register int prf;
	unsigned prfstat;
	unsigned prfmax;
	unsigned long prfstatp;
	unsigned long prfmaxp;
	struct nlist nl[3];

	if((prf = open("/dev/mem", 2)) < 0) {
		printf("cannot open /dev/mem\n");
		exit(1);
	}

	nl[0].n_un.n_name = "_prfstat";
	nl[1].n_un.n_name = "_prfmax";
	bzero((char *)&nl[2], sizeof(struct nlist));
	nlist("/dynix", nl);
	prfstatp = nl[0].n_value;
	prfmaxp = nl[1].n_value;

	if (prfstatp == 0) {
		printf("Profiler not installed\n");
		exit(1);
	}

	(void)lseek(prf, (long)prfmaxp, 0);
	(void)read(prf, (char *)&prfmax, sizeof prfmax);
	if (prfmax == 0) {
		printf("text addresses not loaded\n");
		exit(1);
	}
			
	if (argc > 2) {
		printf("usage: prfstat  [ on ]  [ off ]\n");
		exit(1);
	}

	if (argc == 2) {
		if (strcmp("off", argv[1]) == 0) {
			(void)lseek(prf, (long)prfstatp, 0);
			(void)write(prf, (char *)"0", sizeof prfstat);
		}
		else if (strcmp("on", argv[1]) == 0) {
			(void)lseek(prf, (long)prfstatp, 0);
			(void)write(prf, (char *)"1", sizeof prfstat);
		}
		else {
			printf("eh?\n");
			exit(1);
		}
	}

	(void)lseek(prf, (long)prfstatp, 0);
	(void)read(prf, (char *)&prfstat, sizeof prfstat);

	printf("profiling %s\n", prfstat & PRF_ON ? "enabled" : "disabled");
	(void)lseek(prf, (long)prfmaxp, 0);
	(void)read(prf, (char *)&prfmax, sizeof prfmax);
	printf("%d kernel text addresses\n", prfmax);

	(void)close(prf);

}
