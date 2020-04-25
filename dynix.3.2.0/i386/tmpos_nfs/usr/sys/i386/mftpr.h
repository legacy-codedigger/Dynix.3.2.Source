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
 * $Header: mftpr.h 2.20 1991/11/06 00:24:26 $
 *
 * mftpr.h
 *	Various asm functions to read/write processor special registers.
 *
 * i386 version.
 *
 * Historic name, stands for "move from/to processor registers".
 */

/* $Log: mftpr.h,v $
 *
 *
 */

/*
 * Code to read/write processor registers from C.
 */

#ifndef	lint				/* lint can't handle asm fcts yet */

asm READ_PTROOT()			/* page-table root */
{
/PEEPOFF				/* turn off peephole optimizer */
	movl	%cr3, %eax
/PEEPON					/* turn peephole opt back on */
}

asm WRITE_PTROOT(val)			/* page-table root */
{
%ureg val;
/PEEPOFF				/* turn off peephole optimizer */
	movl	val, %cr3
/PEEPON					/* turn peephole opt back on */

%mem val;
/PEEPOFF				/* turn off peephole optimizer */
	movl	val, %eax
	movl	%eax, %cr3
/PEEPON					/* turn peephole opt back on */
}

asm FLUSH_TLB()				/* flush TLB (11 cycles on 386) */
{
/PEEPOFF				/* turn off peephole optimizer */
	movl	%cr3, %eax
	movl	%eax, %cr3
/PEEPON					/* turn peephole opt back on */
}

asm void INVAL_ONCHIP_CACHE()
{
/PEEPOFF				/* turn off peephole optimizer */
	.byte	0x0f, 0x08		/* invd: using the manly encoding */
/PEEPON					/* turn peephole opt back on */
}

asm READ_MSW()				/* machine status word (CR0) */
{
/PEEPOFF				/* turn off peephole optimizer */
	movl	%cr0, %eax
/PEEPON					/* turn peephole opt back on */
}

asm WRITE_MSW(val)			/* machine status word (CR0) */
{
%con val;
/PEEPOFF				/* turn off peephole optimizer */
	movl	val, %eax
	movl	%eax, %cr0
/PEEPON					/* turn peephole opt back on */
%mem val;
/PEEPOFF				/* turn off peephole optimizer */
	movl	val, %eax
	movl	%eax, %cr0
/PEEPON					/* turn peephole opt back on */
}

asm READ_DSR()				/* Debug-Status Register (DR6) */
{
/PEEPOFF				/* turn off peephole optimizer */
	movl	%db6, %eax
/PEEPON					/* turn peephole opt back on */
}

asm WRITE_DSR(val)			/* Debug-Status Register (DR6) */
{
%con val;
/PEEPOFF				/* turn off peephole optimizer */
	movl	val, %eax
	movl	%eax, %db6
/PEEPON					/* turn peephole opt back on */
}

asm WRITE_DB0(val)			/* Debug-Register 0*/
{
%mem val;
/PEEPOFF				/* turn off peephole optimizer */
	movl	val, %eax
	movl	%eax, %db0
/PEEPON					/* turn peephole opt back on */
}
asm WRITE_DB1(val)			/* Debug-Register 0*/
{
%mem val;
/PEEPOFF				/* turn off peephole optimizer */
	movl	val, %eax
	movl	%eax, %db1
/PEEPON					/* turn peephole opt back on */
}
asm WRITE_DB2(val)			/* Debug-Register 0*/
{
%mem val;
/PEEPOFF				/* turn off peephole optimizer */
	movl	val, %eax
	movl	%eax, %db2
/PEEPON					/* turn peephole opt back on */
}
asm WRITE_DB3(val)			/* Debug-Register 0*/
{
%mem val;
/PEEPOFF				/* turn off peephole optimizer */
	movl	val, %eax
	movl	%eax, %db3
/PEEPON					/* turn peephole opt back on */
}
asm WRITE_DB6(val)			/* Debug-Register 6*/
{
%mem val;
/PEEPOFF				/* turn off peephole optimizer */
	movl	val, %eax
	movl	%eax, %db6
/PEEPON					/* turn peephole opt back on */
}
asm WRITE_DB7(val)			/* Debug-Register 0*/
{
%mem val;
/PEEPOFF				/* turn off peephole optimizer */
	movl	val, %eax
	movl	%eax, %db7
/PEEPON					/* turn peephole opt back on */
}

asm READ_DB0()				/* Debug-Status Register (DR6) */
{
/PEEPOFF				/* turn off peephole optimizer */
	movl	%db0, %eax
/PEEPON					/* turn peephole opt back on */
}

asm READ_DB1()				/* Debug-Status Register (DR6) */
{
/PEEPOFF				/* turn off peephole optimizer */
	movl	%db1, %eax
/PEEPON					/* turn peephole opt back on */
}

asm READ_DB2()				/* Debug-Status Register (DR6) */
{
/PEEPOFF				/* turn off peephole optimizer */
	movl	%db2, %eax
/PEEPON					/* turn peephole opt back on */
}

asm READ_DB3()				/* Debug-Status Register (DR6) */
{
/PEEPOFF				/* turn off peephole optimizer */
	movl	%db3, %eax
/PEEPON					/* turn peephole opt back on */
}

asm READ_DB6()				/* Debug-Status Register (DR6) */
{
/PEEPOFF				/* turn off peephole optimizer */
	movl	%db6, %eax
/PEEPON					/* turn peephole opt back on */
}

asm READ_DB7()				/* Debug-Status Register (DR6) */
{
/PEEPOFF				/* turn off peephole optimizer */
	movl	%db7, %eax
/PEEPON					/* turn peephole opt back on */
}
asm WRITE_DCR(val)			/* Debug-Control Register (DR7) */
{
%con val;
/PEEPOFF				/* turn off peephole optimizer */
	movl	val, %eax
	movl	%eax, %db7
/PEEPON					/* turn peephole opt back on */
}

#endif	lint

/*
 * newptes(vaddr, size)
 *	Insure the MMU TLB for the given address range is flushed of any
 *	stale cached map.  i386 has no per-page way to do this, thus flush
 *	entire TLB.  Doing it in-line avoids some of the overhead.
 *	The i486 can flush individual pages so use the routine newptes 
 *	found in vm_machdep.c.
 */

#define	newptes(vaddr,size)	FLUSH_TLB()

#ifndef lint

asm void INVAL_I486_TLB(addr)
{
%mem addr;
/PEEPOFF                                /* turn off peephole optimizer */
	movl	addr,%eax
	/* invlpg (%eax) */
	.byte   0x0f, 0x01              /* invlpg: using the manly encoding */
	.byte   0x38			/* DS:[eax] */
/PEEPON                                 /* turn peephole opt back on */
}
#endif /* lint */

/*
 * flush_tlb()
 *	flush kernel tlb in a "portable way. On the
 *	386, flush entire tlb.
 */
#define	flush_tlb()	FLUSH_TLB()
