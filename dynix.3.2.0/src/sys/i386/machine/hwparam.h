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
 * $Header: hwparam.h 2.8 1991/05/01 00:01:07 $
 *
 * hwparam.h
 *	Define physical addresses and other parameters of the hardware.
 */

/* $Log: hwparam.h,v $
 *
 */

/*
 * Physical addresses in processor board address space.
 */

#ifdef	KXX

#define	PHYS_SLIC	0x1BF0000	/* phys loc in CPU addr space */
#define	PHYS_SLIC_INTR	0x1BF1000	/* slic's interrupt vector register */
#define	PHYS_CACHE0SET	0x1BF8000	/* cache 0 set data */
#define	PHYS_CACHE1SET	0x1BF9000	/* cache 1 set data */
#define	PHYS_STATICRAM	0x1BFA000	/* local static RAM */
#define	PHYS_CACHE0REV	0x1BFC000	/* cache set 0 (parity reversed) */
#define	PHYS_CACHE1REV	0x1BFD000	/* cache set 1 (parity reversed) */
#define	PHYS_RAMREV	0x1BFE000	/* local static RAM (parity reversed) */

#define	PHYS_MBAD	0x1C00000	/* MBAd base address (phys) */
#define PHYS_SSMVME     0x2000000       /* SSM/VME base address (phys) */

/*
 * Maximum processor addressable main-memory address is limited to 24Meg
 * on K20, due to naive mapping of SLIC in machine/startup.c
 * (alloclocalIO(), maplocalIO()).
 */

#define	MAX_PROC_ADDR_MEM (24*1024*1024)

#else	Real HW

#include "../balance/SGSproc.h"		/* SGS processor specific definitions */

extern  u_long	max_ker_vaddr;
#define MAX_PROC_ADDR_MEM max_ker_vaddr

#endif	KXX

/*
 * Define addressibility of Multi-bus interface.
 */

#define	VIRT_MBAD	PHYS_MBAD	/* MBAd base address (virt) */
#define	MBAD_ADDR_SPACE	(1024*1024)	/* 1M address space */

#define	VA_MBAd(i)	(VIRT_MBAD + (i) * MBAD_ADDR_SPACE)
#define	PA_MBAd(i)	(PHYS_MBAD + (i) * MBAD_ADDR_SPACE)

/*
 * For backwards compatibility -- MBAds started at fixed address.
 */

#define	OLD_VIRT_MBAD	0x0800000	/* MBAd base address (virt) */
/*
 * Define addressibility of SSM/VME bus interface.
 */

#define	VIRT_SSMVME	PHYS_SSMVME	/* SSM/VME memory base address (virt) */
#define	SSMVME_ADDR_SPACE	(32*1024*1024)	/* 32M address space */
#define	NPTE_SSMVME	howmany(SSMVME_ADDR_SPACE, NBPG)

#define	VA_SSMVME(i)	(VIRT_SSMVME + (i) * SSMVME_ADDR_SPACE)
#define	PA_SSMVME(i)	(PHYS_SSMVME + (i) * SSMVME_ADDR_SPACE)
