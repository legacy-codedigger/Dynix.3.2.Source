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

/*
 * $Header: mmu.h 2.7 90/11/08 $
 *
 * mmu.h
 *	Various register field definitions and constants for the MMU.
 *
 * See the Intel 80386 Architecture Specification for more information.
 * These definitions taken from Revision 1.8 (6/15/85) of the spec.
 *
 * See machine/pte.h for definition of the page-table entry.
 * See machine/gdt.h for definition of segment/etc descriptors and Dynix usage.
 */

/* $Log:	mmu.h,v $
 */

/*
 * Control Register 0 (CR0) definitions.
 */

#define	CR0_PG		0x80000000	/* enable paging */
#define CR0_CD          0x40000000      /* disable caching */
#define CR0_NW          0x20000000      /* no write through */
#define CR0_AM          0x00040000      /* alignment mask */
#define CR0_WP          0x00010000      /* write protect */
#define	CR0_ET		0x00000010	/* extension type (0=287, 1=387) */
#define	CR0_TS		0x00000008	/* task switched */
#define	CR0_EM		0x00000004	/* emulate coprocessor */
#define	CR0_NE		0x00000020	/* numeric error mode */
#define	CR0_MP		0x00000002	/* monitor coprocessor */
#define	CR0_PE		0x00000001	/* protection enable */
#define	CR0_RSVD	0x7FFFFFE0	/* reserved bits, should be zero */

/*
 * Page Fault Error Code Format.
 *
 * Page fault error code is pushed after return information on a page fault.
 */

#define	PFEC_P		0x00000001	/* 0=not present, 1=protection fault */
#define	PFEC_WR		0x00000002	/* 0=read, 1=write */
#define	PFEC_US		0x00000004	/* 0=supervisor, 1=user fault */
#define	PFEC_RSVD	0xFFFFFFF8	/* reserved bits, don't rely on value */

/*
 * Breakpoint register definitions.
 * Debug Status Register (DSR) is DR6, Debug Control Register (DCR) is DR7.
 *
 * See Intel spec for more information.
 */

#define	DSR_B0		0x00000001	/* breakpoint 0 occurred */
#define	DSR_B1		0x00000002	/* breakpoint 1 occurred */
#define	DSR_B2		0x00000004	/* breakpoint 2 occurred */
#define	DSR_B3		0x00000008	/* breakpoint 3 occurred */
#define	DSR_BD		0x00002000	/* read/write debug register */
#define	DSR_BS		0x00004000	/* single-step breakpoint */
#define	DSR_BT		0x00008000	/* task-switch breakpoint */

#define	DSR_CLEAR	0x0		/* clear breakpoint status */
#define	DSR_WATCHPT	(DSR_B0|DSR_B1|DSR_B2|DSR_B3)	/* test for watchpoint*/

#define	DCR_L0		0x00000001
#define	DCR_G0		0x00000002
#define	DCR_L1		0x00000004
#define	DCR_G1		0x00000008
#define	DCR_L2		0x00000010
#define	DCR_G2		0x00000020
#define	DCR_L3		0x00000040
#define	DCR_G3		0x00000080
#define	DCR_GE		0x00000100
#define	DCR_LE		0x00000200
#define	DCR_GD		0x00002000
#define	DCR_LENRW0	0x000F0000
#define	DCR_LENRW1	0x00F00000
#define	DCR_LENRW2	0x0F000000
#define	DCR_LENRW3	0xF0000000

#define	DCR_OFF		0x0		/* turn OFF watchpoints */
