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

/* $Header: mbadinit.c 2.0 86/01/28 $ */

/*
 * mbadinit.c
 *	Various procedures dealing with MBIf's.
 */


#include <sys/types.h>
#include "mbad.h"
#include <machine/cfg.h>
#include <machine/mftpr.h>
#include <machine/param.h>

/*
 * Initialize multibus adaptor for standalone use
 * Map 0-256K main mem to 768K-1M on multibus.
 */

mbadinit(mbad)
{
	register struct mb_ios *m;
	register i;

	m = (struct mb_ios *) ((char *)MB_IOSPACE + (mbad * MB_IODELTA));
	for (i=0; i < 256; i++) {
		*(short *)&m->mb_map[i] = i;
	}
	m->mb_ctl[0] = 0x01;		/* enable map register */
}

#ifndef BOOTXX
/*
 * mbad_physmap()
 *	Set up DMA transfer to physical memory in MBAd map registers.
 *
 * addr argument is physical-addr of target/source of DMA.
 *
 * Returns DMA address to tell the multi-bus device.
 *
 * No checking on legality of arguments.
 */

#ifdef	DEBUG
int	mbad_debug = 0;		/* get verbose flag */
#endif	DEBUG

#define	MB_MAPS		256		/* # mapping registers */
#define	MB_MRSIZE	1024		/* size mapped by one register */

#define	MBADMAP(mb,i)	(*(u_short*)(&((mb)->mb_map[i])))
#define	PHYSTOMB(paddr)	(((unsigned)(paddr))/MB_MRSIZE)

unsigned
mbad_physmap(mbad, dmabase, addr, len, nmap)
	register int	dmabase;		/* 1st map register */
	caddr_t		addr;			/* start address to map */
	unsigned	len;			/* # bytes to map */
	int		nmap;			/* # map registers (sanity) */
{
	register struct	mb_ios	*mb;		/* mbad I/O space */
	register int count;
	unsigned offset;
	unsigned val;

	mb = (struct mb_ios *) ((char *)MB_IOSPACE + (mbad * MB_IODELTA));
	offset = (int)addr & (MB_MRSIZE-1);
	count = (offset + len + (MB_MRSIZE-1)) / MB_MRSIZE;
	if(count > nmap) {
		printf("mbad_physmap: too big\n");
		return(-1);
	}

	val = MB_RAMBASE + dmabase * MB_MRSIZE + offset;

	for(; count--; addr += MB_MRSIZE, ++dmabase) {
		if(dmabase > MB_MAPS) {
			printf("mbad_physmap: bad map\n");
			return(-1);
		}
#ifdef	DEBUG
		if (mbad_debug)
			printf("MBAd map[%d] = 0x%x\n", dmabase, PHYSTOMB(addr));
#endif	DEBUG
		MBADMAP(mb, dmabase) = PHYSTOMB(addr);
	}

	return(val);
}
#endif
