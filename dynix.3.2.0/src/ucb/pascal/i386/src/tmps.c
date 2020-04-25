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

#if !defined(lint)
static char rcsid[] = "$Id: tmps.c,v 1.1 88/09/02 11:48:30 ksb Exp $";
#endif lint

#include "whoami.h"
#include "0.h"
#include "objfmt.h"
#if defined(PC)
#include "pc.h"
#endif PC
#include "align.h"
#include "tmps.h"
#include <strings.h>

/*
 * This routine defines the register allocation strategy
 * All temporaries are allocated here, and this routines decides
 * where they are to be put.
 */
#if defined(PC)
/*
 * register temporaries
 *	- are allocated from highreg towards lowreg.
 *	- are of size regsize.
 *	- register numbers from the various register types are mapped to 
 *	  integer register numbers using the offsets.  (cf. pcc/mac2defs)
 *
 * stack temporaries
 *	- are allocated on a downward growing stack.
 */

#if defined(i386)
/*
 * first pass register declaration constants
 */
struct	regtype {
	long	lowreg;
	long	highreg;
	long	regsize;
};

struct regtype regtypes[NUMREGTYPES] = {
	{ 3, 5, 3 },		/* r3..r7 	*/
	{ 15, 15, 0 },		/* f6 (and f7)  */
};
#endif /* i386 */
#endif PC

tmpinit(cbn)
int	cbn;
{
	register struct om *sizesp = &sizes[cbn];
#if defined(PC)
	register int	i;
#endif PC

	sizesp->om_max = -DPOFF1;
	sizesp->curtmps.om_off = -DPOFF1;
#if defined(PC)
	for (i = 0; i < NUMREGTYPES; ++i) {
		sizesp->low_water[i] = regtypes[i].highreg + 1;
		sizesp->curtmps.next_avail[i] = regtypes[i].highreg;
	}
#endif PC
}

/*
 * allocate runtime temporary variables
 */
/*ARGSUSED*/
struct nl *
tmpalloc(size, type, mode)
	long size;
	struct nl *type;
	int mode;
{
	register struct om *op = &sizes[ cbn ];
	register int offset;
	register struct nl *nlp;
	auto long alignment;

#if defined(PC)
#if defined(i386)
	if (mode == REGOK &&
	    type != nl + TDOUBLE &&
	    size == regtypes[REG_DATA].regsize &&
	    op->curtmps.next_avail[REG_DATA] >= regtypes[REG_DATA].lowreg) {
		offset = op->curtmps.next_avail[REG_DATA]--;
		if (offset < op->low_water[REG_DATA]) {
			op->low_water[REG_DATA] = offset;
		}
		nlp = defnl(0, VAR, type, offset);
		nlp -> extra_flags = NLOCAL | NREGVAR;
		putlbracket(ftnno, op);
		return nlp;
	}
	if (mode == REGOK &&
	    type == nl + TDOUBLE &&
	    size == regtypes[REG_FLPT].regsize &&
	    op->curtmps.next_avail[REG_FLPT] >= regtypes[REG_FLPT].lowreg) {
		offset = op->curtmps.next_avail[REG_FLPT]--;
		if (offset < op->low_water[REG_FLPT]) {
			op->low_water[REG_FLPT] = offset;
		}
		nlp = defnl(0, VAR, type, offset);
		nlp -> extra_flags = NLOCAL | NREGVAR;
		putlbracket(ftnno, op);
		return nlp;
	}
#endif /* i386 */
#endif PC
	if (type == NIL) {
	    alignment = A_STACK;
	} else if (type == nl+TPTR) {
	    alignment = A_POINT;
	} else {
	    alignment = align(type);
	}
        op->curtmps.om_off = roundup((int)(op->curtmps.om_off - size),
	    alignment);
	offset = op->curtmps.om_off;
	if ( offset < op->om_max ) {
	        op->om_max = offset;
	}
	nlp = defnl( (char *)0, VAR, type, offset);
#if defined(PC)
	nlp->extra_flags = NLOCAL;
	putlbracket(ftnno, op);
#endif PC
	return nlp;
}

/*
 * deallocate runtime temporary variables
 */
/*ARGSUSED*/
tmpfree(restore)
	register struct tmps	*restore;
{
#if defined(PC)
	register struct om	*op = &sizes[ cbn ];
	bool			change = FALSE;

#if defined(REG_DATA)
	if (restore->next_avail[REG_DATA] > op->curtmps.next_avail[REG_DATA]) {
		op->curtmps.next_avail[REG_DATA] = restore->next_avail[REG_DATA];
		change = TRUE;
	}
#endif	/* DATA registers	*/
#if defined(REG_FLPT)
	if (restore->next_avail[REG_FLPT] > op->curtmps.next_avail[REG_FLPT]) {
		op->curtmps.next_avail[REG_FLPT] = restore->next_avail[REG_FLPT];
		change = TRUE;
	}
#endif /* floating registers	*/

	if (restore->om_off > op->curtmps.om_off) {
		op->curtmps.om_off = restore->om_off;
		change = TRUE;
	}
	if (change) {
		putlbracket(ftnno, op);
	}
#endif /* PC	*/
}

#if defined(PC)
#if defined(ns32000)
/*
 * create a save mask for registers which have been used
 * in this level
 */
char *
savmask()
{
	register int i;
	register char *ptreglist;
	static char *reglist = "rx,rx,rx,rx,rx,rx,rx,rx";
	char *mask;
	int regcount = 0;

	mask = "";
	for (i = 0; i <= regtypes[REG_DATA].highreg; i++) {
	    if (i >= sizes[cbn].low_water[REG_DATA]) {
		regcount++;
	    }
	}
	if (regcount) {
	    ptreglist = (char *) malloc (regcount * 3);
	    mask = strncpy (ptreglist, reglist, regcount * 3 - 1);
	    ptreglist++;
	    for (i = 0; i <= regtypes[REG_DATA].highreg; i++) {
	        if (i >= sizes[cbn].low_water[REG_DATA]) {
		    *ptreglist = i + '0';
		    if (*(ptreglist + 1) == ',')
		    	ptreglist += 3;
	    	}
	    }
	}
	return mask;
}

#include <ctype.h>

int
maskbyte(pch)
register char *pch;
{
	register int rv = 0;		/* return value			*/
	register unsigned ch;

	while (ch = *++pch) {
		ch ^= 0x30;
		if (ch & 0xf8)
			continue;
		rv |= 1 << ch;
	}
	return rv;
}
#endif /* ns32000 */
#endif PC
