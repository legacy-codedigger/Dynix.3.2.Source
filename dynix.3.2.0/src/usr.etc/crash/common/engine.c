/* $Header: engine.c 2.10 1991/07/01 16:16:09 $ */

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

/*
 * $Log: engine.c,v $
 *
 *
 *
 */

#ifndef lint
static char rcsid[] = "$Header: engine.c 2.10 1991/07/01 16:16:09 $";
#endif

#include "crash.h"

#include <sys/param.h>
#include <sys/mutex.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/dir.h>
#include <signal.h>
#ifdef BSD
#include <sys/kernel.h>
#else
#include <sys/resource.h>
#endif
#include <sys/user.h>
#include <sys/proc.h>
#ifdef BSD
#include <machine/engine.h>
#else
#include <sys/timer.h>
#include <sys/engine.h>
#endif


struct engine *l_engine;	/* local copy of engine table */
struct engine *v_engine;	/* kernel virtual address of engine table */
unsigned Nengine;		/* number of engines */
unsigned nonline;		/* number of online engines */

bool_t dblpanic;		/* double panic? */
struct panic_data panic_data;	/* panic data stuffed here */
int     Last_eng;               /* used by _vtop() */

eng_init()
{
	if (search("panic_data") == NULL)
		printf("Warning:  no kernel data structure 'panic_data', probably an old kernel.\n");
	readv(search("Nengine"), &Nengine, sizeof Nengine);
	readv(search("nonline"), &nonline, sizeof nonline);
	readv(search("panic_data"), &panic_data, sizeof panic_data);
	readv(search("dblpanic"), &dblpanic, sizeof dblpanic);
	if (dblpanic)
		printf("Warning: double panic occured\n");
	if (Nengine > MAXNUMCPU)
		printf("%u for Nengine?, memory probably hosed.\n", Nengine);
	else  if (nonline > MAXNUMCPU || nonline > Nengine)
		printf("%u online, %d Nengine?, memory probably hosed.\n",
		    nonline, Nengine);
	else  if (Nengine != nonline)
		printf("Engine table: %d of %d engines online\n", nonline, Nengine);
	readv(search("engine"), &v_engine, sizeof v_engine);
	l_engine = (struct engine *)malloc(Nengine * sizeof (struct engine));
	readv(v_engine, l_engine, Nengine * sizeof (struct engine));
}

Eng()
{
	register int i;
	register char *arg;

	printf("ENG   ADDR SLIC ");
#ifdef BSD
	printf("   HEAD     TAIL  DIAG     FLAG       STATE PRI NPRI CNT ELOCAL\n");
#else
	printf("%-8s %-6s %4s %4s %-10s\n",
		"DIAG", "FLAG", "PRI", "NPRI", "ELOCAL");
#endif
	if ((arg = token()) == NULL) {
		for(i = 0; i < Nengine; i++)
			preng(i);
	} else {
		 do {
			preng(atoi(arg));
		} while ((arg = token()));
	}
}

Every()
{
	register char *arg;
	extern	int	Eve;
	extern	int	To;

	if (!live) {
		printf("option only valid on live systems\n");
		return;
	}
	if ((arg = token()) == NULL) {
		Eve = -1;
	} else
		Eve = atoi(arg);
	To = 1;
	printf("%d seconds\n",Eve);
}

preng(i)
{
	register struct engine *e, *v;
	register char *p;
	char lbuf[128];

	if ( i > Nengine) {
		printf("eng %d?, silly boy..\n", i);
		return;
	}
	Last_eng = i;
	e = &l_engine[i];
	v = &v_engine[i];
	if (live) 
		readv(v, e, sizeof (struct engine));

	printf("%2d %08x %2d ", i, v, e->e_slicaddr);
#ifdef BSD	/* (actualy not cach affinity) */
	if (e->e_head == e->e_tail && e->e_head == (struct proc *)v)
		printf("       -        - ");
	else
		printf("%8.8x %8.8x ", e->e_head, e->e_tail);

	switch (e->e_diag_flag) {
	case 0:		
		p = "      ";
		break;
	default:	
		sprintf(lbuf, "%#06x", e->e_diag_flag);
		p = lbuf; 
		break;
	}
	printf("%s ", p);

	pr_eflags(e->e_flags);
	
	switch (e->e_state) {
	default:	
		sprintf(lbuf, "%#06x", e->e_state);
		p = lbuf;
		break;
	case E_GLOBAL:	
		p = "GLOBAL";
		break;
	case E_BOUND:	
		p = "BOUND "; 
		break;
	}
	printf("%s %3d %3d  %2d %#06x", p, 
		(u_char) e->e_pri, (u_char) e->e_npri, e->e_count, e->e_local);
#else
	/* DIAG FLAG PRI NPRI ELOCAL */
	printf("0x%04x 0x%06x %4d %4d 0x%08x",
		e->e_diag_flag,
		e->e_flags, e->e_pri, e->e_npri, e->e_local);
#endif
	if (v == panic_data.pd_engine) {
		if (panic_data.pd_proc)
			printf(" [proc=%d]", panic_data.pd_proc - proc);
		else
			printf(" [idle]");
	}
	printf("\n");
}


static 
pr_eflags(flags)
	int	 flags;
{
	char	*p;

	/*
	 * 19 char field
	 */
	if (flags&0x1f) {
		switch (flags&0x1f) {
		case E_OFFLINE: 
			p = "OFFLINE"; 
			break;
		case E_BAD:	
			p = "    BAD"; 
			break;
		case E_SHUTDOWN:
			p = "SHUTDWN"; 
			break;
		case E_DRIVER:	
			p = " DRIVER"; 
			break;
		case E_PAUSED:	
			p = " PAUSED"; 
			break;
		}

		printf("%s ", p);
		if (flags&E_FPA) 
			printf("F ");
		else
			printf("  ");
		
		if (flags&E_FPU387) 
			printf("U ");
		else
			printf("  ");

#ifdef E_SGS2
		if (flags&E_SGS2) 
			printf("2 ");
		else
#endif
			printf("  ");
	} else {
		if (flags&E_FPA) 
			printf("FPA ");
		else
			printf("    ");
		
		if (flags&E_FPU387) 
			printf("FPU ");
		else
			printf("    ");

#ifdef E_SGS2
		if (flags&E_SGS2) 
			printf("SGS2 ");
		else
#endif
			printf("     ");
	}
}
