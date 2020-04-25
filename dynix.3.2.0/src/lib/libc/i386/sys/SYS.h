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

/* $Header: SYS.h 2.9 87/06/22 $
 *
 * Macros for creation of system call interfaces.
 */

/* $Log:	SYS.h,v $
 */

#include <syscall.h>

# ifdef	ATTFLG
#undef	SYS_open 
#undef	SYS_creat
#undef	SYS_mknod
#undef	SYS_read
#undef	SYS_stat
#undef	SYS_fstat
#undef  SYS_chown
#undef  SYS_unlink
#undef  SYS_acct
#define	SYS_open SYSV_open
#define	SYS_creat SYSV_creat
#define	SYS_mknod SYSV_mknod
#define	SYS_read SYSV_read
#define	SYS_stat SYSV_stat
#define	SYS_fstat SYSV_fstat
#define SYS_chown SYSV_chown
#define SYS_unlink SYSV_unlink
#define SYS_acct SYSV_acct
# endif	ATTFLG

#define SPOFF	4
#define SPARG0	SPOFF+0(%esp)
#define SPARG1	SPOFF+4(%esp)
#define SPARG2	SPOFF+8(%esp)
#define SPARG3	SPOFF+12(%esp)
#define SPARG4	SPOFF+16(%esp)
#define SPARG5	SPOFF+20(%esp)
#define SPARG6	SPOFF+24(%esp)
#define SPARG7	SPOFF+28(%esp)

/*
 * BASE_SVC_INT must match 1st interrupt number used in kernel (see
 * machine/trap.h).
 */

#include <machine/trap.h>

#define	BASE_SVC_INT	T_SVC0

/*
 * In the following, parameter 'a' to a macro means "number of arguments
 * to syscall", 's' means "name of syscall", and 'p' means "name of
 * pseudo syscall".  UBAR definition is for System V emulation support.
 */

#ifdef	UBAR
# ifdef PROF
#define	ENTRY(s)	.text; .globl __/**/s; .align 2; __/**/s:; \
			pushl %ebp; movl %esp, %ebp; \
			.data; 1: .long 0; .text; leal 1b,%eax; call mcount; \
			leave
# else
#define	ENTRY(s)	.text; .globl __/**/s; .align 2; __/**/s:
# endif PROF
#else	not UBAR
# ifdef PROF
#define	ENTRY(s)	.text; .globl _/**/s; .align 2; _/**/s:; \
			pushl %ebp; movl %esp, %ebp; \
			.data; 1: .long 0; .text; leal 1b,%eax; call mcount; \
			leave
# else
#define	ENTRY(s)	.text; .globl _/**/s; .align 2; _/**/s:
# endif PROF
#endif	UBAR

#define	CERROR		.text; err:; call cerror; ret
#define	SYSCALL(a,s)	ENTRY(s); SVC/**/a(s); jc err
#define	PSEUDO(s,p)	ENTRY(s); SVC0(p)
#define	PSEUDO1(s,p)	ENTRY(s); SVC1(p)

#define	SVC0(s)		SVC(0,s)
#define	SVC1(s)		movl SPARG0, %ecx; SVC(1,s)
#define	SVC2(s)		movl SPARG0, %ecx; movl SPARG1, %edx; SVC(2,s)
#define	SVC3(s)		leal SPARG0, %ecx; SVC(3,s)
#define	SVC4(s)		leal SPARG0, %ecx; SVC(4,s)
#define	SVC5(s)		leal SPARG0, %ecx; SVC(5,s)
#define	SVC6(s)		leal SPARG0, %ecx; SVC(6,s)

#ifdef ATTFLG
#define	SYS_REL	SYS_BSD		/* same for now */
#else
#define SYS_REL	SYS_BSD
#endif

#define	SVC(n,s)    movl $[SYS_/**/s|[SYS_REL<<16]], %eax; int $BASE_SVC_INT+n
