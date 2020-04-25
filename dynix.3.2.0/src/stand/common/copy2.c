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
static char rcsid[] = "$Header: copy2.c 2.3 90/02/26 $";
#endif

#include <stdio.h>

int _buf_size = 0;
extern int errno;
char *bufptr;
unsigned int checksum = 0;
unsigned int gchecksum = 0;
char *calloc();	

#ifdef	DEBUG
int debug;
#endif

main(argc, argv)
	int	argc;
	char	**argv;
{
	register pos, i, o, bs, count;
	register opos, ipos;
	register n;
	register limit, left, verifypass;
	register gcount, gbs, gleft;
	register bytecount, vbytecount;
	char buf[128], input[128], output[128];

	vbytecount = bytecount = verifypass = pos = 0;
	printf("Copy2 program\n");

	if ( argc == 3 ) {
		i = open(argv[1], 0);
		o = open(argv[2], 2);
		limit = 0;
		gleft = 0;
		left =  0;
		opos = 0;
		ipos = 0;
		set_buf_size(buf);
		bs = _buf_size/512;
		gbs = bs;
		count = -1;
		gcount = count;
		if ( (i >= 0) && (o >= 0) )
			goto verifyloop;
	}
	do {
		printf("Input file?");
		gets(input);
		i = open(input, 0);
	} while (i <= 0);

	do {
		printf("Output file?");
		gets(output);
		o = open(output, 2);
	} while (o <= 0);

	printf("Verify?"); gets(buf);
	if (buf[0] == 'y' || buf[0] == 'Y')
		verifypass = 1;

	printf("Seek to Output block?"); gets(buf); opos = atoi(buf) * 512;

	printf("Seek to Input block?"); gets(buf); ipos = atoi(buf) * 512;

	printf("Limit total transfer count?"); gets(buf); limit = atoi(buf);
	if (limit < 0)
		limit = 0;
	gleft = left = limit;

	set_buf_size(buf);
	printf("%d blocks available (%d.%d Meg)\n", 
		_buf_size/512, _buf_size >> 20, 
		(_buf_size % (1024*1024)) / ((1024*1024)/10));
	printf("Buffer by?", _buf_size); gets(buf); bs = atoi(buf);
	if (bs <= 0) {
		bs = _buf_size/512;
		printf(">>> %d used\n", bs);
	}
	gbs = bs;

	printf("Count?"); gets(buf); count = atoi(buf);
	if (count == 0) {
		count = -1;
		printf(">>> infinite count used\n");
	}
	gcount = count;

#ifdef DEBUG
	printf("Debug level?"); gets(buf); debug = atoi(buf);
	if (debug < 0)
		debug = 0;
#endif

	printf("Last chance...."); gets(buf);

verifyloop:
	bs *= 512;
	left *= 512;
	while(count--) {
		if (count == -2)
			count = -1;
		if (limit) {
			if (left <= 0) {
				printf(">>> hit limit of %d blocks\n", limit);
				goto done;
			}
			if (left < bs)
				bs = left;
		}
		lseek(i, pos+ipos, 0);
		if (vbytecount && (bytecount + bs) > vbytecount)
			bs =  vbytecount - bytecount;
#ifdef	DEBUG
		if (debug)
			printf("read of %d bytes returned ", bs);
#endif
		if ((n=read(i, bufptr, bs)) < 0) {
			printf("read err blk %d %d\n", (ipos+pos)/512, n);
			exit(errno);
		}
#ifdef	DEBUG
		if (debug)
			printf("%d bytes\n", n);
#endif
		if (n == 0) {			/* EOF */
			printf(">>> EOF\n");
			goto done;
		}
#undef roundup
#define	roundup(x, y)	((((x)+((y)-1))/(y))*(y))
		/* 
		 * Handle short reads by zeroing rest of buffer and 
		 * rounding up read size by 512 bytes.
		 */
		if (n < bs) {
			bzero(bufptr+n, bs-n);
			bs = n = roundup(n, 512);
		}
		if (verifypass) { 
#ifdef	DEBUG
			if (debug)
				printf("..summing %d bytes at 0x%x..", n, bufptr);
#endif
			sum((short *)bufptr, n);
#ifdef	DEBUG
			if (debug)
				printf("\n");
#endif
		}
		if (verifypass != 2) {
			lseek(o, pos+opos, 0);
#ifdef	DEBUG
			if (debug)
				printf("write of %d bytes returned ", bs);
#endif
			if ((n=write(o, bufptr, bs)) != bs) {
				printf("write err blk %d\n", (pos+opos)/512, n);
				exit(errno);
			}
#ifdef	DEBUG
			if (debug)
				printf("%d bytes\n", n);
#endif
		}
		pos += bs;
		bytecount += bs;
		if (limit)
			left -= bs;
#ifdef	DEBUG
		if (debug)
			printf(">>> left %d, vbytecount %d, bytecount %d, checksum %x\n",
				left, vbytecount, bytecount, checksum);
#endif
		if (vbytecount && bytecount >= vbytecount)
			goto done;
	}
done:
	close(i);
	if (verifypass == 1) {
		do {
			checksum = (checksum & 0xffff) | (checksum >> 16);
		} while (checksum & ~0xffff);
		if (bytecount % 512)
			printf("internal error: non 512 byte multiple byte count (%d)\n", bytecount);
		printf("Pass 1, count \"%d\", checksum \"%x\", doing verify\n", bytecount/512, checksum);
		close(o);
		i = open(output, 0);
		if (i < 0) {
			printf("open error on %s\n", output);
			exit(errno);
		}
		ipos = opos;
		gchecksum = checksum;
		pos = checksum = 0;
		verifypass = 2;
		count = gcount;
		left = gleft;
		bs = gbs;
		vbytecount = bytecount;
		bytecount = 0;
		goto verifyloop;
	}
	if (verifypass == 2) {
		do {
			checksum = (checksum & 0xffff) | (checksum >> 16);
		} while (checksum & ~0xffff);
		if (bytecount % 512)
			printf("Non 512 byte multiple byte count (%d)\n", bytecount);
		printf("Pass 2, count \"%d\", checksum \"%x\"\n", bytecount/512, checksum);
		if (bytecount != vbytecount)
			printf("*** ERROR *** bytecount mismatch\n");
		if (checksum != gchecksum)
			printf("*** ERROR *** checksum mismatch\n");
	} else {
		close(o);
	}
	printf("Done\n");
	exit(0);
}

#undef roundup
#undef MIN
#undef MAX_MEM
#define	roundup(a,b)	(((int)(a)+((b)-1)) & ~((b)-1))
#define MIN(a,b)	((a)>(b)?(b):(a))
/* limit our selves to 4M since some SCSI drivers may have problems! */
#define MAX_MEM		(4*1024*1024)

set_buf_size(x)
{
	register char *p;

	/*
	 * CAVEAT: assumes that rest of memory sans stack is available
	 * to copy2. That is, no driver (execpt when opened early in the
	 * main) will attempt to allocate more memory.
	 */
	callocrnd(1024);
	bufptr = calloc(0);

	/* allow 50k for stack growth */
	p = (char *) ((int)&x - (50 * 1024));

	/* round below 4M so SCSI devices happy */
	p = (char *) MIN((int)p, MAX_MEM);
	p = (char *) roundup(p, 1024);
	_buf_size = ((int)p - (int)bufptr);
}

sum(p, n)
	register short *p;
	register n;
{
	register unsigned int sum = checksum;

	if (n & 01)
		printf("copy2: internal error: summing an odd count\n");
	n /= sizeof(short);
	while (n-- > 0) {
		if (sum & 01)
			sum = (sum>>1) | 0x80000000;
		else
			sum >>= 1;
		sum += (*p++ & 0xffff);
	}
	checksum = sum;
}
