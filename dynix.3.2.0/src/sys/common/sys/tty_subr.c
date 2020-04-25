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
static	char	rcsid[] = "$Header: tty_subr.c 2.2 89/08/03 $";
#endif

/*
 * tty_subr.c
 * 	Clist manipulation and other tty related subroutines.
 */

/* $Log:	tty_subr.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/clist.h"

#include "../machine/gate.h"
#include "../machine/intctl.h"

struct	cblock *cfreelist;
int	cfreecount;
#ifdef PERFSTAT
int	cfreelow;
#endif PERFSTAT

/*
 * All of the following routines assume that
 * the clists are locked outside of the clist
 * structure at a priority sufficient to disallow
 * interrupts from the device on the current processor.
 * For example, with ttys the clists are locked via
 * t_ttylock at SPLTTY. The spl interface is maintained for
 * use of any non-tty mono-processor driver which
 * needs clists (e.g. line printer).
 */

/*
 * Character list get/put
 */
getc(p)
	register struct clist *p;
{
	register struct cblock *bp;
	register int c, s;

	s = spl5();
	if (p->c_cc <= 0) {
		c = -1;
		p->c_cc = 0;
		p->c_cf = p->c_cl = NULL;
	} else {
		c = *p->c_cf++ & 0377;
		if (--p->c_cc<=0) {
			bp = (struct cblock *)(p->c_cf-1);
			bp = (struct cblock *) ((int)bp & ~CROUND);
			p->c_cf = NULL;
			p->c_cl = NULL;
			VOID_P_GATE(G_CFREE);
			bp->c_next = cfreelist;
			cfreelist = bp;
			cfreecount += CBSIZE;
			V_GATE(G_CFREE, SPL5);
		} else if (((int)p->c_cf & CROUND) == 0){
			bp = (struct cblock *)(p->c_cf);
			bp--;
			p->c_cf = bp->c_next->c_info;
			VOID_P_GATE(G_CFREE);
			bp->c_next = cfreelist;
			cfreelist = bp;
			cfreecount += CBSIZE;
			V_GATE(G_CFREE, SPL5);
		}
	}
	splx(s);
	return(c);
}

/*
 * Return count of contiguous characters
 * in clist starting at q->c_cf.
 * Stop counting if flag&character is non-null.
 */
ndqb(q, flag)
	register struct clist *q;
{
	register cc;
	int s;

	s = spl5();
	if (q->c_cc <= 0) {
		cc = -q->c_cc;
		goto out;
	}
	cc = ((int)q->c_cf + CBSIZE) & ~CROUND;
	cc -= (int)q->c_cf;
	if (q->c_cc < cc)
		cc = q->c_cc;
	if (flag) {
		register char *p, *end;

		p = q->c_cf;
		end = p;
		end += cc;
		while (p < end) {
			if (*p & flag) {
				cc = (int)p;
				cc -= (int)q->c_cf;
				break;
			}
			p++;
		}
	}
out:
	splx(s);
	return(cc);
}

/*
 * Flush cc bytes from q.
 */
ndflush(q, cc)
	register struct clist *q;
	register cc;
{
	register struct cblock *bp;
	char *end;
	int rem, s;

	s = spl5();
	if (q->c_cc <= 0) {
		goto out;
	}
	while (cc>0 && q->c_cc) {
		bp = (struct cblock *)((int)q->c_cf & ~CROUND);
		if ((int)bp == (((int)q->c_cl-1) & ~CROUND)) {
			end = q->c_cl;
		} else {
			end = (char *)((int)bp + sizeof (struct cblock));
		}
		rem = end - q->c_cf;
		if (cc >= rem) {
			cc -= rem;
			q->c_cc -= rem;
			q->c_cf = bp->c_next->c_info;
			VOID_P_GATE(G_CFREE);
			bp->c_next = cfreelist;
			cfreelist = bp;
			cfreecount += CBSIZE;
			V_GATE(G_CFREE, SPL5);
		} else {
			q->c_cc -= cc;
			q->c_cf += cc;
			if (q->c_cc <= 0) {
				VOID_P_GATE(G_CFREE);
				bp->c_next = cfreelist;
				cfreelist = bp;
				cfreecount += CBSIZE;
				V_GATE(G_CFREE, SPL5);
			}
			break;
		}
	}
	if (q->c_cc <= 0) {
		q->c_cf = q->c_cl = NULL;
		q->c_cc = 0;
	}
out:
	splx(s);
}


putc(c, p)
	register struct clist *p;
{
	register struct cblock *bp;
	register char *cp;
	register spl_t s;

	s = spl5();
	if ((cp = p->c_cl) == NULL || p->c_cc < 0 ) {
		VOID_P_GATE(G_CFREE);
		if ((bp = cfreelist) == NULL) {
			V_GATE(G_CFREE, s);	/* return at spl s */
#ifdef	i386
			splx(s);		/* V_GATE() doesn't touch spl */
#endif	i386
			return(-1);
		}
		cfreelist = bp->c_next;
		cfreecount -= CBSIZE;
#ifdef PERFSTAT
		if (cfreecount < cfreelow)
			cfreelow = cfreecount;
#endif PERFSTAT
		V_GATE(G_CFREE, SPL5);
		bp->c_next = NULL;
		p->c_cf = cp = bp->c_info;
	} else if (((int)cp & CROUND) == 0) {
		bp = (struct cblock *)cp - 1;
		VOID_P_GATE(G_CFREE);
		if ((bp->c_next = cfreelist) == NULL) {
			V_GATE(G_CFREE, s);
#ifdef	i386
			splx(s);		/* V_GATE() doesn't touch spl */
#endif	i386
			return(-1);
		}
		bp = bp->c_next;
		cfreelist = bp->c_next;
		cfreecount -= CBSIZE;
#ifdef PERFSTAT
		if (cfreecount < cfreelow)
			cfreelow = cfreecount;
#endif PERFSTAT
		V_GATE(G_CFREE, SPL5);
		bp->c_next = NULL;
		cp = bp->c_info;
	}
	*cp++ = c;
	p->c_cc++;
	p->c_cl = cp;
	splx(s);
	return(0);
}



/*
 * copy buffer to clist.
 * return number of bytes not transfered.
 *
 * Rewritten to allocate needed cblocks at
 * one time. Thus gate accesses are reduced.
 */
b_to_q(cp, cc, q)
	register char *cp;
	struct clist *q;
	register int cc;
{
	register char *cq;
	register struct cblock *bp;
	register int ncbwant;
	register int nc;
	int acc;
	spl_t s;

	if (cc <= 0)
		return(0);
	acc = cc;


	s = spl5();
	if ((cq = q->c_cl) == NULL || q->c_cc < 0) {
		ncbwant = cc / CBSIZE;
		if ((cc-(ncbwant*CBSIZE)) > 0)
			ncbwant++;
		VOID_P_GATE(G_CFREE);
		/*
		 * Get the 1st cblock - if possible.
		 */
		if ((bp = cfreelist) == NULL) {
			V_GATE(G_CFREE, SPL5);
			goto out;
		}
		cfreelist = bp->c_next;
		cfreecount -= CBSIZE;
		q->c_cf = cq = bp->c_info;	/* first one allocated */

		/*
		 * Allocate the rest.
		 */
		while((--ncbwant > 0) && ((bp->c_next = cfreelist) != NULL)) {
			bp = cfreelist;
			cfreelist = bp->c_next;
			cfreecount -= CBSIZE;
		}
#ifdef PERFSTAT
		if (cfreecount < cfreelow)
			cfreelow = cfreecount;
#endif PERFSTAT
		V_GATE(G_CFREE, SPL5);
		bp->c_next = NULL;		/* last one allocated */
	}

	while (cc) {
		if (((int)cq & CROUND) == 0) {
			bp = (struct cblock *)cq - 1;
			if (bp->c_next != NULL)
				/*
				 * more in already allocated chain
				 */
				cq = bp->c_next->c_info;
			else {
				ncbwant = cc / CBSIZE;
				if ((cc-(ncbwant*CBSIZE)) > 0)
					ncbwant++;
				VOID_P_GATE(G_CFREE);
				/*
				 * Get another cblock - if possible.
				 */
				if ((bp->c_next = cfreelist) == NULL) {
					V_GATE(G_CFREE, SPL5);
					goto out;
				}
				bp = cfreelist;
				cfreelist = bp->c_next;
				cfreecount -= CBSIZE;
				cq = bp->c_info;

				/*
				 * Allocate the rest.
				 */
				while((--ncbwant > 0) && ((bp->c_next = cfreelist) != NULL)) {
					bp = cfreelist;
					cfreelist = bp->c_next;
					cfreecount -= CBSIZE;
				}
#ifdef PERFSTAT
				if (cfreecount < cfreelow)
					cfreelow = cfreecount;
#endif PERFSTAT
				V_GATE(G_CFREE, SPL5);
				bp->c_next = NULL;
			}
		}
		nc = MIN(cc, sizeof(struct cblock) - ((int)cq & CROUND));
		bcopy(cp, cq, (unsigned)nc);
		cp += nc;
		cq += nc;
		cc -= nc;
	}
out:
	q->c_cl = cq;
	q->c_cc += acc-cc;
	splx(s);
	return(cc);
}

/*
 * Given a non-NULL pointer into the list (like c_cf which
 * always points to a real character if non-NULL) return the pointer
 * to the next character in the list or return NULL if no more chars.
 *
 * Callers must not allow getc's to happen between nextc's so that the
 * pointer becomes invalid.  Note that interrupts are NOT masked.
 */
char *
nextc(p, cp)
	register struct clist *p;
	register char *cp;
{

	if (p->c_cc && ++cp != p->c_cl) {
		if (((int)cp & CROUND) == 0)
			return (((struct cblock *)cp)[-1].c_next->c_info);
		return (cp);
	}
	return (0);
}

/*
 * Remove the last character in the list and return it.
 */
unputc(p)
	register struct clist *p;
{
	register struct cblock *bp;
	register int c, s;
	struct cblock *obp;

	s = spl5();
	if (p->c_cc <= 0)
		c = -1;
	else {
		c = *--p->c_cl;
		if (--p->c_cc <= 0) {
			bp = (struct cblock *)p->c_cl;
			bp = (struct cblock *)((int)bp & ~CROUND);
			p->c_cl = p->c_cf = NULL;
			VOID_P_GATE(G_CFREE);
			bp->c_next = cfreelist;
			cfreelist = bp;
			cfreecount += CBSIZE;
			V_GATE(G_CFREE, SPL5);
		} else if (((int)p->c_cl & CROUND) == sizeof(bp->c_next)) {
			p->c_cl = (char *)((int)p->c_cl & ~CROUND);
			bp = (struct cblock *)p->c_cf;
			bp = (struct cblock *)((int)bp & ~CROUND);
			while (bp->c_next != (struct cblock *)p->c_cl)
				bp = bp->c_next;
			obp = bp;
			p->c_cl = (char *)(bp + 1);
			bp = bp->c_next;
			VOID_P_GATE(G_CFREE);
			bp->c_next = cfreelist;
			cfreelist = bp;
			cfreecount += CBSIZE;
			V_GATE(G_CFREE, SPL5);
			obp->c_next = NULL;
		}
	}
	splx(s);
	return (c);
}

/*
 * Put the chars in the from queue
 * on the end of the to queue.
 */
catq(from, to)
	struct clist *from, *to;
{
	register c;

	while ((c = getc(from)) >= 0)
		(void) putc(c, to);
}
