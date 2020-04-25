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
char	rcsid[] = "$Header: prfdc.c 2.1 86/04/07 $";
#endif	lint

/*
 * prfdc.c
 *	kernel profiling: dump counters to log file.
 */

#include "time.h"
#include "signal.h"
#include "stdio.h"
#include "errno.h"
#include "a.out.h"

# define PRF_ON    1
# define PRFMAX  2048

int	buf[PRFMAX + 1];	/* Symbols, Kernel ctrs, and User ctr */
int	prfmax;			/* number of text addresses */

sigalrm()
{
	(void)signal(SIGALRM, sigalrm);
}

main(argc, argv)
	char	**argv;
{
	register  int  prf, log;
	register  int  rate = 10, first = 1, toff = 17;
	int	tvec;
	struct	tm	*localtime();
	unsigned long prfsymp;
	unsigned long prfctrp;
	unsigned long prfmaxp;	
	struct nlist nl[4];

	switch(argc) {
		default:
			printf("usage: prfdc  logfile  [ rate  [ off_hour ] ]\n");
			exit(1);
		case 4:
			toff = atoi(argv[3]);
		case 3:
			rate = atoi(argv[2]);
		case 2:
			;
	}

	if (rate <= 0) {
		printf("invalid sampling rate\n");
		exit(1);
	}

	if ((prf = open("/dev/mem", 0)) < 0) {
		printf("cannot open /dev/mem/n");
		exit(1);
	}

	if (open(argv[1], 0) >= 0) {
		printf("existing file would be truncated\n");
		exit(1);
	}

	if ((log = creat(argv[1], 0666)) < 0) {
		printf("cannot creat log file\n");
		exit(1);
	}

	nl[0].n_un.n_name = "_prfmax";
	nl[1].n_un.n_name = "_prfsym";
	nl[2].n_un.n_name = "_prfctr";
	bzero((char *)&nl[3], sizeof(struct nlist));
	nlist("/dynix", nl);
	prfmaxp = nl[0].n_value;
	prfsymp = nl[1].n_value;
	prfctrp = nl[2].n_value;

	if (prfmaxp == 0) {
		printf("Profiler not installed\n");
		exit(1);
	}

	(void)lseek(prf, (long)prfmaxp, 0);
	(void)read(prf, (char *)&prfmax, sizeof prfmax);
	(void)write(log, (char *)&prfmax, sizeof prfmax);

	if (fork())
		exit(0);
	(void)setpgrp(0,0);
	sigalrm();

	for(;;) {
		(void)alarm((unsigned)60 * rate);
		(void)time(&tvec);
		(void)lseek(prf, (long)prfctrp, 0);
		(void)read(prf, (char *)buf, (prfmax + 1) * sizeof (int));
		if(first) {
			(void)lseek(prf, (long)prfsymp, 0);
			(void)read(prf, (char *)buf, (prfmax + 1) * sizeof (int));
			(void)write(log, (char *)buf, prfmax * sizeof (int));
			(void)lseek(prf, (long)prfctrp, 0);
			(void)read(prf, (char *)buf, (prfmax + 1) * sizeof (int));
			first = 0;
		}
		(void)write(log, (char *)&tvec, sizeof tvec);
		(void)write(log, (char *)buf, (prfmax + 1) * sizeof (int));
		if(localtime(&tvec)->tm_hour == toff)
			exit(0);
		(void)pause();
	}
}

