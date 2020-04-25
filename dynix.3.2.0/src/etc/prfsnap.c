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
char	rcsid[] = "$Header: prfsnap.c 2.1 86/04/07 $";
#endif	lint

/*
 * prfsnap.c
 *	kernel profiling: snapshot counters.
 */

#include "stdio.h"
#include "errno.h"
#include "a.out.h"

# define PRFMAX 2048

int	buf[PRFMAX + 1];
int	prfmax;

main(argc, argv)
	int	argc;
	char	**argv;
{
	register  int  prf, log;
	int	tvec;
	unsigned prfstat;
	int prfstatp;
	int prfmaxp;
	int prfsymp;
	int prfctrp;
	struct nlist nl[5];

	if (argc != 2) {
		printf("usage: prfsnap logfile\n");
		exit(1);
	}

	if ((prf = open("/dev/mem", 0)) < 0) {
		printf("cannot open /dev/mem\n");
		exit(1);
	}

	if ((log = open(argv[1], 1)) < 0)
		if((log = creat(argv[1], 0666)) < 0) {
			printf("cannot creat log file");
			exit(1);
	}	

	nl[0].n_un.n_name = "_prfmax";
	nl[1].n_un.n_name = "_prfsym";
	nl[2].n_un.n_name = "_prfctr";
	nl[3].n_un.n_name = "_prfstat";
	bzero((char *)&nl[4], sizeof(struct nlist));
	nlist("/dynix", nl);
	prfmaxp = nl[0].n_value;
	prfsymp = nl[1].n_value;
	prfctrp = nl[2].n_value;
	prfstatp = nl[3].n_value;

	if (prfmaxp == 0) {
		printf("Profiler not installed\n");
		exit(1);
	}

	(void)lseek(prf, (long)prfstatp, 0);
	(void)read(prf, (char *)&prfstat, sizeof prfstat);
	if (prfstat == 0) {
		printf("Profiler not enabled");
		exit(1);
	}

	(void)lseek(prf, (long)prfmaxp, 0);
	(void)read(prf, (char *)&prfmax, sizeof prfmax);

	(void)lseek(log, (long)0, 2);
	(void)time(&tvec);
	if (lseek(log, (long)0, 1) == 0) {
		(void)lseek(prf, (long)prfsymp, 0);
		(void)read(prf, (char *)buf, (prfmax +1) * sizeof (int));
		(void)write(log, (char *)&prfmax, sizeof prfmax);
		(void)write(log, (char *)buf, prfmax * sizeof (int));
	}

	(void)lseek(prf, (long)prfctrp, 0);
	(void)read(prf, (char *)buf, (prfmax + 1) * sizeof (int));
	(void)write(log, (char *)&tvec, sizeof tvec);
	(void)write(log, (char *)buf, (prfmax + 1) * sizeof (int));

	(void)close(log);
	(void)close(prf);

}

