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

/* @(#)$Header: remote.c 1.4 86/03/12 $ */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include "remote.h"
#include "host.h"

int maxpacket = PACKETSIZE;		/* default maxpacket size */

unsigned char esc_list[50] = { SOH, STX, ETX, '\r', DLE, DC1, DC3, ESC };

static unsigned char packet[MAXPACKET];	/* packet data */
static int num;				/* packet number */
static int len;				/* packet length */

docommand()
{
	register unsigned char type;

	type = rpack(1, &len, &num, packet);
	switch (type) {
	default:
	case ACK:		/* unexpected here, reject */
	case NAK:		/* unexpected here, reject */
	case DATA:		/* unexpected here, reject */
		spack(NAK, 0, 0, packet);
		return;
	case OPEN:		/* open a file */
		ropen();
		break;
	case CLOSE:		/* close a file */
		rclose();
		break;
	case LSEEK:		/* seek a file */
		rlseek();
		break;
	case PREAD:		/* read a file */
		rread();
		break;
	case PWRITE:		/* write a file */
		rwrite();
		break;
	case SCRIPT:		/* change script file */
		script();
		break;
	case NOSCRIPT:		/* turn off script file */
		noscript();
		break;
	}
}

ropen()
{
	register int fd, i;
	register unsigned char *p;
	register int how;

	how = 0;
	p = packet;
	for (i=0; i < 8; i++)
		how |= UNBYTE(*p++) << (i * 4);
	
	/*
	 * the file name consists of "rs(0,0)name" and the "rs(0,0)" part
	 * is currently unused.  It is hereby discarded.
	 */
	while (*p && *p != ')')
		p++;
	p++;				/* skip the ) too */

	if (how >= 1)
		how |= O_CREAT;		/* use new open with create */
	fd = open(p, how, 0666);
	if (fd < 0) {
		fprintf(stderr, "\r\n%s: can't open %s\r\n", myname, p);
		packet[0] = BYTE(0);
	} else
		packet[0] = BYTE(fd);

	spack(ACK, 1, 0, packet);
}

rclose()
{
	register int fd;

	fd = UNBYTE(packet[0]);		/* get the file descriptor */
	if (close(fd))
		packet[0] = BYTE(1);
	else
		packet[0] = BYTE(0);
	spack(ACK, 1, 0, packet);	/* send the ACK pack */
}

rlseek()
{
	register int fd, i;
	register int offset, whence;
	register unsigned char *p;

	p = packet;
	fd = UNBYTE(*p++);

	offset = 0;
	for (i = 0; i < 8; i++)
		offset |= UNBYTE(*p++) << (i * 4);
	
	whence = UNBYTE(*p);

	offset = lseek(fd, offset, whence);

	p = packet;
	for (i = 8; i > 0; i--) {
		*p++ = BYTE(offset & 0xf);
		offset >> 4;
	}

	spack(ACK, 8, 0, packet);
}

rread()
{
	register int fd, request;		/* file descriptor, count */
	register unsigned char *p, *q, *r;
	register int c, cc, cnt;
	unsigned char rpacket[MAXPACKET];	/* read and receive packet */
	unsigned char type;			/* packet type */
	int retrycnt, expect = 1, rlen, rnum, eof = 0;

	/*
	 * get the file descriptor out of the packet
	 */
	p = packet;
	fd = UNBYTE(*p++);

	/*
	 k get the request count out of the packet
	 */
	request = 0;
	for (c=0; c < 8; c++)
		request |= UNBYTE(*p++) << (c * 4);

	/*
	 * send the ACK pack
	 */
	spack(ACK, 0, 0, packet);

	/*
	 * now, collect data, escaping and compressing, transmit until file
	 * fails, or request is exausted
	 */
loop:
	/*
	 * build the packet
	 */
	len = 0;			/* zero the length counter */
	if (request == 0)		/* treat a fulfilled request as eof */
		eof++;
	if (!eof) {
		cc = read(fd, rpacket, MAXPACKET);	/* read some data */
		if (cc < 0)
			fprintf(stderr, "\r\n%s: read error\r\n", myname);
		if (cc <= 0)
			eof++;
	}
	q = rpacket;			/* point to the read packet */
	p = packet;			/* point to the destination packet */
	while ((p-packet) < maxpacket && eof == 0 &&
	       (q-rpacket) < cc      && request > 0) {
		/* 
		 * check for compression
		 */
		c = *q++;
		cnt = 1;
		request--;
		while (request > 0 && eof == 0) {
			if ((q-rpacket) >= cc) {
				cc = read(fd, rpacket, MAXPACKET);
				if (cc < 0)
					fprintf(stderr,"\r\n%s: read error\r\n",
						myname);
				if (cc <= 0)
					eof++;
				q = rpacket;
			}
			if ((q-rpacket) >= cc)
				break;
			if (c != *q)		/* match for compression */
				break;
			if (cnt >= 94 * 94)
				break;
			q++;			/* skip identical char */
			cnt++;			/* increment count */
			request--;		/* count this char */
		}
		/*
		 * see if the char matches the escape list
		 */
		r = esc_list;
		while (*r) {
			if (*r == c)
				break;
			r++;
		}
		/*
		 * put the cnt chars into the packet
		 */
		if (cnt < 4) {
			while (cnt--) {
				if (*r) {
					*p++ = ESC; *p++ = CTL(c); len += 2;
				} else {
					*p++ = c; len++;
				}
			}
		} else if (cnt <= 94) {		/* output STX escape */
			*p++ = STX;
			*p++ = BYTE(cnt);
			if (*r) {
				*p++ = ESC; *p++ = CTL(c); len += 4;
			} else {
				*p++ = c; len += 3;
			}
		} else {			/* output ETX escape */
			*p++ = ETX;	
			UNWORD(cnt, p);
			if (*r) {
				*p++ = ESC; *p++ = CTL(c); len += 5;
			} else {
				*p++ = c; len += 4;
			}
		}
	}
	if (!eof)
		lseek(fd, -(cc - (q-rpacket)), 1);	/* seek back to here */

	/*
	 * send the data, or eof if none
	 */
	retrycnt = 5;
retry:
	spack(DATA, len, expect, packet);
	type = rpack(0, &rlen, &rnum, rpacket);
	if (type != ACK || rlen != 0 || rnum != expect) {
		if (--retrycnt)
			goto retry;
		else
			return;
	}
	if (eof && len == 0)
		return;

	putc(':', stderr);		/* notify the use that a packet went */
	expect = ++ expect % 64;
	goto loop;
}

rwrite()
{
	register int fd;
	register unsigned char *p, *q;
	register int expect = 1;
	register unsigned char type;
	register int retrycnt, i;
	unsigned char buf[MAXPACKET];

	/*
	 * get the file descriptor
	 */
	fd = UNBYTE(packet[0]);
	spack(ACK, 0, 0, packet);

	/*
	 * loop getting DATA packets, writing them to disk, and acking
	 */
loop:
	retrycnt = 5;
retry:
	type = rpack(0, &len, &num, packet);
	if (type != DATA || num != expect) {
		spack(NAK, 0, expect, packet);
		if (--retrycnt)
			goto retry;
		else
			return;
	}
	
	/*
	 * is this the last packet ?
	 */
	if (len == 0) {
		spack(ACK, 0, expect, packet);
		return;
	}

	p = buf; q = packet;
	while (len > 0) {
		*p = UNBYTE(*q++);
		*p++ |= UNBYTE(*q++) << 4;
		len -= 2;
	}

	i = write(fd, buf, p - buf);
	if (i < 0) {			/* error, this is the last */
		packet[0] = BYTE(1);
		return;
	} else
		packet[0] = BYTE(0);
	spack(ACK, 1, expect, packet);

	putc('.', stderr);		/* notify the use that we got packet */
	expect = ++expect % 64;
	goto loop;
}

script()
{
	if (scriptfp)
		fclose(scriptfp);
	if (scriptfp = fopen(packet, "a")) {
		fprintf(stderr, "\r\n%s: open script file %s\r\n",
			myname, packet);
		packet[0] = BYTE(0);
	} else {
		fprintf(stderr, "\r\n%s: error opening script file %s\r\n",
			myname, packet);
		packet[0] = BYTE(1);
	}
	fflush(stdout);
	spack(ACK, 1, 0, packet);
}

noscript()
{
	if (scriptfp) {
		fclose(scriptfp);
		scriptfp = 0;
	}
	spack(ACK, 0, 0, packet);
}
