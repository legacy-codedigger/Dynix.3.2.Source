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

#ifdef RCS
static	char rcsid[] = "$Header: packet.c 2.1 87/04/14 $";
#endif

#include "rs.h"

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
	p = buffer;
	while (p < ptr)
		putchar(*p++);
}

/*
 * r p a c k
 *
 * Read a packet
 */

unsigned char				/* returns packet type */
rpack(len, num, data)
register int *len, *num;		/* packet length, number */
register unsigned char *data;		/* packet data */
{
	register int c, d;		/* character */
	register unsigned char *p;	/* for copying the data */
	unsigned char type;		/* packet type */
	unsigned cchksum, rchksum;	/* computed and rec'd chksums */
	register int i;			/* general purpose counter */

	/*
	 * look for the start
	 */
	for (c=0; c != SOH; ) {
		c = getch();
		if (c == -1)		/* check for timeout */
			return(0);
	}

#define CHECK(c)	{if (c == -1) return(0); if (c == SOH) goto resync;}

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

	/*
	 * validate the checksum
	 */
	if (rchksum != cchksum)
		return(0);

	return(type);			/* return packet type */
}

int
getch()
{
	register int c, i;
	extern int cpuspeed;

	for (i = (cpuspeed * 1000000); i--; )	/* timeout, with non-blocking read */
		if ((c = igetchar()) >= 0)
			break;
	return(c);
}
