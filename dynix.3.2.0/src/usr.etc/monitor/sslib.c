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
static char rcsid[] = "$Header: sslib.c 2.6 87/04/29 $";
#endif

/*
 * sslib.c
 *
 * Library routines to get performance monitors
 */
#include <sys/file.h>
#include <sys/param.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/vm.h>
#include <sys/dk.h>
#include <sys/buf.h>
#include <machine/pte.h>
#include <machine/plocal.h>
#include <machine/engine.h>
#include <sys/proc.h>
#include <sys/mbuf.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/flock.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <net/if_arp.h>
#include <netinet/if_ether.h>
#include <sec/sec.h>
#include <netif/if_se.h>
#include <stdio.h>
#include <nlist.h>
#include "sslib.h"

char *namelist = "/dynix";	/* default kernel namelist file */

struct nlist nl[] = {
#define X_NENGINE	0
	{ "_Nengine" },
#define	X_DK		1
	{ "_dk" },
#define X_DK_NXDRIVE	2
	{ "_dk_nxdrive" },
#define X_ENGINE	3
	{ "_engine" },
#define X_NONLINE	4
	{ "_nonline" },
#define X_NPROC		5
	{ "_nproc" },
#define X_PROC		6
	{ "_proc" },
#define X_PROCMAX	7
	{ "_procmax" },
#define	X_SE_MAX_UNIT	8
	{ "_se_max_unit" },
#define	X_SE_STATE	9
	{ "_se_state" },
#define	X_TOTAL		10
	{ "_total" },
#define	X_AVENRUN	11
	{ "_avenrun" },
#define	X_AVEDIRTY	12
	{ "_avedirty" },
#define	X_DEFICIT	13
	{ "_deficit" },
#define	X_FLCKINFO	14
	{ "_flckinfo" },
	{ "" },
};


#define STEAL(offset, addr, size) \
	if (offset) { lseek(kmem, (off_t)offset, 0); read(kmem, (char *)addr, (int)size); } \
	else bzero((char *)addr, size);

extern char *calloc();
extern char *whoiam;

int kmem;
struct engine *engine;	/* array of engine structures */
struct engine *pengine;	/* kernel addresses of engine structures */
struct dk *dk;		/* array of dk structures */
unsigned Nengine, nonline, dk_nxdrive, nse_unit;
struct se_state *se_softp, *Kse_state;

struct proc *procp;
struct proc *procmax;
struct proc *proc;
unsigned nproc;
int pagesize;

struct ppriv_pages *pprivp, *ppriv;
struct flckinfo *flckinfop;

get_sscfg(sc)
	struct ss_cfg *sc;
{
	extern char *mapkmem();

	/*
	 * Do the one time setup stuff
	 */
	if ((kmem = open("/dev/kmem", 0)) < 0) {
		fprintf(stderr, "%s: ", whoiam);
		perror("cannot open /dev/kmem");
		exit(1);
	}
	if (nlist(namelist, nl) < 0 || nl[X_NENGINE].n_type == 0) {
		fprintf(stderr, "%s: no namelist\n", whoiam);
		exit(1);
	}
	STEAL(nl[X_NENGINE].n_value, &Nengine, sizeof Nengine);
	STEAL(nl[X_DK_NXDRIVE].n_value, &dk_nxdrive, sizeof dk_nxdrive);
	STEAL(nl[X_NONLINE].n_value, &nonline, sizeof nonline);
	if (nonline == 0)
		nonline = 1;

	STEAL(nl[X_ENGINE].n_value, &pengine, sizeof pengine);
	engine = (struct engine *)calloc(Nengine, sizeof(struct engine));
	if (engine == (struct engine *)NULL) {
		fprintf(stderr, "%s: No space for engine table\n", whoiam);
		exit(1);
	}
	STEAL(pengine, &engine[0], Nengine*sizeof(struct engine));
	/*
	 * Get the scsi/ether setup info (ptr to soft state and # se's)
	 */
	STEAL(nl[X_SE_MAX_UNIT].n_value, &nse_unit, sizeof(nse_unit));
	STEAL(nl[X_SE_STATE].n_value, &Kse_state, sizeof(Kse_state));
	/*
	 * Get the process table stuff
	 */
	STEAL(nl[X_NPROC].n_value, &nproc, sizeof(nproc));
	STEAL(nl[X_PROC].n_value, &procp, sizeof(procp));
	pagesize = getpagesize();

	proc = (struct proc *) mapkmem((char *)procp,
				(int)nproc * sizeof(struct proc));

	ppriv = (struct ppriv_pages *) mapkmem((char *)engine[0].e_local,
				(int)Nengine * SZPPRIV);

	se_softp = (struct se_state *) mapkmem((char *)Kse_state,
				(int)nse_unit * sizeof(struct se_state));

	flckinfop = (struct flckinfo *) mapkmem((char *)nl[X_FLCKINFO].n_value,
				sizeof(struct flckinfo));

	free((char *)engine);

	/*
	 * Fill out the configuration stuff
	 */
	sc->Nengine = Nengine;
	sc->nonline = nonline;
	sc->dk_nxdrive = dk_nxdrive;
	sc->nse_unit = nse_unit;
}

char *
mapkmem(kernaddr, size)
	char *kernaddr;
	int size;
{
	extern char *sbrk();
	off_t pos;
	int sz, off;
	char *va;

	va = (char *)(((unsigned)sbrk(0) + (pagesize-1)) &~ (pagesize-1));
	pos = (unsigned)kernaddr & ~(pagesize-1);
	off = (unsigned)kernaddr - pos;
	sz = size + off;
	sz = (sz+pagesize-1) & ~(pagesize-1); 
	if (mmap(va, sz, PROT_READ, MAP_SHARED, kmem, pos) != 0) {
		fprintf(stderr, "%s: mmap: ", whoiam);
		perror("");
		exit(1);
	}
	return((char *) (va + off));
}

/*
 * Sample the data
 * nonline is also sampled here in case someone did online/offline.
 */

ss_sample(sc, vm, dp, ep, ps, vmtot, avenrun, avedirtyp, deficitp, flp)
	struct ss_cfg *sc;
	register struct vmmeter *vm;
	struct dk *dp;
	struct ether_stat *ep;
	struct proc_stat *ps;
	struct vmtotal *vmtot;
	double *avenrun;
	unsigned *avedirtyp, *deficitp;
	struct flckinfo *flp;
{
	register int i;
	register struct plocal *plp;
	register struct ppriv_pages *prp; 
	long l_avenrun[3]; 

	for (i = 0, prp = ppriv; i < sc->Nengine; i++, prp++) {
		plp = (struct plocal *) prp->pp_local;
		*vm = plp->cnt;
		vm++;
	}
	STEAL(nl[X_TOTAL].n_value, vmtot, sizeof(struct vmtotal));
	STEAL(nl[X_NONLINE].n_value, &nonline, sizeof(nonline));
	sc->nonline = nonline;

	STEAL(nl[X_DK].n_value, dp, dk_nxdrive*sizeof(struct dk));
	getnwstat(sc, ep);
	getpstats(ps);
	STEAL(nl[X_AVENRUN].n_value, l_avenrun, sizeof(l_avenrun));
	for (i = 0; i < 3; i++) {
		*avenrun++ = ((double)l_avenrun[i])/1000.0;
	}
	STEAL(nl[X_AVEDIRTY].n_value, avedirtyp, sizeof(*avedirtyp));
	STEAL(nl[X_DEFICIT].n_value, deficitp, sizeof(*deficitp));
	*flp = *flckinfop;
}

/*
 *	Read kernel networking stats.
 */
getnwstat(sc, ep)
	struct ss_cfg *sc;
	register struct ether_stat *ep;
{
	register int unit;
	register struct se_state *sp;

	sp = se_softp;
	for (unit = 0; unit < sc->nse_unit; ++unit, ep++, sp++) {
		if (sp->ss_alive) {
			ep->pktin = sp->ss_arp.ac_if.if_ipackets;
			ep->pktout = sp->ss_arp.ac_if.if_opackets;
		}
	}
}


ss_sum(sc, sd, sum)
	struct ss_cfg *sc;		/* config stuff */
	register struct vmmeter *sd;	/* per processor data */
	struct vmmeter *sum;		/* put the sum here */
{
	register int i;
	register unsigned *sp, *dp;

	bzero((char *)sum, sizeof(struct vmmeter));
	/*
	 * Calculate the sum
	 */
	for (i = 0; i < sc->Nengine; i++, sd++) {
		sp = &(sum->v_first);
		for (dp = &(sd->v_first); dp <= &(sd->v_last); dp++, sp++)
			*sp += *dp;
	}
}


getpstats(ps)
	register struct proc_stat *ps;
{
	register struct proc *p, *l_procmax;
	int i_procs;	/* # of interesting proc table slots */

	STEAL(nl[X_PROCMAX].n_value, &procmax, sizeof(procmax));
	i_procs = procmax - procp;
	l_procmax = proc + i_procs;
	/*
	 * collect process specific data.
	 */
	ps->processes = 0;
	ps->running = 0;
	ps->runnable = 0;
	ps->fastwait = 0;
	ps->sleeping = 0;
	ps->swapped = 0;
	for(p=proc; p < l_procmax; ++p) {
		if (p->p_stat == 0 || p->p_flag & SSYS)
			continue;
		ps->processes++;
		switch (p->p_stat) {
		case SSLEEP:
		case SSTOP:
		case SWAIT:
		case SZOMB:
			if (p->p_noswap) {		/* approx */
				ps->fastwait++;
			} else if (p->p_flag & SLOAD) {
				if (p->p_pri <= PZERO) {
					ps->fastwait++;
				} else {
					ps->sleeping++;
				}
			} else {
				ps->swapped++;
			}
			break;

		case SONPROC:
			ps->running++;
			break;
		case SRUN:
		case SIDL:
			if (p->p_flag & SLOAD)
				ps->runnable++;
			else
				ps->swapped++;
			break;
		}
	}
}
