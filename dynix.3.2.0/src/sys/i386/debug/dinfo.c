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
static	char	rcsid[] = "$Header: dinfo.c 2.14 87/05/29 $";
#endif

/*
 * dinfo.c
 *	Procedures for various debug info purposes.
 */

/* $Log:	dinfo.c,v $
 */

#ifdef	DEBUG

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/map.h"
#include "../h/vm.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../h/file.h"
#include "../h/clist.h"
#include "../h/cmap.h"
#include "../h/mbuf.h"

#include "../balance/engine.h"
#include "../balance/slic.h"
#include "../balance/slicreg.h"

#include "../machine/pte.h"
#include "../machine/plocal.h"
#include "../machine/mmu.h"
#include "../machine/gate.h"
#include "../machine/intctl.h"
#include "../machine/psl.h"

struct	proc	*D_proc;		/* selected proc[] entry */

/*
 * procdump()
 *	Dump some useful stuff on a process.
 */

procdump(p)
	register struct proc *p;
{
	register spl_t	s;
#define	PF(sep,f)	printf("%c f=%x", sep, p->f)

	printf("<%d>:", p-proc);
	PF('\t',p_stat);
	PF(',',p_flag);
	PF(',',p_ul2pt);
	s = p_lock(&p->p_state, SPLHI);
	if (p->p_flag & SLOAD) {
		printf(", rshand=%x, rscurr=%x", p->p_rshand, p->p_rscurr);
	}
	v_lock(&p->p_state, s);
	printf("\n");

	PF('\t',p_pid);
	PF(',',p_ppid);
	PF(',',p_dsize);
	PF(',',p_ssize);
	PF(',',p_rssize);
	printf("\n");

	PF('\t',p_cursig);
	PF(',',p_sig);
	PF(',',p_sigmask);
	PF(',',p_sigignore);
	PF(',',p_sigcatch);
	printf("\n");
}

/*
 * debugit()
 *	Called with argument character from input junk.
 */

#define	BUMPPARAM(var,min,max,incr) \
		{ extern int var; \
		  if ((var += (incr)) > (max)) var = min; \
		  printf("var=%d\n", var); }

#define	BUMPDEBUG(var,nstate)	BUMPPARAM(var,0,nstate-1,1)

#define	DUMPSTAT(var)	{ extern int var; printf("var=%d\n", var); }

static	char	*help[] = {
	"a:	toggle abtdebug",
	"A:	toggle arp debug",
	"B:	dump buffer stats",
	"e:	toggle ether input debug",
	"E:	toggle ether output debug",
	"f:	print freemem",
	"F:	memory, uptmap stats",
#ifdef	GATESTAT
	"g,G:	gate usage statistics",
#endif	GATESTAT
#ifdef	PERFSTAT
	"i:	inode, file-table, c-list usage stats",
#endif	PERFSTAT
	"I:	toggle putprocid (processor # at start of line)",
	"j:	up select pid",
	"J:	down select pid",
	"k:	kill 9 selected pid",
	"l:	print load-average",
	"m,M:	toggle mbad_debug",
	"p:	info on selected proc[]",
	"P:	info on all existing proc[]'s",
	"s:	toggle syscalltrace",
	"S:	toggle swapdebug",
	"t:	toggle traptrace",
	"T:	stack-trace selected pid",
#ifdef	PERFSTAT
	"v:	some VM usage stats",
	"V:	some VM kluster stats",
#endif	PERFSTAT
	"y:	dump 'total' structure",
	"?:	what you're reading now",
};
#define	NARRAY(array)	(sizeof(array)/sizeof(array[0]))

debugit(c)
	char	c;
{
	register struct proc *p;
	register int	i;
	int	got_mutex;
	extern int freemem;
	extern sema_t dc_mutex;			/* console output mutex */

	got_mutex = cp_sema(&dc_mutex);		/* at least try to mutex! */

	switch(c) {
	case 'a':				/* toggle abtdebug */
		BUMPDEBUG(abtdebug, 2);
		break;
	case 'A':				/* ARP debugging */
		BUMPDEBUG(if_ether_debug, 3);
		break;
	case 'B':				/* dump buffer stats */
		DUMPSTAT(albshrink);
		DUMPSTAT(albgrow);
#ifdef	PERFSTAT
		{
			extern int nswbuf, numswfree, minswfree;
			printf("Nswbuf: %d, cur: %d, min: %d\n",
					nswbuf, numswfree, minswfree);
		}
#endif	PERFSTAT
		break;
	case 'e':				/* ether input debug */
		BUMPDEBUG(se_ibug, 3);
		break;
	case 'E':				/* ether output debug */
		BUMPDEBUG(se_obug, 3);
		break;
	case 'f':				/* report free-memory */
		DUMPSTAT(freemem);
		DUMPSTAT(dirtymem);
		break;
	case 'F':				/* more memory stats */
		DUMPSTAT(freemem);
		DUMPSTAT(dirtymem);
		printf("Ave free=%d, free30=%d, dirty=%d, dirty30=%d\n",
			avefree, avefree30, avedirty, avedirty30);
		DUMPSTAT(deficit);
#ifdef	PERFSTAT
		DUMPSTAT(min_freemem);
		DUMPSTAT(max_dirtymem);
		uptmap_stat();
#endif	PERFSTAT
		break;
#ifdef	GATESTAT
	case 'g':				/* dump gate statistics */
	case 'G':
		dumpgates();
		break;
#endif	GATESTAT
#ifdef	PERFSTAT
	case 'i':				/* stats on file/inode usage */
		{ extern int ninode;
		  extern int nfile;
		  extern int nclist, cfreecount, cfreelow;

			printf("	Max	Cur	Min\n");
			printf("Inode:	%d\n", ninode);
			printf("File:	%d\n", nfile);
			printf("C-list:	%d	%d	%d\n",
					(nclist-1)*CBSIZE, cfreecount, cfreelow);
		}
		break;
#endif	PERFSTAT
	case 'I':				/* toggle putprocid */
		BUMPDEBUG(putprocid, 2);
		break;
	case 'j':				/* up selected proc */
		++D_proc;
		if (D_proc < &proc[1] || D_proc >= procmax)
			D_proc = &proc[1];
		procdump(D_proc);
		break;
	case 'J':				/* down selected proc */
		--D_proc;
		if (D_proc < &proc[1] || D_proc >= procmax)
			D_proc = procmax-1;
		procdump(D_proc);
		break;
	case 'k':				/* hard kill selected process */
		{				/* if want mult signals, make */
						/* this a procedure: D_kill */
			/*
			 * sort of in-line kill1(0, 9, D_proc->p_pid).
			 * Done this way to preserve "spl" (we are at intr
			 * level here if SCSISQMON) and avoid u. ref's.
			 */
			register spl_t s;
			register struct proc *kp;

			s = spl6();
			if (D_proc && (kp = pfind(D_proc->p_pid))) {
				lpsignal(kp, 9);
				v_lock(&kp->p_state, s);
			} else {
				splx(s);
				printf("Process not found.\n");
			}
		}
		break;
	case 'l':				/* print load-average */
		printf("load: %d, %d, %d\n", avenrun[0], avenrun[1], avenrun[2]);
		break;
	case 'm':				/* toggle mbad_debug */
	case 'M':
		BUMPDEBUG(mbad_debug, 2);
		break;
	case 'p':				/* dump selected proc[] */
		if (D_proc == 0)
			D_proc = &proc[1];
		procdump(D_proc);
		break;
	case 'P':				/* dump about everybody */
		printf("proc=%x\n", proc);
		for(p = proc; p < procmax; p++) {
			if (p->p_stat != NULL)
				procdump(p);
		}
		break;
	case 's':				/* toggle sys-call tracing */
		BUMPDEBUG(syscalltrace, 2);
		break;
	case 'S':				/* toggle swapdebug */
		BUMPDEBUG(swapdebug, 3);
		break;
	case 't':				/* toggle trap tracing */
		BUMPDEBUG(traptrace, 3);
		break;
	case 'T':				/* stack-trace D_proc */
		p = D_proc;
		if ((p->p_flag & SLOAD) && (p->p_stat == SSLEEP || p->p_stat == SSTOP))
			dbg_stacktrace(p->p_uarea);
		else
			printf("can't stack-trace process\n");
		break;
#ifdef	PERFSTAT
	case 'v':				/* dump some VM stats */
		printf("pteasy=%d, ptexpand=%d\n",
				swptstat.pteasy, swptstat.ptexpand);
		{
			extern	int	cnt_RS_sfail, cnt_RS_grow, cnt_RS_rfail,
					cnt_RS_shrink, cnt_PFF_call;
			printf("RS calls=%d, grow=%d, shrink=%d, range-fail=%d, space-fail=%d\n",
				cnt_PFF_call, cnt_RS_grow,
				cnt_RS_shrink, cnt_RS_rfail, cnt_RS_sfail);
		}
		{
			extern long min_forkmap;
			printf("forkmap=%d, min_forkmap=%d, maxforkRS=%d\n",
					forkmap, min_forkmap, maxforkRS);
		}
		printf("dmmin=%d, dmmax=%d, maxdmap=%d\n", dmmin, dmmax, maxdmap);
		DUMPSTAT(set_sfswap);
		swapmap_stat();
		break;
	case 'V':				/* dump VM kluster stats */
		{	extern int fod_kcnt[], klocnt[], klicnt[];
			printf("%d pgout_klust, %d pgin_klust\n",
					klocnt[0], klicnt[0]);
			printf("Size	fod	klo	kli\n");
			for (i = 1; i <= KLMAX; i++) {
				printf("%d	%d	%d	%d\n",
					i, fod_kcnt[i], klocnt[i], klicnt[i]);
			}
		}
		break;
#endif	PERFSTAT
	case 'y':				/* dump 'total' structure */
#define	DUMPTOT(f)	printf("f\t%d\n", total.f)
		DUMPTOT(t_rq);
		DUMPTOT(t_dw);
		DUMPTOT(t_pw);
		DUMPTOT(t_sl);
		DUMPTOT(t_sw);
		DUMPTOT(t_vm);
		DUMPTOT(t_avm);
		DUMPTOT(t_rm);
		DUMPTOT(t_arm);
		DUMPTOT(t_free);
		break;
	case '?':
		for(i = 0; i < NARRAY(help); i++)
			printf("%s\n", help[i]);
		break;
	default:
		printf("Use '?' for help.\n");
		break;
	}

	if (got_mutex)
		v_sema(&dc_mutex);
}

#ifdef	GATESTAT
/*
 * dumpgates()
 *	Dump about gate collisions.  See ../machine/gate.c
 *
 * This junk taken from ../test/tgate.c; must track changes to gate
 * DS's to be useful!
 */

struct	gate	{			/* stolen from gate.c */
	short		g_procid;	/* processor currently holding */
	unsigned long	g_tries;	/* P() count */
	unsigned long	g_collide;	/* collision count */
};
extern	struct gate	gate[];

static	unsigned long collide[NUMGATES];
static	unsigned long tries[NUMGATES];

/*
 * a = what % of b, with round up, and scaled using int's
 */
#define	PCTOF(a,b)	((100*(a)+((b)/2))/(b))

dumpgates()
{
	register struct gate *g;
	register int i;
	int	cval, cdelta;
	int	tval, tdelta;
	int	cdeltsum, ctotsum;
	int	tdeltsum, ttotsum;

	ttotsum = tdeltsum = 0;
	ctotsum = cdeltsum = 0;
	printf("\t\tTotals\t\t\tDeltas\n");
	printf("Gate\tTries\tColl's\t%%Coll\tTries\tColl's\t%%Coll's\n");
	for(i = 0, g = gate; g < &gate[NUMGATES]; g++, i++) {
		tval = g->g_tries;
		cval = g->g_collide;
		tdelta = tval - tries[i];
		cdelta = cval - collide[i];
		if (tdelta > 0) {
			printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\n", i,
					tval,   cval,   PCTOF(cval,tval),
					tdelta, cdelta, PCTOF(cdelta,tdelta)
				);
			collide[i] = cval;
			tries[i] = tval;
			tdeltsum += tdelta;
			cdeltsum += cdelta;
			ttotsum += tval;
			ctotsum += cval;
		}
	}
	if (tdeltsum > 0) {
		printf("Totals:\t%d\t%d\t%d\t%d\t%d\t%d\n",
				ttotsum, ctotsum, PCTOF(ctotsum,ttotsum),
				tdeltsum, cdeltsum, PCTOF(cdeltsum,tdeltsum)
			);
	} else
		printf("No change\n");
}
#endif	GATESTAT

#define	PRINTVM(f)	printf("f\t%d\n", vm->f)
dump_vmmeter(vm)
	register struct vmmeter *vm;
{
	PRINTVM(v_swtch	);
	PRINTVM(v_trap	);
	PRINTVM(v_syscall);
	PRINTVM(v_intr	);
	PRINTVM(v_pswpin);
	PRINTVM(v_pswpout);
	PRINTVM(v_pgin	);
	PRINTVM(v_pgout	);
	PRINTVM(v_pgpgin);
	PRINTVM(v_pgpgout);
	PRINTVM(v_intrans);
	PRINTVM(v_pgrec	);
	PRINTVM(v_pgfrec);
	PRINTVM(v_pgdrec);
	PRINTVM(v_exfod	);
	PRINTVM(v_zfod	);
	PRINTVM(v_nexfod);
	PRINTVM(v_nzfod	);
	PRINTVM(v_realloc);
	PRINTVM(v_redofod);
	PRINTVM(v_faults);
	PRINTVM(v_dfree	);
}

/*
 * dbg_stacktrace()
 *	Print process stack-trace given pointer to it's Uarea.
 *
 * Currently, u_ap -> saved ebp,pc (see resume).
 */

dbg_stacktrace(ua)
	struct	user	*ua;
{
	dbg_printstacktrace(ua, (int *) ua->u_sp);
}

/*
 * dbg_printstacktrace() is cloned from printstacktrace()
 */

#define	TOP_KSTK	((int *)(VA_UAREA + UPAGES * NBPG))
#define	BOT_KSTK	((int *)(VA_UAREA + sizeof(struct user)))
#define	POPECX		0x59			/* pops a single argument */
#define	ADDLESP		0xc483			/* addl $xxx, %esp */

#define	RELOC(t,addr,ua)	(t) ((int)(ua) + ((int)(addr) - (int)&u))

dbg_printstacktrace(ua, bp)
	struct user *ua;
	int	*bp;			/* current frame pointer (unrelocated) */
{
	register int *rbp = RELOC(int*,bp,ua);	/* relocated frame pointer */
	int	*newbp = (int *) rbp[0];	/* next frame pointer */
	int	*oldpc = (int *) rbp[1];	/* pc this frame returns to */
	int	*oldap = &rbp[2];		/* point to first argument */
	int	 nargs;				/* number of arguments passed */
	extern	int etext;

	if ((oldpc < &etext) && ((*oldpc & 0xffff) == ADDLESP))
		nargs = ((*oldpc>>16) & 0xff) / 4;
	else if ((oldpc < &etext) && ((*oldpc & 0xff) == POPECX))
		nargs = 1;
	else
		nargs = 5;	/* default for OS */
		
	printf("@ 0x%x call(", oldpc);
	while (nargs) {
		printf("0x%x", *oldap++);
		if (--nargs)
			printf(", ");
	}
	printf(")\n");

	if ((newbp < TOP_KSTK) && (newbp > bp) && (newbp > BOT_KSTK))
		dbg_printstacktrace(ua, newbp);
}

dump_fpu(fpu)
	register struct fpusave *fpu;
{
	register int i;

	printf("fpu_control =	0x%x\n", fpu->fpu_control);
	printf("fpu_status =	0x%x\n", fpu->fpu_status);
	printf("fpu_tag =	0x%x\n", fpu->fpu_tag);
	printf("fpu_ip =	0x%x\n", fpu->fpu_ip);
	printf("fpu_cs =	0x%x\n", fpu->fpu_cs);
	printf("fpu_data_offset= 0x%x\n", fpu->fpu_data_offset);
	for (i = 0; i < 8; i++) {
		printf("fpu_stack[%d] = 0x%x 0x%x 0x%x 0x%x 0x%x\n", i,
			fpu->fpu_stack[i][0],
			fpu->fpu_stack[i][1],
			fpu->fpu_stack[i][2],
			fpu->fpu_stack[i][3],
			fpu->fpu_stack[i][4]
		);
	}
}

/*
 * Dump page-tables (for by-hand check).
 */

dumpPT(l1pt)
	struct pte *l1pt;
{
	register int i, j;
	register struct pte *pte;

	for (i = 0; i < KL1PT_PAGES*NPTEPG; i++) {
		if (*(int*)(&l1pt[i]) != PG_INVAL) {
			printf("l1pt[%d] = 0x%x\n", i, *(int*)(&l1pt[i]));
			pte = (struct pte *)PTETOPHYS(l1pt[i]);
			for (j = 0; j < NPTEPG; j++) {
				if (*(int*)(&pte[j]) != PG_INVAL) {
					printf("\tl2pt[%d][%d] = 0x%x\n",
						i, j, *(int*)(&pte[j]));
				}
			}
		}
	}
}

/*
 * Dump IDT and GDT for hand-check.
 */

dumpDT()
{
	register int i;

	printf("GDT @ 0x%x\n", l.eng->e_local->pp_local[0] + ((int)l.gdt - (int)&l));
	for (i = 0; i < GDT_SIZE; i++)
		printf("\tgdt[%d] = 0x%x 0x%x\n", i,
			*(int*)(&l.gdt[i]), *((int*)(&l.gdt[i])+1) );

	printf("IDT @ 0x%x\n", l.eng->e_local->pp_local[0] + ((int)l.idt - (int)&l));
	for (i = 0; i < IDT_SIZE; i++)
		printf("\tidt[%d] = 0x%x 0x%x\n", i,
			*(int*)(&l.idt[i]), *((int*)(&l.idt[i])+1) );

	printf("KERNEL_CS = 0x%x\n", KERNEL_CS);
	printf("KERNEL_DS = 0x%x\n", KERNEL_DS);
	printf("USER_CS = 0x%x\n", USER_CS);
	printf("USER_DS = 0x%x\n", USER_DS);
}

checkspl0(i)
{
	register spl_t s = spl0();
	
	if ((s & 0xff) != SPL0)
		printf("!SPL0 %d, at 0x%x\n", i, s & 0xff);
	asm("pushfl");
	asm("popl %edi");
	if ((s & FLAGS_IF) == 0)
		printf("!STI %d\n", i);
}

checkrunQ(i)
{
	if (CP_GATE(G_RUNQ))
		V_GATE(G_RUNQ, SPL0);
	else
		printf("runQ held! %d\n", i);
}

dumpUPT(p)
	struct proc *p;
{
	register int i;
	register struct pte *pte;

	printf("<%d,%s>: dsize=%d, ssize=%d\n",
		p-proc, p->p_uarea->u_comm, p->p_dsize, p->p_ssize);

	pte = dvtopte(p, 0);
	printf("text+data ptes...\n");
	for (i = 0; i < p->p_dsize; i++, pte++)
		printf("\t[%d] @ 0x%x = 0x%x\n", i, pte, *(int*)pte);

	printf("stack ptes...\n");
	i = p->p_ssize-1;
	pte = sptopte(p, i);
	for (; i >= 0; i--, pte++)
		printf("\t[%d] @ 0x%x = 0x%x\n", i, pte, *(int*)pte);

	printf("Rset: rssize=%d, rshand=%d, rscurr=%d\n",
		p->p_rssize, p->p_rshand, p->p_rscurr);
}

static	char	hex[] = "0123456789abcdef";

dump_mbuf_chain(m)
	register struct mbuf *m;
{
	register int	mcnt;

	for (mcnt = 0; m != NULL; mcnt++, m = m->m_next) {
		printf("mbuf[%d]:", mcnt);
		dump_bytes(mtod(m, char *), m->m_len);
	}
}

dump_bytes(cp, len)
	register char	*cp;
	register int	len;
{
	register int	cnt;

	for (cnt = 0; cnt < len; cnt++, cp++) {
		if ((cnt % 20) == 0)
			printf("\n\t");
		printf(" %c%c", hex[((int)(*cp) >> 4) & 0xf], hex[(*cp) & 0xf]);
	}
	printf("\n");
}
#endif	DEBUG
