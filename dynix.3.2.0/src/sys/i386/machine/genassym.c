/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
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
static	char	rcsid[] = "$Header: genassym.c 2.29 1992/02/13 00:15:48 $";
#endif

/*
 * genassym.c
 *	Generate symbols for use by assembly language part of kernel.
 *
 * Avoids need for two copies of any header files.
 * Some C-flavor #defines are acceptable for ASM use; such might be used
 * as needed.
 */

/* $Log: genassym.c,v $
 *
 */

#define	GENASSYM		/* so headers can drop decl's in X-environ */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/user.h"
#include "../h/cmap.h"
#include "../h/map.h"
#include "../h/proc.h"
#include "../h/mbuf.h"
#include "../h/seg.h"

#include "../balance/engine.h"
#include "../balance/slic.h"

#include "../machine/mmu.h"
#include "../machine/pte.h"
#include "../machine/gate.h"
#include "../machine/trap.h"
#include "../machine/plocal.h"
#include "../machine/intctl.h"
#include "../machine/psl.h"
#include "../machine/hwparam.h"
#include "../machine/reg.h"

main()
{
	register struct proc	*p = (struct proc *)0;
	register struct vmmeter	*vm = (struct vmmeter *)0;
	register struct user	*up = (struct user *)0;
	struct	engine		*en = (struct engine *)0;
	struct	plocal		*pl = (struct plocal *)0;
	struct	priv_map	*pm = (struct priv_map *)0;
	struct	cpuslic		*sl = (struct cpuslic *)0;
	struct	bin_header	*bh = (struct bin_header *)0;
	struct	watchpt		*wp = (struct watchpt *)0;
	struct	uprof		*uprof = (struct uprof *)0;
	label_t			*lt = (label_t *)0;
	struct	mbuf		*m = (struct mbuf *)0;

	/*
	 * Gen all the "VA"'s for checking (human!) and use by ASM.
	 * All the VA's defined in ../machine/plocal.h
	 */

	printf("#define	VA_USER	0x%X\n",	VA_USER);
	printf("#define	VA_UAREA	0x%X\n",	VA_UAREA);
	printf("#define	VA_PLOCAL	0x%X\n",	VA_PLOCAL);
	printf("#define	VA_PT	0x%X\n",	VA_PT);
	printf("#define	VA_FPA	0x%X\n",	VA_FPA);
	printf("#define	VA_FPA_PTE	0x%X\n",	VA_FPA_PTE);

	printf("#define	VA_SLIC	0x%X\n",	VA_SLIC);

	/*
	 * PLOCAL_PTE_OFF is offset in Level-1 page-table of pte that maps
	 * processor local data (used in resume() to preserve plocal mapping
	 * across context switch).
	 */

	printf("#define	PLOCAL_PTE_OFF	0x%X\n",
					L1IDX(VA_PLOCAL)*sizeof(struct pte));

	/*
	 * USER_ADDR_MASK is used to insure user supplied syscall arg
	 * pointer is reasonable.  locore.s assumes this is a cloud
	 * of 1's (ie, 2**n-1).
	 */

	printf("#define	USER_SPACE	0x%X\n",	USER_SPACE);
	if (USER_SPACE & (USER_SPACE-1))
		printf("ERROR USER_ADDR_MASK -- check genassym.c, locore.s\n");
	printf("#define	USER_ADDR_MASK	0x%X\n",	USER_SPACE-1);

	/*
	 * Lock and gate states.
	 */

	printf("#define	G_LOCKED	0x%X\n",	G_LOCKED);
	printf("#define	G_UNLOCKED	0x%X\n",	G_UNLOCKED);
	printf("#define	L_LOCKED	0x%X\n",	L_LOCKED);
	printf("#define	L_UNLOCKED	0x%X\n",	L_UNLOCKED);

	/*
	 * Per-processor local data ("l." structure).
	 */

	printf("#define	L_ME	0x%X\n",	&pl->me);
	printf("#define	L_ENG	0x%X\n",	&pl->eng);
	printf("#define	L_PRIV_PT	0x%X\n",	&pl->priv_pt);
	printf("#define	L_PRIVSTK	0x%X\n",	&pl->privstk);
	printf("#define	L_PLOCAL_PTE	0x%X\n",	&pl->plocal_pte);
	printf("#define	L_RUNRUN	0x%X\n",	&pl->runrun);
	printf("#define	L_NOPROC	0x%X\n",	&pl->noproc);
	printf("#define	L_WATCHPTON	0x%X\n",	&pl->watchptON);
	printf("#define	L_USINGFPU	0x%X\n",	&pl->usingfpu);
	printf("#define	L_FPUON	0x%X\n",	&pl->fpuon);
	printf("#define	L_FPUOFF	0x%X\n",	&pl->fpuoff);
	printf("#define	L_CNT	0x%X\n",	&pl->cnt);
	printf("#define	L_TRAP_ERR_CODE	0x%X\n",	&pl->trap_err_code);
	printf("#define L_FLAGS         0x%X\n",        &pl->flags);
	printf("#define L_PTYPE         0x%X\n",        &pl->ptype);
	printf("#define L_DISPATCH      0x%X\n",        &pl->dispatch);
#ifdef COBUG
	printf("#define PL_C0BUG        0x%X\n",        PL_C0BUG);
#endif /* COBUG */
#ifdef	DEBUG
	printf("#define	L_HOLDGATE	0x%X\n",	&pl->holdgate);
	printf("#define	L_LASTPROC	0x%X\n",	&pl->lastproc);
#endif	DEBUG
	printf("#define	L_SLIC_DELAY	0x%X\n",	&pl->slic_delay);

	/*
	 * vmmeter l.cnt fields
	 */

	printf("#define	V_INTR	0x%X\n",	&vm->v_intr);
	printf("#define	V_TRAP	0x%X\n",	&vm->v_trap);
	printf("#define	V_FAULTS	0x%X\n",	&vm->v_faults);

	/*
	 * proc[] table fields.
	 */

	printf("#define	P_FLAG	0x%X\n",	&p->p_flag);
	printf("#define	P_ENGNO	0x%X\n",	&p->p_engno);
	printf("#define	P_PRI	0x%X\n",	&p->p_pri);
	printf("#define	P_PTB1	0x%X\n",	&p->p_ptb1);
	printf("#define	P_STAT	0x%X\n",	&p->p_stat);
	printf("#define	P_STATE	0x%X\n",	&p->p_state);
	printf("#define	P_USRPRI	0x%X\n",	&p->p_usrpri);

	printf("#define	SFSWAP	0x%X\n",	SFSWAP);

	/*
	 * U-area fields.
	 */

	printf("#define	UPAGES	0x%X\n",	UPAGES);
	printf("#define	UBYTES	0x%X\n",	UPAGES*NBPG);

	printf("#define	U_FPUSAVE	0x%X\n",	&up->u_fpusave);
	printf("#define	U_PMAPCNT	0x%X\n",	&up->u_pmapcnt);
	printf("#define	U_PROCP	0x%X\n",	&up->u_procp);
	printf("#define	U_SWTRAP	0x%X\n",	&up->u_swtrap);
	printf("#define	U_FLTADDR	0x%X\n",	&up->u_fltaddr);
	printf("#define	U_ARG	0x%X\n",	&up->u_arg[0]);
	printf("#define	U_SP	0x%X\n",	&up->u_sp);
	printf("#define	U_WATCHPT	0x%X\n",	&up->u_watchpt);
	printf("#define	U_PROF	0x%X\n",	&up->u_prof);
	printf("#define U_FLAGS 0x%X\n",        &up->u_flags);
#ifdef	FPU_SIGNAL_BUG
	printf("#define	UF_USED_FPU 0x%X\n",	UF_USED_FPU);
#endif
#ifdef COBUG
	printf("#define UF_FPSTEP       0x%X\n",        UF_FPSTEP);
	printf("#define UF_OTBIT        0x%X\n",        UF_OTBIT);
#endif /* COBUG */

	printf("#define FPAE_EXTRA_STATUS 0x%X\n", &up->u_fpae_extra_status);

	printf("#define	PR_BASE	0x%X\n",	&uprof->pr_base);
	printf("#define	PR_SIZE	0x%X\n",	&uprof->pr_size);
	printf("#define	PR_OFF	0x%X\n",	&uprof->pr_off);
	printf("#define	PR_SCALE	0x%X\n",	&uprof->pr_scale);

	/*
	 * Engine structure fields.
	 */

	printf("#define	ENGSIZE	0x%X\n",	sizeof(struct engine));
	printf("#define	E_SLICADDR	0x%X\n",	&en->e_slicaddr);
	printf("#define	E_LOCAL	0x%X\n",	&en->e_local);
	printf("#define	E_PRI	0x%X\n",	&en->e_pri);
	printf("#define	E_NPRI	0x%X\n",	&en->e_npri);

	/*
	 * Trap type mnemonics.
	 */

	printf("#define	T_DIVERR	%d\n", T_DIVERR);
	printf("#define	T_DBG	%d\n", T_DBG);
	printf("#define	T_NMI	%d\n", T_NMI);
	printf("#define	T_INT3	%d\n", T_INT3);
	printf("#define	T_INTO	%d\n", T_INTO);
	printf("#define	T_CHECK	%d\n", T_CHECK);
	printf("#define	T_UND	%d\n", T_UND);
	printf("#define	T_DNA	%d\n", T_DNA);
	printf("#define	T_SYSERR	%d\n", T_SYSERR);
	printf("#define	T_RES	%d\n", T_RES);
	printf("#define	T_BADTSS	%d\n", T_BADTSS);
	printf("#define	T_NOTPRES	%d\n", T_NOTPRES);
	printf("#define	T_STKFLT	%d\n", T_STKFLT);
	printf("#define	T_GPFLT	%d\n", T_GPFLT);
	printf("#define	T_PGFLT	%d\n", T_PGFLT);
	printf("#define	T_COPERR	%d\n", T_COPERR);
	printf("#define	T_SWTCH	%d\n", T_SWTCH);

	printf("#define	T_SVC2	%d\n", T_SVC2);

	/*
	 * Register offsets in stack locore frames.
	 */

	printf("#define	INTR_SP_CS	0x%X\n",	INTR_SP_CS*sizeof(int));
	printf("#define	SP_CS	0x%X\n",	SP_CS*sizeof(int));
	printf("#define	SP_EIP	0x%X\n",	SP_EIP*sizeof(int));
	printf("#define	SP_EAX	0x%X\n",	SP_EAX*sizeof(int));
	printf("#define	SP_FLAGS	0x%X\n",	SP_FLAGS*sizeof(int));

	/*
	 * label_t offsets.
	 * fsetjmp() (see machine/inline.h) is *very* dependent on
	 * field order being esp,ebp,eip.
	 */

	if ((int)&lt->lt_esp != 0 || (int)&lt->lt_ebp != 4 || (int)&lt->lt_eip != 8)
		printf("ERROR -- fsetjmp() glitch; see genassym.c\n");

	printf("#define	LT_ESP	0x%X\n", &lt->lt_esp);
	printf("#define	LT_EBP	0x%X\n", &lt->lt_ebp);
	printf("#define	LT_EIP	0x%X\n", &lt->lt_eip);
	printf("#define	LT_EBX	0x%X\n", &lt->lt_ebx);
	printf("#define	LT_ESI	0x%X\n", &lt->lt_esi);
	printf("#define	LT_EDI	0x%X\n", &lt->lt_edi);

	/*
	 * Flags register bits.
	 */

	printf("#define	FLAGS_CF	0x%X\n",	FLAGS_CF);
	printf("#define	FLAGS_IF	0x%X\n",	FLAGS_IF);
	printf("#define FLAGS_TF        0x%X\n",        FLAGS_TF);
	printf("#define	KERNEL_IOPL	0x%X\n",	KERNEL_IOPL);

	/*
	 * SLIC offsets.
	 */

	printf("#define	SL_CMD_STAT	0x%X\n",	&sl->sl_cmd_stat);
	printf("#define	SL_DEST	0x%X\n",	&sl->sl_dest);
	printf("#define	SL_SMESSAGE	0x%X\n",	&sl->sl_smessage);
	printf("#define	SL_B0INT	0x%X\n",	&sl->sl_b0int);
	printf("#define	SL_BININT	0x%X\n",	&sl->sl_binint);
	printf("#define	SL_NMIINT	0x%X\n",	&sl->sl_nmiint);
	printf("#define	SL_LMASK	0x%X\n",	&sl->sl_lmask);
	printf("#define	SL_GMASK	0x%X\n",	&sl->sl_gmask);
	printf("#define	SL_IPL	0x%X\n",	&sl->sl_ipl);
	printf("#define	SL_ICTL	0x%X\n",	&sl->sl_ictl);
	printf("#define	SL_TCONT	0x%X\n",	&sl->sl_tcont);
	printf("#define	SL_TRV	0x%X\n",	&sl->sl_trv);
	printf("#define	SL_TCTL	0x%X\n",	&sl->sl_tctl);
	printf("#define	SL_SDR	0x%X\n",	&sl->sl_sdr);
	printf("#define	SL_PROCGRP	0x%X\n",	&sl->sl_procgrp);
	printf("#define	SL_PROCID	0x%X\n",	&sl->sl_procid);
	printf("#define	SL_CRL	0x%X\n",	&sl->sl_crl);

#if	defined(KXX) || defined(SLIC_GATES)
	/*
	 * Slic commands.
	 */

	printf("#define	SL_REQG	0x%X\n",	SL_REQG);
	printf("#define	SL_RELG	0x%X\n",	SL_RELG);

	/*
	 * Slic command/status register bits. 
	 */

	printf("#define	SL_BUSY	0x%x\n",	SL_BUSY);
	printf("#define	SL_OK	0x%x\n",	SL_OK);
#endif	KXX

	/*
	 * Values from intctl.h (SPL mnemonics, etc).
	 */

	printf("#define	SPL0	0x%X\n",	SPL0);
	printf("#define	SPL1	0x%X\n",	SPL1);
	printf("#define	SPL2	0x%X\n",	SPL2);
	printf("#define	SPL3	0x%X\n",	SPL3);
	printf("#define	SPL_HOLE	0x%X\n",	SPL_HOLE);
	printf("#define	SPL4	0x%X\n",	SPL4);
	printf("#define	SPL5	0x%X\n",	SPL5);
	printf("#define	SPL6	0x%X\n",	SPL6);
	printf("#define	SPL7	0x%X\n",	SPL7);

	printf("#define	SPLNET	0x%X\n",	SPLNET);
	printf("#define	SPLBUF	0x%X\n",	SPLBUF);
	printf("#define	SPLFS	0x%X\n",	SPLFS);
	printf("#define	SPLIMP	0x%X\n",	SPLIMP);
	printf("#define	SPLHI	0x%X\n",	SPLHI);

	printf("#define	NETINTR	0x%X\n",	NETINTR);
	printf("#define	SOFTCLOCK	0x%X\n",	SOFTCLOCK);
	printf("#define	RESCHED	0x%X\n",	RESCHED);

	/*
	 * Offsets for interrupt vector table.
	 */

	printf("#define	BH_SIZE	0x%X\n",	&bh->bh_size);
	printf("#define	BH_HDLRTAB	0x%X\n",	&bh->bh_hdlrtab);

	/*
	 * FPA offsets and values.
	 */

	printf("#define	FPA_STCTX	0x%X\n",	FPA_STCTX);
	printf("#define	FPA_LDCTX	0x%X\n",	FPA_LDCTX);
	printf("#define	FPA_NREGS	0x%X\n",	FPA_NREGS);
	printf("#define	FPA_STOR_R1	0x%X\n",	FPA_STOR_R1);
	printf("#define	FPA_PCR_EM_ALL	0x%X\n",	FPA_PCR_EM_ALL);
	printf("#define	U_FPA_PCR	0x%X\n",	&up->u_fpasave.fpa_pcr);
	printf("#define	U_FPA_REGS	0x%X\n",	up->u_fpasave.fpa_regs);

	/*
	 * Watchpoint/debug stuff.
	 */

	printf("#define	WP_VADDR	0x%X\n",	&wp->wp_vaddr[0]);
	printf("#define	WP_CONTROL	0x%X\n",	&wp->wp_control);
	printf("#define	DSR_CLEAR		0x%x\n",	DSR_CLEAR);
	printf("#define	DCR_OFF		0x%x\n",	DCR_OFF);
#if	DCR_OFF != 0
	ERROR -- "some asm assumes DCR_OFF == 0 -- eg, cswitch.s"
#endif

	/*
	 * Segmentation values.
	 */

	printf("#define	KERNEL_DS	0x%X\n",	KERNEL_DS);
	printf("#define	KERNEL_CS	0x%X\n",	KERNEL_CS);
	printf("#define	KERNEL_TSS	0x%X\n",	GDTXTOKSEL(GDT_TSS));
	printf("#define	USER_DS	0x%X\n",	USER_DS);
	printf("#define	USER_CS	0x%X\n",	USER_CS);

	printf("#define	RPL_MASK	0x%X\n",	RPL_MASK);

	/*
	 * Misc useful values.
	 */

	printf("#define	NBPG	0x%X\n",	NBPG);
	printf("#define	PGSHIFT	0x%X\n",	PGSHIFT);
	printf("#define	CLSIZE	0x%X\n",	CLSIZE);
	printf("#define	CLBYTES	0x%X\n",	CLBYTES);
	printf("#define	CLOFSET	0x%X\n",	CLOFSET);
	printf("#define	NOT_CLOFSET	0x%X\n",	~CLOFSET);
	printf("#define	CPGATESUCCEED	0x%X\n",	CPGATESUCCEED);
	printf("#define	CPGATEFAIL	0x%X\n",	CPGATEFAIL);
	printf("#define	CPLOCKFAIL	0x%X\n",	CPLOCKFAIL);
#if	defined(KXX) || defined(SLIC_GATES)
	printf("#define	GATE_GROUP	0x%X\n",	GATE_GROUP);
#endif	KXX
	printf("#define	LOWPAGES	0x%X\n",	LOWPAGES);
	printf("#define	PIDLE	0x%X\n",	PIDLE);
	printf("#define	SONPROC	0x%X\n",	SONPROC);
	printf("#define	USRSTACK	0x%X\n",	USRSTACK);
	printf("#define	B_READ	0x%X\n",	B_READ);
	printf("#define	B_WRITE	0x%X\n",	B_WRITE);
	printf("#define	RO	0x%X\n",	RO);
	printf("#define	RW	0x%X\n",	RW);
	printf("#define	ENOENT	0x%X\n",	ENOENT);
	printf("#define	EFAULT	0x%X\n",	EFAULT);

	printf("#define	M_NEXT	0x%X\n",	&m->m_next);
	printf("#define	M_OFF	0x%X\n",	&m->m_off);
	printf("#define	M_LEN	0x%X\n",	&m->m_len);
}
