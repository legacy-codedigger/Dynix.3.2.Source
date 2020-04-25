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
static	char	rcsid[] = "$Header: in_cksum.c 2.5 90/06/09 $";
#endif

/*
 *	Internet checksum routine - contains ASM
 */

/* $Log:	in_cksum.c,v $
 */

#include "../h/param.h"
#include "../h/mbuf.h"
#include "../netinet/in.h"
#include "../netinet/in_systm.h"

/*
 * Checksum routine for Internet Protocol family headers (NS32000 Version).
 *
 * This routine is very heavily used in the network
 * code and should be modified for each CPU to be as fast as possible.
 */

in_cksum(m, len)
#ifdef	vax
	register struct mbuf *m;
	register int len;
#endif	vax
#ifdef	ns32000
	register struct mbuf *m;
	register int	len;
#endif	ns32000
#ifdef	i386
	struct mbuf *m;
	int	len;
#endif	i386
{
#ifdef	vax
	register u_short *w;		/* on vax, known to be r9 */
	register int sum = 0;		/* on vax, known to be r8 */
	register int mlen = 0;
#endif	vax
#ifdef	ns32000
	register u_short *w;		/* on NS32000, known to be r5 */
	register int sum = 0;		/* on NS32000, known to be r4 */
	register int mlen = 0;		/* on ns32000, known to be r3 */
#endif	ns32000
#ifdef	i386
	register int mlen = 0;		/* on i386, known to be %edi */
	register u_short *w;		/* on i386, known to be %esi */
	register int sum = 0;		/* on i386, known to be %ebx */
#endif	i386

	for (;;) {
		/*
		 * Each trip around loop adds in words from one mbuf segment.
		 */
		w = mtod(m, u_short *);
		if (mlen == -1) {
			/*
			 * There is a byte left from the last segment;
			 * add it into the checksum.  Don't have to worry
			 * about a carry-out here because we make sure
			 * that high part of (32 bit) sum is small below.
			 */
			sum += *(u_char *)w << 8;
			w = (u_short *)((char *)w + 1);
			mlen = m->m_len - 1;
			len--;
		} else
			mlen = m->m_len;
		m = m->m_next;
		if (len < mlen)
			mlen = len;
		len -= mlen;
		/*
		 * Force to long boundary so we do longword aligned
		 * memory operations.  It is too hard to do byte
		 * adjustment, do only word adjustment.
		 */
		if (((int)w&0x2) && mlen >= 2) {
			sum += *w++;
			mlen -= 2;
		}
		/*
		 * Do as much of the checksum as possible 32 bits at at time.
		 * In fact, this loop is unrolled to make overhead from
		 * branches &c small.
		 *
		 * We can do a 16 bit ones complement sum 32 bits at a time
		 * because the 32 bit register is acting as two 16 bit
		 * registers for adding, with carries from the low added
		 * into the high (by normal carry-chaining) and carries
		 * from the high carried into the low on the next word
		 * by use of the adwc instruction.  This lets us run
		 * this loop at almost memory speed.
		 *
		 * Here there is the danger of high order carry out, and
		 * we carefully use adwc (on VAX).
		 */
		while ((mlen -= 32) >= 0) {
#ifdef	vax
#undef ADD
			asm("clrl r0");	/* clears carry */
#define ADD		asm("adwc (r9)+,r8;");
			ADD; ADD; ADD; ADD; ADD; ADD; ADD; ADD;
			asm("adwc $0,r8");
#endif	vax

#ifdef	ns32000
	/*** N32000 does not have autoincrement and incrementing pointer (w)
		affects the carry so roll out 8 explicit adds ***/
	/*** r4 starts at zero, or last addcd 0, r4 cleared carry, so
		there is no carry for first addd ***/
			asm("addd 0(r5), r4");	
			asm("addcd 4(r5), r4");
			asm("addcd 8(r5), r4");
			asm("addcd 12(r5), r4");
			asm("addcd 16(r5), r4");
			asm("addcd 20(r5), r4");
			asm("addcd 24(r5), r4");
			asm("addcd 28(r5), r4");
			asm("addcd 0, r4");	/** last carry bit **/
			asm("addr 32(r5), r5"); /** increment pointer **/
				/*** n.b. does not affect carry ***/
#endif	ns32000

#ifdef	i386
		/*
		 * Use string-instructions to auto-inc "w".
		 */
#define	ADD	asm("slodl"); asm("adcl %eax, %ebx");
		asm("slodl"); asm("addl %eax, %ebx");	/* 1st add */
		ADD; ADD; ADD; ADD; ADD; ADD; ADD;
		asm("adcl $0, %ebx");			/* last carry bit */
#endif	i386
		}
		mlen += 32;
		while ((mlen -= 8) >= 0) {
#ifdef	vax
			asm("clrl r0");
			ADD; ADD;
			asm("adwc $0,r8");
#endif	vax

#ifdef	ns32000
	/*** either the carry is already cleared from above loop, or
		there weren't 32 bytes to checksum and r4 is 0,
		or addr at end of this loop leaves cleared carry, so
		addd then addcd to get 8 byte chuncks ***/
			asm("addd 0(r5), r4");
			asm("addcd 4(r5), r4");
			asm("addcd 0, r4"); /** get carry from last add **/
			asm("addr 8(r5), r5"); /** increment pointer (w) **/
#endif	ns32000

#ifdef	i386
		asm("slodl"); asm("addl %eax, %ebx");	/* 1st add */
		ADD;
		asm("adcl $0, %ebx");			/* last carry bit */
#endif	i386
		}
		mlen += 8;
		/*
		 * Now eliminate the possibility of carry-out's by
		 * folding back to a 16 bit number (adding high and
		 * low parts together.)  Then mop up trailing words
		 * and maybe an odd byte.
		 */
#ifdef	vax
		{ asm("ashl $-16,r8,r0; addw2 r0,r8");
		  asm("adwc $0,r8; movzwl r8,r8"); }
		while ((mlen -= 2) >= 0) {
			asm("movzwl (r9)+,r0; addl2 r0,r8");
		}
#endif	vax

#ifdef	ns32000
		{ asm("movd r4, r0");
		  asm("ashd -16, r0");
		  asm("addw r0, r4");
		  asm("addcd 0, r4");
		  asm("movzwd r4, r4");
		}
		while ((mlen -= 2) >= 0) {
			asm("movzwd 0(r5), r0");
			asm("addd r0, r4");	/** carry goes into upper **/
			asm("addqd 2, r5");	/** increment pointer (w) **/
		}
#endif	ns32000

#ifdef	i386
		{ asm("movl %ebx, %eax");
		  asm("shrl $16, %eax");	/* top 16-bits %eax == 0 */
		  asm("addw %ax, %bx");
		  asm("adcl $0, %ebx");
		  asm("andl $0xffff, %ebx");
		}
		/*
		 *** NOTE: LOOP ASSUMES %eax NOT MODIFIED EXCEPT IN asm() ***
		 */
		while ((mlen -= 2) >= 0) {
			asm("slodw"); asm("addl %eax, %ebx");
		}
#endif	i386
		if (mlen == -1) {
			sum += *(u_char *)w;
		}
		if (len == 0)
			break;
		/*
		 * Locate the next block with some data.
		 * If there is a word split across a boundary we
		 * will wrap to the top with mlen == -1 and
		 * then add it in shifted appropriately.
		 */
		for (;;) {
			if (m == 0) {
				printf("cksum: out of data\n");
				/*
				 *+ An arror occured during the Internet
				 *+ Protocal checksum routine.
				 *+ This may be due to bad packets being
				 *+ sent or recieved.
				 */
				goto done;
			}
			if (m->m_len)
				break;
			m = m->m_next;
		}
	}
done:
	/*
	 * Add together high and low parts of sum
	 * and carry to get cksum.
	 * Have to be careful to not drop the last
	 * carry here.
	 */
#ifdef	vax
	{ asm("ashl $-16,r8,r0; addw2 r0,r8; adwc $0,r8");
	  asm("mcoml r8,r8; movzwl r8,r8"); }
#endif	vax

#ifdef	ns32000
	{ asm("movd r4, r0");
	  asm("ashd -16, r0");
	  asm("addw r0, r4");
	  asm("addcd 0, r4");
	  asm("comd r4, r4");
	  asm("movzwd r4, r4");
	}
#endif	ns32000

#ifdef	i386
	{ asm("movl %ebx, %eax");
	  asm("shrl $16, %eax");
	  asm("addw %ax, %bx");
	  asm("adcl $0, %ebx");
	  asm("notl %ebx");
	  asm("andl $0xffff, %ebx");
	}
#endif	i386

	return (sum);
}
