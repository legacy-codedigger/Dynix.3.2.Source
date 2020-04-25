/* 
 * $Copyright: $
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

#ifndef lint
static char rcsid[] = "$Header: pte.c 1.7 1991/07/01 16:11:02 $";
#endif

/*
 * $Log: pte.c,v $
 *
 *
 *
 */

#include "crash.h"

#include <sys/param.h>
#include <sys/vmparam.h>
#include <sys/systm.h>
#include <sys/dir.h>
#ifdef BSD
#include <sys/user.h>
#include <sys/proc.h>
#include <machine/pte.h>
#else
#include <sys/resource.h>
#include <signal.h>
#include <sys/user.h>
#include <sys/timer.h>
#include <sys/proc.h>
#include <sys/pte.h>
#endif

#ifndef USOFF
#define USOFF 0
#endif

Upte()
{

	struct proc *p;
	struct user *u;
	char *arg;
	unsigned long vaddr;
	unsigned long idx1;
	unsigned long idx2;
	unsigned long phys;
	struct pte pte;
	int i;
	char *c;

	if ((arg = token()) == NULL) {
		printf("need a number\n");
		return;
	}

	if ((p = readproc(getproc(arg))) == NULL)
		return;
	

	printf("\tuser segment = 0x%8.8x (user) 0x%8.8x (kernel)\n", 
		USOFF, VA_USER);

	printf("\tul1pt(phys)  ul2pt(phys) ul2pt(virt) ul2pt top(virt)\n");
	printf("\t 0x%8.8x   0x%8.8x  0x%8.8x      0x%8.8x\n",
		 p->p_ptb1, vtop(p->p_ul2pt), p->p_ul2pt, p->p_pttop);

	if ((arg = token()) == NULL) 
		return;

	vaddr = atoi(arg);

	idx1 = p->p_ptb1 + ((L1IDX(vaddr) + L1IDX(USOFF)) % 0x3ff) *
			sizeof (struct pte);
	if (readp(idx1, &pte, sizeof pte) != sizeof pte) {
		printf("read error on pte\n");
		return;
	}
	i = *(int *)&pte;
	printf("\n");
	printf("\tpage\t addr of\t:valid:prot:pg_pfnum'\n");
	printf("\ttable\tpte entry\n");
	printf("\tUL1PT:\t0x%8.8x\t", idx1);
	printf("%s:", pte.pg_v & PG_V ? "v" : "i");
	switch (i & PG_PROT) {
#if PG_KR != PG_KW
	case PG_KR:
		c = "kr";
		break;
#endif
	case PG_KW:
		c = "kw";
		break;
	case PG_URKW:
		c = "ur";
		break;
	case PG_UW:
		c = "uw";
		break;
	}
	printf("%2s:0x%8.8x \n", c, PTETOPHYS(pte));

	if (!pte.pg_v)
		return;
	idx2 = PTETOPHYS(pte) + (L2IDX(vaddr) * sizeof(struct pte));
	if (readp(idx2, &pte, sizeof pte) != sizeof pte) {
		printf("read error on pte\n");
		return;
	}
	i = *(int *)&pte;
	printf("\tUL2PT:\t0x%8.8x\t", idx2);
	if (i == PG_ZFOD) {
		printf("Zero Fill On Demand\n");
		return;
	}
	printf("%s:", pte.pg_v & PG_V ? "v" : "i");
	switch (i & PG_PROT) {
#if PG_KR != PG_KW
	case PG_KR:
		c = "kr";
		break;
#endif
	case PG_KW:
		c = "kw";
		break;
	case PG_URKW:
		c = "ur";
		break;
	case PG_UW:
		c = "uw";
		break;
	}
	if (PTEMAPPED(pte)) {
		printf("Map indexed %d ", PTETOMAPX(pte));
	}
	printf("%2s:0x%8.8x \n", c, PTETOPHYS(pte));
	if (!pte.pg_v)
		return;
	phys = (PTETOPHYS(pte)) + (vaddr & NBPG-1);
	printf("\t\tphysaddr = 0x%8.8x\n", phys);
}
