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
static char rcsid[] = "$Header: vmstat.c 2.2 87/02/09 $";
#endif

/*
 * vmstat - print virtual memory statistics.
 *
 * This was ported to the Balance 8000, and vestigial sun and vax
 * code was removed.
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/vm.h>
#include <sys/time.h>
#include <sys/dk.h>
#include <nlist.h>
#include <sys/buf.h>
#include <machine/pte.h>
#include <machine/plocal.h>
#include <machine/engine.h>

/*
 * The utod macro is a workaround for not being able to
 * convert from unsigned to double via movdl as generated by compiler
 */
#define	utod(u)		((double) ((u)/2) * 2 + ((u)&1))
/*
 * The so macro handles the difference of two unsigned's in the
 * presence of wraparound
 */
#define MAXUNSIGNED	4294967295
#define	so(n,o)		(((n) >= (o)) ? ((n)-(o)) : (MAXUNSIGNED-(o)+(n)+1))

struct nlist nl[] = {
#define X_ENGINE	0
	{ "_engine" },
#define X_NENGINE	1
	{ "_Nengine" },
#define X_NONLINE	2
	{ "_nonline" },
#define X_TOTAL		3
	{ "_total" },
#define	X_DEFICIT	4
	{ "_deficit" },
#define	X_BOOTTIME	5
	{ "_boottime" },
#define	X_DK		6
	{ "_dk" },
#define X_DK_NDRIVES	7
	{ "_dk_ndrives" },
#define X_DK_NXDRIVE	8
	{ "_dk_nxdrive" },
#define X_HZ		9
	{ "_hz" },
#define X_AVEDIRTY	10
	{ "_avedirty" },
	{ "" },
};

int	hz;
struct
{
	struct dk	*dk;
	struct	vmmeter cnt;
	unsigned rectime;
	unsigned pgintime;
} s[2];

struct	vmtotal	total;

int	deficit;
double ttime;
double	etime;
int 	mf;
int	swflag;
struct engine *engine;		/* array of engine structures */
struct engine *pengine;		/* kernel addresses of engine structures */
struct plocal **pplocal;		/* array of plocal pointers */
unsigned Nengine, nonline;
int	dk_ndrives, dk_nxdrive;
int	avedirty;

#define STEAL(offset, addr, size) \
	if (offset) { lseek(mf, (long)offset, 0); read(mf, (char *)addr, size); } \
	else bzero((char *)addr, size);

main(argc, argv)
	int argc;
	char **argv;
{
	register i,j;
	int new, old;		/* indices for snapshots */
	time_t now;
	int lines;
	extern char *ctime();
	int iter, nintv, intv;
	time_t boottime;
	double t;
	extern char _sobuf[];

	setbuf(stdout, _sobuf);
	nlist("/dynix", nl);
	if(nl[X_NENGINE].n_type == 0) {
		printf("no /dynix namelist\n");
		exit(1);
	}
	mf = open("/dev/kmem", 0);
	if(mf < 0) {
		printf("cannot open /dev/kmem\n");
		exit(1);
	}
	STEAL(nl[X_NENGINE].n_value, &Nengine, sizeof Nengine);
	STEAL(nl[X_DK_NDRIVES].n_value, &dk_ndrives, sizeof dk_ndrives);
	STEAL(nl[X_DK_NXDRIVE].n_value, &dk_nxdrive, sizeof dk_nxdrive);
	STEAL(nl[X_HZ].n_value, &hz, sizeof hz);
	STEAL(nl[X_ENGINE].n_value, &pengine, sizeof pengine);
	engine = (struct engine *)calloc(Nengine, sizeof(struct engine));
	STEAL(pengine, &engine[0], Nengine*sizeof(struct engine));
	pplocal = (struct plocal **)calloc(Nengine, sizeof(struct plocal *));
	for (i = 0; i < Nengine; i++) {
		pplocal[i] = (struct plocal *)&engine[i].e_local->pp_local[0][0];
	}
	s[0].dk = (struct dk *)calloc(dk_ndrives?dk_ndrives:1, sizeof(struct dk));
	s[1].dk = (struct dk *)calloc(dk_ndrives?dk_ndrives:1, sizeof(struct dk));
	iter = 0;
	argc--, argv++;
	while (argc>0 && argv[0][0]=='-') {
		char *cp = *argv++;
		argc--;
		while (*++cp) switch (*cp) {

		case 'S':
			swflag = 1 - swflag;
			break;

#ifdef notyet
		case 't':
			dotimes();
			exit(0);
#endif notyet

		case 'f':
			doforkst();
			exit(0);
		
		case 's':
			dosum();
			exit(0);

		default:
			fprintf(stderr, "usage: vmstat [ -fs ] [ interval ] [ count]\n");
			exit(1);
		}
	}
	if (argc >= 1)
		intv = atoi(argv[0]);
	if(argc > 1)
		iter = atoi(argv[1]);
	STEAL((long)nl[X_BOOTTIME].n_value, &boottime, sizeof boottime);
	time(&now);
	nintv = now - boottime;
	if (nintv <= 0 || nintv > 60*60*24*365*10) {
		printf("Time makes no sense... namelist must be wrong.\n");
		exit(1);
	}

	new = 0;
	old = 1;
	STEAL(nl[X_DK].n_value, &s[new].dk[0], dk_ndrives*sizeof(struct dk));

reprint:
	lines = 20;
	printf(" procs           memory                 page ");
	if (dk_nxdrive > 0) {
		for (i = 0; i < dk_nxdrive-2; ++i)
			printf("   ");
		printf(" disk ");
	}
	printf(" faults          cpu\n\
 r b w   avm   fre   di  %7s  pi  po  de ",
		swflag ? " si  so" : " re  rd");
 	if (dk_nxdrive > 0) {
		if (dk_nxdrive == 1) printf("   ");
		for (i = 0; i < dk_nxdrive; i++)
			printf("%c%c ", s[new].dk[i].dk_name[0],
			    s[new].dk[i].dk_name[strlen(s[new].dk[i].dk_name)-1]);
	}
	printf(" in  sy  cs us sy id\n");
loop:
	fill_cnt( &s[new].cnt );	/* Fill the vmmeter structure */

	STEAL(nl[X_DK].n_value, &s[new].dk[0], dk_nxdrive*sizeof(struct dk));
	STEAL(nl[X_TOTAL].n_value, &total, sizeof total);
	STEAL(nl[X_DEFICIT].n_value, &deficit, sizeof deficit);
	STEAL(nl[X_AVEDIRTY].n_value, &avedirty, sizeof avedirty);
	STEAL(nl[X_NONLINE].n_value, &nonline, sizeof nonline);
	if (nonline == 0)
		nonline = 1;
	ttime = 0;
	for (i=0; i < CPUSTATES; i++) {
		ttime += utod(so(s[new].cnt.v_time[i],s[old].cnt.v_time[i]));
	}
	if(ttime == 0.0) {
		etime = 1.0;
		ttime = 1.0;
	} else
		etime = ttime / (double) (nonline * hz);
	printf("%2d%2d%2d", total.t_rq, total.t_dw+total.t_pw, total.t_sw);
#define pgtok(a) ((a)*NBPG/1024)
	printf("%6d%6d", pgtok(total.t_avm), pgtok(total.t_free));
	printf("%5d", pgtok(avedirty));
#define rate(fld)	((int) (so(s[new].cnt.fld,s[old].cnt.fld) / etime+0.5))
	printf("%5d%4d",
	    swflag ?
	        so(s[new].cnt.v_swpin,s[old].cnt.v_swpin) :
	        rate(v_pgrec),
	    swflag ?
		so(s[new].cnt.v_swpout,s[old].cnt.v_swpout) :
	        rate(v_pgdrec));
	printf("%4d", pgtok(rate(v_pgpgin)));
	printf("%4d%4d", pgtok(rate(v_pgpgout)), pgtok(deficit));
	if (dk_nxdrive == 1)
		printf("   ");
	for(i = 0; i < dk_nxdrive; i++)
		printf("%3d",((int)(so(s[new].dk[i].dk_xfer,s[old].dk[i].dk_xfer)/etime + 0.5)));
	printf("%4d%4d", rate(v_intr), rate(v_syscall));
	printf("%4d", rate(v_swtch));
	for(i=0, t = 0.0; i<CPUSTATES; i++) {
		t += utod(so(s[new].cnt.v_time[i],s[old].cnt.v_time[i]));
		if (i == 0)		/* US+NI */
			continue;
		printf("%3d", ((int) (t*100./ttime + 0.5)));
		t = 0.0;
	}
	printf("\n");
	fflush(stdout);
contin:
	old = new;	/* Flip the two indices */
	new = !old;
	--iter;
	if(iter)
	if(argc > 0) {
		sleep(intv);
		if (--lines <= 0)
			goto reprint;
		goto loop;
	}
}

#ifdef notyet
dotimes()
{

	STEAL(nl[X_REC].n_value, &s.rectime, sizeof s.rectime);
	STEAL(nl[X_PGIN].n_value, &s.pgintime, sizeof s.pgintime);
	STEAL(nl[X_SUM].n_value, &sum, sizeof sum);
	printf("%d reclaims, %d total time (usec)\n", sum.v_pgrec, s.rectime);
	printf("average: %d usec / reclaim\n", s.rectime/sum.v_pgrec);
	printf("\n");
	printf("%d page ins, %d total time (msec)\n",sum.v_pgin, s.pgintime/10);
	printf("average: %8.1f msec / page in\n", s.pgintime/(sum.v_pgin*10.0));
}
#endif notyet

dosum()
{

#define sum	s[0].cnt
	fill_cnt( &s[0].cnt );	/* Fill the vmmeter structure */
	printf("%9d swap ins\n", sum.v_swpin);
	printf("%9d swap outs\n", sum.v_swpout);
	printf("%9d pages swapped in\n", sum.v_pswpin / CLSIZE);
	printf("%9d pages swapped out\n", sum.v_pswpout / CLSIZE);
	printf("%9d total address trans. faults taken\n", sum.v_faults);
	printf("%9d page ins\n", sum.v_pgin);
	printf("%9d page outs\n", sum.v_pgout);
	printf("%9d pages paged in\n", sum.v_pgpgin);
	printf("%9d pages paged out\n", sum.v_pgpgout);
	printf("%9d total reclaims (%d%% fast)\n", sum.v_pgrec,
	    (sum.v_pgfrec * 100) / (sum.v_pgrec == 0 ? 1 : sum.v_pgrec));
	printf("%9d reclaims from free list\n", sum.v_pgfrec);
	printf("%9d dirty page reclaims\n", sum.v_pgdrec);
	printf("%9d intransit blocking page faults\n", sum.v_intrans);
	printf("%9d zero fill pages created\n", sum.v_nzfod / CLSIZE);
	printf("%9d zero fill page faults\n", sum.v_zfod / CLSIZE);
	printf("%9d executable fill pages created\n", sum.v_nexfod / CLSIZE);
	printf("%9d executable fill page faults\n", sum.v_exfod / CLSIZE);
	printf("%9d pages freed by the pageout daemon\n", sum.v_dfree / CLSIZE);
	printf("%9d cpu context switches\n", sum.v_swtch);
	printf("%9d device interrupts\n", sum.v_intr);
	printf("%9d traps\n", sum.v_trap);
	printf("%9d system calls\n", sum.v_syscall);
	printf("%9d page table entries realloc'd before reclaimed\n", sum.v_realloc);
	printf("%9d page table entries rebuilt to fill on demand\n", sum.v_redofod);
#undef sum
}

doforkst()
{
	struct plocal plocal;
	unsigned cntfork, cntvfork, sizfork, sizvfork;
	int i;

	cntfork = cntvfork = 0;
	sizfork = sizvfork = 0;
	for (i = 0; i < Nengine; ++i) {
		STEAL(pplocal[i], &plocal, sizeof plocal);
		cntfork += plocal.cnt.v_cntfork;
		cntvfork += plocal.cnt.v_cntvfork;
		sizfork += plocal.cnt.v_sizfork;
		sizvfork += plocal.cnt.v_sizvfork;
	}
	printf("%d forks, %d pages, average=%.2f\n",
		cntfork, sizfork, (double) sizfork / cntfork);
	printf("%d vforks, %d pages, average=%.2f\n",
		cntvfork, sizvfork, (double)sizvfork / cntvfork);
}

fill_cnt( vp )
struct vmmeter *vp;
{
	register unsigned *cp, *sp;
	register int i;
	struct plocal plocal;

	bzero((caddr_t)vp, sizeof(struct vmmeter));
	for (i = 0; i < Nengine; i++) {
		STEAL(pplocal[i], &plocal, sizeof plocal);
		for ( sp = &(vp->v_first), cp = &plocal.cnt.v_first;
		      cp <= &plocal.cnt.v_last; cp++, sp++ ) {
			*sp += *cp;
		}
	}
}
