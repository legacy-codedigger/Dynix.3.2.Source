/* @(#)$Copyright:	$
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

/* @(#)$Header: packet.c 1.4 86/03/12 $ */

#include <stdio.h>
#include <sgtty.h>
#include <signal.h>
#include <setjmp.h>
#include "remote.h"
#include "host.h"

FILE *packetdebug;		/* for watching packets go by */

/*
 * s p a c k
 *
 * Write a packet.
 */

spack(type, len, num, data)
unsigned char type;			/* packet type */
register int len, num;			/* packet length, number */
register unsigned char *data;		/* packet data */
{
	register unsigned char *p;	/* for copying the data */
	register int chksum = 0;	/* computed chksum */
	unsigned char buffer[MAXPACKET+30]; /* buffer */
	register unsigned char *ptr;	/* pointer into buffer */
	register int i;			/* general purpose counter */

	/*
	 * build the packet
	 */
	/*
	 * put the header information into buffer
	 */
	ptr = buffer;			/* point to the buffer again */
	*ptr++ = SOH;			/* Packet marker */

	UNWORD(len+4, ptr);		/* put the byte count in place */
	chksum += ptr[-2] + ptr[-1];	/* add into checksum */

	*ptr = BYTE(num);		/* packet sequence number */
	chksum += *ptr++;		/* add into checksum */

	*ptr = type;			/* packet type */
	chksum += *ptr++;		/* add into checksum */

	p = data;			/* point to the data */
	for (i=0; i < len; i++) {
		*ptr = *p++;		/* copy the data */
		chksum += *ptr++;
	}

	/*
	 * put the checksum into the packet
	 */
	chksum %= 94 * 94;		/* bring it into range */
	UNWORD(chksum, ptr);		/* put the ckecksum in */

	/*
	 * transmit the packet
	 */
	write(port, buffer, ptr - buffer);
	if (packetdebug) {
		p = buffer;
		while (p < ptr) {
			i = *p++;
			putc(i, packetdebug);
		}
		fprintf(stderr, "send type=%c num=%d len=%d\r\n",
			type < ' ' ? type | 0x40 : type, num, len);
	}
}

static jmp_buf	rpenv;			/* environment for timeout */

/*
 * r p _ a l r m
 *
 * read packet alarm clock handler.
 */
static
rp_alrm()
{
	signal(SIGALRM, SIG_IGN);
	longjmp(rpenv, 1);
}

/*
 * r p a c k
 *
 * Read the remainder of a packet, the SOH may have already been seen.
 */

unsigned char				/* returns packet type */
rpack(hdr, len, num, data)
int hdr;				/* packet header seen */
register int *len, *num;		/* packet length, number */
register unsigned char *data;		/* packet data */
{
	register int c, d;		/* character */
	register unsigned char *p;	/* for un-escaping the data */
	unsigned char type;		/* packet type */
	unsigned cchksum, rchksum;	/* computed and rec'd chksums */
	register int i;			/* general purpose counter */

	if (setjmp(rpenv))
		return(0);		/* timed out, fail */
	signal(SIGALRM, rp_alrm);
	alarm(10);			/* should get a packet in 10 s */

	if (hdr == 0) 			/* header not yet seen */
		for (c=0; c != SOH; )
			c = getch();
	if (packetdebug)
		putc(SOH, packetdebug);

#define CHECK(c)	if (c == SOH) goto resync; \
			if (packetdebug) putc(c, packetdebug)
resync:
	/*
	 * get the byte count
	 */
	c = getch(); CHECK(c); cchksum = c;
	d = getch(); CHECK(d); cchksum += d;
	*len = WORD(c, d) - 4;	/* calculate the bytecount */

	/*
	 * get the sequence number
	 */
	c = getch(); CHECK(c); cchksum += c;
	*num = UNBYTE(c);

	/*
	 * get the packet type
	 */
	c = getch(); CHECK(c); cchksum += c;
	type = c;
	if (packetdebug)
		fprintf(stderr, "recv type=%c num=%d len=%d",
				type < ' ' ? type|0x40 : type, *num, *len);
	/* 
	 * get the data
	 */
	p = data;
	for (i = *len; i > 0; i--) {
		c = getch(); CHECK(c); cchksum += c;

		*p++ = c;
	}

	/*
	 * get the checksum
	 */

	c = getch(); CHECK(c); 

	d = getch(); CHECK(d);

	rchksum = WORD(c, d);
	cchksum %= 94 * 94;

	alarm(0);			/* turn off the alarm clock */
	if (packetdebug)
		fprintf(stderr, " cck=%d rck=%d\r\n", cchksum, rchksum);
	/*
	 * validate the checksum
	 */
	if (rchksum != cchksum)
		return(0);

	return(type);			/* return packet type */
}
