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
static char rcsid[]= "$Header: rs.c 2.2 86/04/04 $";
#endif

/*
 * rs.c:  Remote Serial port packet driver.
 */

/*
 * rs(UNIT, OFFSET)
 *  UNIT: Currently ignored, could be used to designate port in DH mode
 *	  implementation.  Current implementation uses DZ mode and default
 *	  port.
 *  OFFSET: Ignored.
 */

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include "saio.h"
#include "rs.h"

#define i_debug i_cyloff		/* unused parameter */
#define i_hostfd i_errcnt		/* unused in this driver */

static unsigned char packet[MAXPACKET];		/* packet buffer */
static int len;					/* packet length */
static int num;					/* packet number */

unsigned char rpack();

rsopen(io)
	register struct iob *io;
{
	register int i;
	register int type, retrycnt = 5;
	register unsigned char *p, *q;
	unsigned val;

retry:
	/*
	 * build the packet
	 */
	p = packet;
	val = io->i_howto;
	for (i = 8; i > 0; i--) {
		*p++ = BYTE(val & 0xf);
		val >>= 4;
	}
	q = (u_char *)io->i_fname;
	while (*q)
		*p++ = *q++;
	*p++ = 0;		/* terminate the name */

	/*
	 * send the packet
	 */
	spack(OPEN, p - packet, 0, packet);

	/*
	 * get the reply
	 */
	type = rpack(&len, &num, packet);
	if (type != ACK || num != 0 || len != 1) {	/* mis-sent packet */
		if (--retrycnt)
			goto retry;
		else {
			io->i_error = EHER;
			return;
		}
	}
	
	/*
	 * check if the file file opened
	 */
	io->i_hostfd = UNBYTE(packet[0]); /* save the host file descriptor */
	if (!io->i_hostfd)
		io->i_error = EBADF;
}

rsstrategy(io, func)
	register struct iob *io;
{
	if (func == READ)
		return(rsread(io));
	else
		return(rswrite(io));
}

rsclose(io)
	register struct iob *io;
{
	register int type, retrycnt;

	/*
	 * send the request
	 */
	retrycnt = 5;
retry:
	packet[0] = BYTE(io->i_hostfd);
	spack(CLOSE, 1, 0, packet);

	/*
	 * get the reply
	 */
	type = rpack(&len, &num, packet);
	if (type != ACK || num != 0 || len != 1) {	/* mis-sent packet */
		if (--retrycnt)
			goto retry;
		else {
			io->i_error = EHER;
			return;
		}
	}
	
	/*
	 * check if the file closed
	 */
	if (UNBYTE(packet[0]))
		io->i_error = EBADF;
}

rslseek(io, offset, whence)
	register struct iob *io;
{
	register int type, retrycnt = 5;
	register unsigned val;
	register unsigned char *p;
	register int i;

retry:
	/*
	 * build the data
	 */
	p = packet;		/* point to the buffer */
	*p++ = BYTE(io->i_hostfd);
	val = offset;
	for (i = 8; i > 0; i--) {
		*p++ = BYTE(val & 0xf);
		val >>= 4;
	}
	*p = BYTE(whence);

	/*
	 * send the request
	 */
	spack(LSEEK, 10, 0, packet);

	/*
	 * get the reply
	 */
	type = rpack(&len, &num, packet);
	if (type != ACK || num != 0 || len != 8) {	/* mis-sent packet */
		if (--retrycnt)
			goto retry;
		else {
			io->i_error = EHER;
			return(-1);
		}
	}
	
	/*
	 * build the return value
	 */
	val = 0;
	p = packet;
	for (i = 0; i < 8; i++)
		val |= UNBYTE(*p++) << (i * 4);
	return(val);
}

/*ARGSUSED*/
rsioctl(io, cmd, arg)
	register struct iob *io;
	int cmd;
	caddr_t arg;
{
	register flag;
	io->i_cc = 0;

	switch(cmd) {
	/*
	 * Set or Clear debug flags
	 */
	case SAIODEBUG:
		flag = (int)arg;
		if (flag > 0)
			io->i_debug |= flag;
		else
			io->i_debug  &= ~flag;
		break;

	default:
		printf("rs: bad ioctl (('%c'<<8)|%d)\n",
			(u_char)(cmd>>8),(u_char)cmd);
		io->i_error = ECMD;
		return -1;
	}
	return 0;
}


/*
 * r s r e a d
 */
int
rsread(io)
	struct iob *io;
{
	register int c;
	register unsigned char *p;
	register unsigned char *ptr;
	register unsigned val, count;
	register int i;
	int retrycnt = 5, type;
	int expect;

retry:
	/*
	 * build packet, with fd and count
	 */
	p = packet;
	*p++ = BYTE(io->i_hostfd);
	val = io->i_cc;
	for (i = 8; i > 0; i--) {
		*p++ = BYTE(val & 0xf);
		val >>= 4;
	}

	/*
	 * send packet
	 */
	spack(PREAD, 9, 0, packet);

	/*
	 * get command reply
	 */
	type = rpack(&len, &num, packet);
	if (type != ACK || num != 0 || len != 0) {	/* mis-sent packet */
		if (--retrycnt)
			goto retry;
		else {
			io->i_error = EHER;
			return(-1);
		}
	}

	/*
	 * now loop forever (until zero data size) getting data
	 */
	expect = 1;			/* sequence expected */
	ptr = (u_char *)io->i_ma;	/* point to where to unpack */
loop:
	retrycnt = 5;			/* set retry count each loop */
getdata:
	type = rpack(&len, &num, packet);
	if (type != DATA || num != expect) {
		if (--retrycnt) {
			spack(NAK, 0, expect, packet);
			goto getdata;
		} else {
			io->i_error = EHER;
			return(-1);
		}
	}
	/*
	 * was this the last packet ?
	 */
	if (len == 0) {
		spack(ACK, 0, expect, packet);
		return(ptr - (u_char *)io->i_ma);
	}

	/*
	 * unpack and unescape the data into the buffer
	 */
	p = packet;
	while (len--) {
		switch (*p) {
		case ESC:		/* next char is escaped */
			p++;		/* skip the ESC */
			*ptr++ = CTL(*p++);
			len--;
			break;
		case STX:		/* next char is count */
			p++;
			count = UNBYTE(*p++);
			c = *p++;
			if (c == ESC) {
				c = CTL(*p++);
				len--;
			}
			while (count--)
				*ptr++ = c;
			len -= 2;
			break;
		case ETX:		/* next two chars are count */
			p++;
			count = WORD(p[0], p[1]);
			p += 2;
			c = *p++;
			if (c == ESC) {
				c = CTL(*p++);
				len--;
			}
			while (count--)
				*ptr++ = c;
			len -= 3;
			break;
		default:
			*ptr++ = *p++;
			break;
		}
	}

	/*
	 * send ACK pack and loop
	 */
	spack(ACK, 0, expect, packet);
	expect = ++expect % 64;
	goto loop;
}

/*
 * r s w r i t e
 */
int
rswrite(io)
	struct iob *io;
{
	register unsigned char *p, *ptr;
	register unsigned char *buf;
	int retrycnt = 5;
	register int expect, sent = 0, cnt;
	register unsigned char type;
	unsigned count;

	buf = (u_char *)io->i_ma;
	count = io->i_cc;
retry:
	/*
	 * build packet, with fd
	 */
	p = packet;
	*p = BYTE(io->i_hostfd);

	/*
	 * send packet
	 */
	spack(PWRITE, 1, 0, packet);

	/*
	 * get command reply
	 */
	type = rpack(&len, &num, packet);
	if (type != ACK || num != 0 || len != 0) {	/* mis-sent packet */
		if (--retrycnt)
			goto retry;
		else {
			io->i_error = EHER;
			return(-1);
		}
	}

	/*
	 * now loop forever (until zero data size) sending data
	 */
	expect = 1;			/* sequence expected */
loop:
	retrycnt = 5;			/* set retry count each loop */
senddata:
	/*
	 * build a packet
	 */
	if (count == 0) {
		/*
		 * send a null packet
		 */
		spack(DATA, 0, expect, packet);

		/*
		 * get his ACK pack
		 */
		type = rpack(&len, &num, packet);
		if (type != ACK || num != expect || len != 0) { /* mis-sent packet */
			if (--retrycnt)
				goto senddata;
			else {
				io->i_error = EHER;
				return(-1);
			}
		}
		return(sent);
	}

	/*
	 * just send the data as BYTE format for now
	 */
	ptr = packet;
	for (cnt=0; (cnt < 1024) && (cnt < count); cnt++) {
		*ptr++ = BYTE(buf[cnt] & 0xf);
		*ptr++ = BYTE(buf[cnt] >> 4);
	}

	/*
	 * send the packet
	 */
	spack(DATA, cnt * 2, expect, packet);

	/*
	 * get the reply
	 */
	type = rpack(&len, &num, packet);
	if (type != ACK || num != expect || len != 1) {
		if (--retrycnt)
			goto senddata;
		else {
			io->i_error = EHER;
			return(-1);
		}
	}

	if (UNBYTE(packet[0]))	{	/* he got an error */
		io->i_error = EHER;
		return(-1);
	}

	sent += cnt;
	buf += cnt;
	count -= cnt;
	expect = ++expect % 64;
	goto loop;
}
