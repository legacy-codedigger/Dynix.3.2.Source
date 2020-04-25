/* $Header: buffer.c 1.7 1991/07/01 16:09:12 $ */

/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
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
 * $Log: buffer.c,v $
 *
 *
 */

#ifndef lint
static char rcsid[] = "$Header: buffer.c 1.7 1991/07/01 16:09:12 $";
#endif

#include	"crash.h"
#include	<sys/buf.h>

#ifndef BSD
#include	<sys/sysmacros.h>
#endif

int nbuf;
struct buf *bufp;
struct buf *readbuf();

buffer_init()
{
	readv(search("buf"), &bufp, sizeof bufp);
#ifdef BSD
	readv(search("nbuf"), &nbuf, sizeof nbuf);
#else
	nbuf = v.v_buf;
#endif

}

Bufhdr()
{

	struct buf *bb, *b = bufp;
	register int j;
	char *arg;
	int count, slt;

	if (live)
		buffer_init();

#ifdef BSD
	printf("BUF   ADDR   COUNT MAJ  MIN   BLKNO      VNODE  TYPE FLAGS\n");
#else
	printf("BUF   ADDR   COUNT      DEVP   BLKNO      VNODE  TYPE FLAGS\n");
#endif
						
	if ((arg = token()) == NULL) {		/* print the works */
		for (j = nbuf; j > 0; b++, j--) {
			if ((bb = readbuf(b)) == NULL)
				continue;
			if (bb->b_flags & B_INVAL)
				continue;
			printf("%-3d %#6x", nbuf - j, b);
			prbufhdr(bb);
		}
	} else if (memcmp("0x", arg, 2) == 0) {/* buffer address supplied */
		b = (struct buf *)atoi(arg);
		slt = b - bufp;
		printf("%-3d %#6x", slt, b);
		if( (bb = readbuf(atoi(arg))) != NULL)
			prbufhdr(bb);
	} else {				/* selective print */
		slt = atoi(arg);
		b += slt;
		count = atoi(token());
		if (!count) count = 1;
		count = ((slt + count) > nbuf) ? (nbuf - slt) : count;
		for (j = count; j > 0; b++, j--, slt++) {
			if ((bb = readbuf(b)) == NULL)
				continue;
			printf("%-3d %#6x", slt, b);
			prbufhdr(bb);
		}
	}	
}

prbufhdr(b)
register struct buf *b;
{

	printf(" %-5d ", b->b_bcount);
#ifdef BSD
	printf("%3.3x %4.4x ", major(b->b_dev), minor(b->b_dev));
#else
	printf("%#8x ", b->b_devp);
#endif
	printf("%7d %#10x", b->b_blkno, b->b_vp);

	switch(b->b_iotype) {
	case (B_FILIO):
		printf("  %s  ", "fil");
		break;
	case (B_RAWIO):
		printf("  %s  ", "raw ");
		break;
	case (B_PTEIO):
		printf("  %s  ", "pte ");
		break;
	case (B_PTBIO):
		printf("  %s  ", "ptb ");
		break;
	}
	printf("%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
	b->b_flags & B_WRITE ? "wrt " : "",
	b->b_flags & B_READ ? "rd " : "",
	b->b_flags & B_ERROR ? "err " : "",
	b->b_flags & B_PHYS ? "phy " : "",
	b->b_flags & B_AGE ? "age ": "",
	b->b_flags & B_ASYNC ? "asy ": "",
	b->b_flags & B_DELWRI ? "del ": "",
	b->b_flags & B_TAPE ? "tap ": "",
	b->b_flags & B_UAREA ? "uar ": "",
	b->b_flags & B_PAGET ? "pag ": "",
	b->b_flags & B_DIRTY ? "dir ": "",
	b->b_flags & B_PGIN ? "pgi ": "",
	b->b_flags & B_INVAL ? "inv ": "",
	b->b_flags & B_HEAD ? "hed ": "",
	b->b_flags & B_CALL ? "call ": "",
	b->b_flags & B_IOCTL ? "ioc ": "",
	b->b_flags & B_REALLOC ? "rlc ": "",
	b->b_flags & B_NOTREF ? "nrf ": "",
	b->b_flags & B_NOCLR ? "ncl ": "");
#ifdef B_SYNC
	if (b->b_flags & B_SYNC) 
		printf("syn ");
#endif
#ifdef B_EXPRESS
	if (b->b_flags & B_EXPRESS) 
		printf("exp ");
#endif
#ifdef B_NOCACHE
	 if (b->b_flags & B_NOCACHE)
		printf("nca ");
#endif
	return;

}

buffer()
{
}

struct buf *
readbuf(bb)
	struct buf *bb;
{

	static struct buf bufbuf;

	if (readv(bb, &bufbuf, sizeof bufbuf) != sizeof bufbuf) {
		printf("read error on buf\n");
		return NULL;
	}
	return &bufbuf;

}

