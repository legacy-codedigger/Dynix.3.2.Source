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

#include "assym.h"

/* $Header: crt0.s 1.8 1991/06/03 20:19:55 $
 *
 * start()
 *
 * Entry point for standalone programs.  Standalone runs
 * in a flat 4 Gig segment with interrupts disabled.
 */
	.text
	.globl	start
	.align	2
start:
	movl	$CD_LOC,%eax		/* pointer to config data */
	movl	c_version(%eax),%ebx
	orl	%ebx,%ebx
	je	oldfw
	movl	c_bottom(%eax),%esp	/* set initial stack pointer */
	pushl	%eax			/* address of CD_LOC*/
	jmp common
oldfw:
	movl	$CFG_PTR,%eax		/* pointer to configuration */
	movl	head_cfg(%eax),%eax	/* pointer to cfg_boot */
	movl	b_bottom(%eax),%esp	/* load stack pointer */
common:
	CALL	_slic_init		/* set up slic stuff */
	CALL	_clearbss		/* clear our own bss */
#ifndef BOOTXX
	pushl	%esp			/* address for argv */
	pushl	%esp			/* address for argc */
	CALL	__setargs		/* set argc argv */
#endif
	CALL	_main			/* do the work */
	pushl	%eax			/* return value */
	CALL	_exit

	/* relative speed of machine */
	.data
	.align	2
	.globl	_cpuspeed
_cpuspeed:
	.long	8
	.globl  _cpu_init
	.align  2
_cpu_init:
	movl    $0x0,%eax
	movl    %eax,%cr0
	ret
