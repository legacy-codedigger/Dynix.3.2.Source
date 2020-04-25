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

#ifdef	RCS
static char rcsid[] = "$Header: setargs.c 1.2 90/08/15 $";
#endif

#include <sys/types.h>
#include <machine/cfg.h>


#define ARGS 10

/*
 * Stand-alone argc and argv setup pria to main being called.
 * Note this routine modifies its arguments.
 */

char *_argv[ARGS];

_setargs(argc, argv, envp)
int	argc;
char	**argv;
struct config_desc *envp;
{
	register char	*p;
	register int	i;
	register int	*argcp = &argc;
	register char	*e;

	argv = _argv;
	e = &envp->c_boot_name[BNAMESIZE];

	p = envp->c_boot_name;
	/*
	 * Setup argv array. limit to ARGS. (also limited by
	 * original input line).
	 */
	for(i = 0; i < ARGS;) {
		/*
		 * span nulls.
		 */
		while (*p == '\0') {
			if (++p > e) {
				*argcp = i;
				return;
			}
		}
		_argv[i] = p;
		i++;
		/*
		 * scan to end of string.
		 */
		while (*p++ != '\0') {
			if (p > e) {
				*argcp = i;
				return;
			}
		}
	}
	*argcp = 10;
}
