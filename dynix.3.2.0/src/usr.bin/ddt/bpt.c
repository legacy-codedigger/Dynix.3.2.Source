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


/*
 * bpt.c: version 1.7 of 8/29/83
 * 
 */
# ifndef lint
static char rcsid[] = "$Header: bpt.c 2.0 86/01/28 $";
# endif

/* breakpoint functions */

/*
 * host defines
 */

#include <setjmp.h>
#include <stdio.h>

/*
 * target defines
 */

#include "main.h"
#include "sym.h"
#include "display.h"
#include "error.h"
#include "bpt.h"
#include "parse.h"
#include "machine.h"

#define	INVALID -1
#define	BPR0	1	
#define	BPR1	2
#define	BPT	3

#define	BPTCODE	0xf2

extern int pid;
extern int errno;

struct bptentry {
	int	bptat;
	int	bpttype;
	int	realcode;
	int	bptcode;
	int	sumode;
};

int firsttime = TRUE;
struct bptentry bptab[MAXBPTS];
struct bptentry tempbpt;

bptabort() 
{
	longjmp(resetbuf, BPT_ERROR);
}

initbpttable()
{
	register int i;

	firsttime = FALSE;
	for (i = 0; i < MAXBPTS; i++)
		bptab[i].bpttype = INVALID;
}

findentry()
{
	register int i;

	if (firsttime)
		initbpttable();
	for (i = 0; i < MAXBPTS; i++) {
		if (bptab[i].bpttype == INVALID)
			return(i);
	}
	printf("\r\nall bpts used\r\n");
	bptabort();
	/*NOTREACHED*/
}

findmatch(addrat)
{
	register int i;

	for (i = 0; i < MAXBPTS; i++)
		if (bptab[i].bpttype == BPT && bptab[i].bptat == addrat)
			return(TRUE);
	return(FALSE);
}

maketemp(addrat)
{
	tempbpt.bptat = addrat;
	tempbpt.bpttype = BPT;
}

makebpt(addrat)
{
	int i;
	register struct bptentry *pbpt;

	i = findentry();
	if (findmatch(addrat)) {
		printf("\r\nbreakpoint exists already\r\n");
		bptabort();
	}
	pbpt = &bptab[i];
	pbpt->bptat = addrat;
	pbpt->bpttype = BPT;
}

rembpt(indexat)
{
	bptab[indexat].bpttype = INVALID;
	remastop(indexat);
}

remallbpts()
{
	register int i;

	for (i=0; i < MAXBPTS; i++)
		bptab[i].bpttype = INVALID;
	remallstops();
}

listbpt()
{
	register int i;

	for (i = 0; i < MAXBPTS; i++) {
		if (bptab[i].bpttype == BPT) {
		    printf("\r\n");
		    printf("%d:\t", i);
		    typeout(bptab[i].bptat, "*/\t");
		    display(bptab[i].bptat);
		}
	}
	printf("\r\n");
}

showbpt(addrat)
{
	register int i;

	for (i = 0; i < MAXBPTS; i++) {
		if (bptab[i].bptat == addrat && bptab[i].bpttype == BPT) {
			printf("\r\n");
			typeout(bptab[i].bptat, "*/\t");
			display(bptab[i].bptat);
			printf("\t<<bpt %d", i);
			fflush(stdout);
			dostop(i);
			return;
		}
	}
	typeout(addrat,"*/\t");
	display(addrat);
	printf("\t<<bpt");
	fflush(stdout);
}

putinbpts(istemp)
{
	int i;
	register struct bptentry *pbpt;

	if (pid == -1) {
		printf("\r\nno process to put bpts in\r\n");
		bptabort();
	}
	if (istemp <= TEMP) {
		pbpt = &tempbpt;
		if ((!findmatch(pbpt->bptat)) || (istemp != PTEMP)) {
			pbpt->realcode = machine(RMEM, pid, pbpt->bptat, 0);
			if (errno) {
				printf("\r\ncan not read for bpt temp\r\n");
				bptabort();
			}
			pbpt->bptcode = (pbpt->realcode & 0xffffff00) | BPTCODE;
			machine(WMEM, pid, pbpt->bptat, pbpt->bptcode);
			if (errno) {
				printf("\r\ncan not insert bpt temp\r\n");
				bptabort();
			}
		}
		if (istemp == TEMP)
			return;
	}
	for (i = 0; i < MAXBPTS; i++) {
		pbpt = &bptab[i];
		if (pbpt->bpttype != BPT)
			continue;
		pbpt->realcode = machine(RMEM, pid, pbpt->bptat, 0);
		if (errno) {
			printf("\r\ncan not read for bpt %d\r\n", i);
			bptabort();
		}
		pbpt->bptcode = (pbpt->realcode & 0xffffff00) | BPTCODE;
		machine(WMEM, pid, pbpt->bptat, pbpt->bptcode);
		if (errno) {
			printf("\r\ncan not insert bpt %d\r\n", i);
#ifdef DEBUG
			printf("\r\npid %d, loc 0x%x, value %x\r\n",
				pid, pbpt->bptat, pbpt->bptcode);
#endif DEBUG
			bptabort();
		}
	}
}

outbpts(istemp)
	int istemp;
{
	int i;
	register struct bptentry *pbpt;

	if (pid == -1) {
	    return;	
	}
	if (istemp <= TEMP) {
		pbpt = &tempbpt;
		if ((!findmatch(pbpt->bptat)) || (istemp != PTEMP)) {
			machine(WMEM, pid, pbpt->bptat, pbpt->realcode);
			if (errno) {
				printf("\r\ncan not remove bpt temp\r\n");
				bptabort();
			}
		}
		if (istemp == TEMP)
		    return;
	}
	for (i = 0; i < MAXBPTS; i++) {
		pbpt = &bptab[i];
		if (pbpt->bpttype != BPT)
			continue;
		machine(WMEM, pid, pbpt->bptat, pbpt->realcode);
		if (errno) {
			printf("\r\ncan not remove bpt %d\r\n", i);
			bptabort();
		}
	}
}
