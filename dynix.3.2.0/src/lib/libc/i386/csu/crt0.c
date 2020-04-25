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

#ifndef lint
static char rcsid[] = "$Header: crt0.c 1.4 87/04/23 $";
#endif

/*
 *	C start up routine.
 *
 *	Stack Frame on return from kernel "iret" instruction
 *	which start the user program (note: stack grows down)
 *	 ---------
 *	| argc 	  |	<- user stack pointer
 *	| argv[0] |	<- where argv wants to point
 *	| ..      |
 *	| 0       |
 *	| env[0]  |	<- where envp wants to point
 *	| ..      |
 *	| 0       |
 *	 ---------
 *
 * $Log:	crt0.c,v $
 */

#ifdef	COPYRIGHT
# include "copyright"
#endif

#define NBPW	sizeof(int)
struct kframe {
	int	kargc;
	char	*kargv[1];
};

char **environ = (char **)0;

start()
{
	/* ALL REGISTER VARIABLES */
	register int bootflags;		/* %edi -- needed for init */
	register struct kframe *kfp;	/* %esi -- address of params */
	register char **argv;		/* %ebx -- argv pointer */
#ifdef MCRT0
	extern unsigned char etext;
#endif MCRT0
#ifdef	SYSTEM5
	int i, *ip;
#endif

#ifdef lint
	edi = kfp = 0;
#endif
	asm("   movl	%esi, %ecx ");	/* %ecx -- Systemid needed by init */
	asm("	movl	%esp, %esi ");	/* kfp = sp */	
#ifdef	SYSTEM5
	asm("	movl	%esp, %ebp ");		/* allocate two locals */
	asm("	subl	$8, %esp ");
#endif
	argv = (char **) &kfp->kargv[0];
	environ = (char **) ((unsigned)argv + ((kfp->kargc + 1) * NBPW));
	_ppinit(kfp->kargc, argv);	/* for parallel inits */
#ifdef MCRT0
	monstartup(42, &etext);		/* 42 == current pc via sed script */
#endif MCRT0
#ifdef	SYSTEM5
#define	SIGXFSZ	25		/* exceeded file size limit */
	signal(SIGXFSZ,1);	/* ignore */
	/* clear stack */
	ip = (int *)kfp;	/* get orig sp */
	ip--; ip--;		/* skip locals */
	for (i = 0; i < 24; i++)
		*--ip = 0;
	asm("	movl	$0, %ebp ");	/* for debuggers */
#endif
	exit(main(kfp->kargc, argv, environ));
	asm("	hlt");			/* shouldn't get here */
}

#ifdef MCRT0
exit(code)
	int code;
{
	monitor(0);
	_cleanup();
	_exit(code);
}
#endif MCRT0

#ifdef MCRT0
#	ifdef GPROF
#		include "gmon.c"
#	else PROF
#		include "mon.c"
#	endif GPROF/PROF
#endif MCRT0

#ifdef CRT0
/*
 * null mcount and moncontrol,
 * just in case some routine is compiled for profiling
 */
moncontrol(val)
	int val;
{
}

mcount()
{
}
#endif CRT0
