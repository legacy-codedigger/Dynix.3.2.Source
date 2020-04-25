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
static char rcsid[] = "$Header: iostat.c 2.1 90/02/01 $";
#endif

/*
 * iostat
 *
 * This has been modified extensively for the NS32000
 * and multiprocessor dynix.
 * The vax- and sun-specific stuff are gone now.
 */
#include <stdio.h>
#include <nlist.h>
#include <time.h>
#include <sys/param.h>
#include <sys/dk.h>
#include <sys/buf.h>
#include <machine/pte.h>
#include <sys/vm.h>
#include <machine/plocal.h>
#include <machine/engine.h>

struct nlist nl[] = {
	{ "_dk_ndrives" },
#define	X_DK_NDRIVES	0
	{ "_dk_nxdrive" },
#define	X_DK_NXDRIVE	1
	{ "_dk" },
#define	X_DK		2
	{ "_engine" },
#define	X_ENGINE	3
	{ "_Nengine" },
#define	X_NENGINE	4
	{ "_nonline" },
#define X_NONLINE	5
	{ "_hz" },
#define	X_HZ		6
	{ 0 },
};

struct stats {
	long	cp_time[CPUSTATES];
	struct dk *dk;
	long	ttyin;
	long	ttyout;
} s[2];

int	new, old;		/* indices into struct stats array */
int	mf;
int	hz;
unsigned ttime;
double	etime;
int	dk_ndrives, dk_nxdrive;
unsigned	Nengine, nonline;
struct engine	*engine;	/* array of engine structures */
struct engine	*pengine;	/* kernel addr of array of engine structures */
struct plocal	**pplocal;	/* array of plocal pointers */

#define	STEAL(where, var, size) \
	if ((int)where) { lseek(mf, (long)where, 0); read(mf, (caddr_t)var, size); } \
	else bzero((caddr_t)var, size);

main(argc, argv)
char *argv[];
{
	extern char *ctime();
	register int i, j;
	int iters = 1, sleeptime = 0;
	double f1, f2;
	long t;
	int tohdr = 1;

	nlist("/dynix", nl);
	if(nl[X_NENGINE].n_type == 0) {
		printf("Nengine not found in /dynix namelist\n");
		exit(1);
	}
	mf = open("/dev/kmem", 0);
	if(mf < 0) {
		printf("cannot open /dev/kmem\n");
		exit(1);
	}
	while (argc>1&&argv[1][0]=='-') {
		argc--;
		argv++;
	}
	STEAL(nl[X_NENGINE].n_value, &Nengine, sizeof Nengine);
	STEAL(nl[X_DK_NDRIVES].n_value, &dk_ndrives, sizeof dk_ndrives);
	STEAL(nl[X_DK_NXDRIVE].n_value, &dk_nxdrive, sizeof dk_nxdrive);
	STEAL(nl[X_HZ].n_value, &hz, sizeof hz);
	STEAL(nl[X_NONLINE].n_value, &nonline, sizeof nonline);
	if (nonline == 0)
		nonline = 1;
	STEAL(nl[X_ENGINE].n_value, &pengine, sizeof pengine);
	engine = (struct engine *)calloc(Nengine, sizeof(struct engine));
	STEAL(pengine, &engine[0], Nengine*sizeof(struct engine));
	/*printf("engine structure at 0x%x\n", pengine);*/
	pplocal = (struct plocal **)calloc(Nengine, sizeof(struct plocal *));
	for (i = 0; i < Nengine; i++) {
		pplocal[i] = (struct plocal *)&engine[i].e_local->pp_local[0][0];
	}
	s[0].dk = (struct dk *)calloc(dk_ndrives?dk_ndrives:1, sizeof(struct dk));
	s[1].dk = (struct dk *)calloc(dk_ndrives?dk_ndrives:1, sizeof(struct dk));
	STEAL(nl[X_DK].n_value, &s[0].dk[0], dk_ndrives*sizeof(struct dk));

	if (argc > 1) {
		sleeptime = atoi(argv[1]);
		iters = 0x7FFFFFFF;
	}
	if(argc > 2)
		iters = atoi(argv[2]);
	new = 0;
	old = 1;
	while (--iters >= 0) {
		if (--tohdr == 0) {
			printf("      tty");
			for (i = 0; i < dk_nxdrive; i++)
				if (s[new].dk[i].dk_bps != 0)
					printf("          %3.3s ", s[new].dk[i].dk_name);
			printf("         cpu\n");
			printf(" tin tout");
			for (i = 0; i < dk_nxdrive; i++)
				if (s[new].dk[i].dk_bps != 0)
					printf(" bps tps msps ");
			printf(" us ni sy id\n");
			tohdr = 19;
		}
		for (i = 0; i < CPUSTATES; ++i)
			s[new].cp_time[i] = 0;
		s[new].ttyin = s[new].ttyout = 0;
		for (i = 0; i < Nengine; i++) {
			struct plocal plocal;

			STEAL(pplocal[i], &plocal, sizeof plocal);
			/*printf("cpu %d: ", i);*/
			for (j = 0; j < CPUSTATES; ++j) {
				/*printf(" %d", plocal.cp_time[j]);*/
				s[new].cp_time[j] += plocal.cnt.v_time[j];
			}
			s[new].ttyin += plocal.cnt.v_ttyin;
			s[new].ttyout += plocal.cnt.v_ttyout;
		}
		STEAL(nl[X_DK].n_value, &s[new].dk[0], dk_nxdrive*sizeof(struct dk));
		ttime = 0;
		for(i = 0; i < CPUSTATES; i++) {
			ttime += (s[new].cp_time[i] - s[old].cp_time[i]);
		}
		if (ttime == 0)
			ttime = 1;
		etime = (double) ttime / (double) (hz*nonline);
		printf("%4d %4d",
			((int) ((s[new].ttyin - s[old].ttyin)/etime + 0.5)),
			((int) ((s[new].ttyout - s[old].ttyout)/etime + 0.5)));
		for (i = 0; i < dk_nxdrive; i++)
			if (s[new].dk[i].dk_bps != 0)
				stats(i);
		for (i = 0; i < CPUSTATES; i++)
			printf(" %2d", ((int) (100. * (s[new].cp_time[i] - s[old].cp_time[i]) / (double) ttime + 0.5)));
		printf("\n");
		old = new;		/* Flip the two indices */
		new = !old;
		if(iters && sleeptime)
			sleep(sleeptime);
	}
}

stats(dn)
{
	register i;
	double atime, blks, xtime, itime;
	int seeks;

	if (s[new].dk[dn].dk_bps == 0.0) {
		printf(" %3.0f %3.0f %4.1f ", 0.0, 0.0, 0.0);
		return;
	}
	atime =
	 (double)(s[new].dk[dn].dk_time.tv_sec - s[old].dk[dn].dk_time.tv_sec) +
	 (s[new].dk[dn].dk_time.tv_usec - s[old].dk[dn].dk_time.tv_usec)/1000000.0;
	/* number of blocks transferred */
	blks = s[new].dk[dn].dk_blks - s[old].dk[dn].dk_blks;
	/* number of seeks */
	seeks = s[new].dk[dn].dk_seek - s[old].dk[dn].dk_seek;
	/* transfer time */
	xtime = blks*(s[new].dk[dn].dk_bps - s[old].dk[dn].dk_bps);
	/* time not transferring */
	itime = atime - xtime;
/*
	printf("\ndn %d, blks %8.2f, atime %6.2f, xtime %6.2f, itime %6.2f\n",
	    dn, blks, atime, xtime, itime);
*/
	if (xtime < 0)
		itime += xtime, xtime = 0;
	if (itime < 0)
		xtime += itime, itime = 0;
	printf(" %3.0f", blks/etime);
	printf(" %3.0f", (s[new].dk[dn].dk_xfer - s[old].dk[dn].dk_xfer)/etime);
	printf(" %4.1f ", seeks ? itime*1000./seeks : 0.0);
}

/*
 * Timer arithmetic routines stolen from the kernel.
 */

/*
 * Add and subtract routines for timevals.
 * N.B.: subtract routine doesn't deal with
 * results which are before the beginning,
 * it just gets very confused in this case.
 * Caveat emptor.
 */
timevaladd(t1, t2)
	struct timeval *t1, *t2;
{

	t1->tv_sec += t2->tv_sec;
	t1->tv_usec += t2->tv_usec;
	timevalfix(t1);
}

timevalsub(t1, t2)
	struct timeval *t1, *t2;
{

	t1->tv_sec -= t2->tv_sec;
	t1->tv_usec -= t2->tv_usec;
	timevalfix(t1);
}

timevalfix(t1)
	struct timeval *t1;
{

	if (t1->tv_usec < 0) {
		t1->tv_sec--;
		t1->tv_usec += 1000000;
	}
	if (t1->tv_usec >= 1000000) {
		t1->tv_sec++;
		t1->tv_usec -= 1000000;
	}
}
