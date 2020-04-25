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

#ifndef	lint
static	char	rcsid[] = "$Header: nargs.c 1.2 86/09/17 $";
#endif

/*
 * nargs.c
 *	Implement C-callable get-number-arguments function.
 *
 * Returns # words (32-bit) of arguments caller was called with.
 *
 * This function is machine/compiler/etc dependent.
 */

/* $Log:	nargs.c,v $
 */

#define	ADDL_IMM8	0xc483
#define ADDL_IMM32	0xc481
#define POPL_ECX	0x59

nargs()
{
	register char	*pc;
	int	length;					/* lives under FP */

	/*
	 * Get callers return pc (this proc'c FP -> fp in callers return
	 * stack frame).
	 */

	pc = (char*)(*((int*)(*((&length)+1)) + 1));	/* callers return PC */

	/*
	 * Is either "popl %eax", "addl $imm,%esp", or caller had no arguments.
	 */

	if ((*pc & 0xff) == POPL_ECX)
		return(1);

	switch(*(unsigned short *)pc) {
	
	case ADDL_IMM8:
		return(*(pc+2)/4);

	case ADDL_IMM32:
		return((*(int *)(pc+2))/4);

	default:
		return(0);
	}
}
