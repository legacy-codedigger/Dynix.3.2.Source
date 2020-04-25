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
static	char	rcsid[] = "$Header: conf_syscalls.c 1.2 87/06/04 $";
#endif

/*
 * conf_syscalls.c
 * 	System-call entry configuration table.
 *
 * Primarily used to configure upwards-compatible system call interfaces.
 */

/* $Log:	conf_syscalls.c,v $
 */

/*
 * No v2.1 upwards compatibility necessary for i386 systems.
 */

extern	nosyscall();
extern	syscall();

int	(*syscall_handler[])() = {
	nosyscall,			/* [0] v2.1 system call handler */
	syscall,			/* [1] v3.0 system call handler */
};

int	syscall_nhandler = sizeof(syscall_handler) / sizeof(syscall_handler[0]);
