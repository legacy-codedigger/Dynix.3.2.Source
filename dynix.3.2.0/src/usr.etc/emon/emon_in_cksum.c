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
static	char	rcsid[] = "$Header: emon_in_cksum.c 2.3 87/04/11 $";
#endif

/*
 * $Log:	emon_in_cksum.c,v $
 *
 */

#include "emon.h"

#ifdef	ns32000

/*
 * Checksum routine for Internet Protocol family headers (16K Version).
 *
 * This routine is very heavily used in the network
 * code and should be modified for each CPU to be as fast as possible.
 */

in_cksum(m, len)
	register u_short *m;
	register int	len;
{
	register u_short *w;		/* on 16k, known to be r5 */
	register int sum = 0;		/* on 16k, known to be r4 */
	register int mlen = len;

	for (;;) {

		/*
		 * Each trip around loop adds in
		 * word from one mbuf segment.
		 */

		w = m;

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
		 * we carefully use adwc.
		 */

		while ((mlen -= 32) >= 0) {

	/*
	 * 16k does not have autoincrement and incrementing pointer (w)
	 * affects the carry so roll out 8 explicit adds
	 */

	/*
	 * r4 starts at zero, or last addcd 0, r4 cleared carry, so
	 * there is no carry for first addd
	 */
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
		}
		mlen += 32;
		while ((mlen -= 8) >= 0) {

	/*
	 * either the carry is already cleared from above loop, or
	 * there weren't 32 bytes to checksum and r4 is 0,
	 * or addr at end of this loop leaves cleared carry, so
	 * addd then addcd to get 8 byte chuncks
	 */
			asm("addd 0(r5), r4");
			asm("addcd 4(r5), r4");
			asm("addcd 0, r4"); 	/* get carry from last add */
			asm("addr 8(r5), r5"); 	/* increment pointer (w) */
		}
		mlen += 8;

		/*
		 * Now eliminate the possibility of carry-out's by
		 * folding back to a 16 bit number (adding high and
		 * low parts together.)  Then mop up trailing words
		 * and maybe an odd byte.
		 */

		{ asm("movd r4, r0");
		  asm("ashd -16, r0");
		  asm("addw r0, r4");
		  asm("addcd 0, r4");
		  asm("movzwd r4, r4");
		}
		while ((mlen -= 2) >= 0) {
			asm("movzwd 0(r5), r0");
			asm("addd r0, r4");	/* carry goes into upper */
			asm("addqd 2, r5");	/* increment pointer (w) */
		}
		if (mlen == -1)
			sum += *(u_char *)w;

		break;

		/*
		 * Locate the next block with some data.
		 * If there is a word split across a boundary we
		 * will wrap to the top with mlen == -1 and
		 * then add it in shifted appropriately.
		 */
	}

	/*
	 * Add together high and low parts of sum
	 * and carry to get cksum.
	 * Have to be careful to not drop the last
	 * carry here.
	 */

	{ asm("movd r4, r0");
	  asm("ashd -16, r0");
	  asm("addw r0, r4");
	  asm("addcd 0, r4");
	  asm("comd r4, r4");
	  asm("movzwd r4, r4");
	}
	return (sum);
}

#else

/*
 *                      I N _ C K S U M
 *
 * Checksum routine for Internet Protocol family headers (C Version)
 *
 */

in_cksum(addr, len)
	u_short *addr;
	int len;
{
        register int nleft = len;
        register u_short *w = addr;
        register u_short answer;
        register int sum = 0;

        /*
         *  Our algorithm is simple, using a 32 bit accumulator (sum),
         *  we add sequential 16 bit words to it, and at the end, fold
         *  back all the carry bits from the top 16 bits into the lower
         *  16 bits.
         */

        while( nleft > 1 )  {
                sum += *w++;
                nleft -= 2;
        }

        /*
	 * mop up an odd byte, if necessary
	 */

        if( nleft == 1 )
                sum += *(u_char *)w;

        /*
         * add back carry outs from top 16 bits to low 16 bits
         */

        sum += (sum >> 16);     /* add hi 16 to low 16 */
        answer = ~sum;          /* truncate to 16 bits */
        return (answer);
}
#endif
