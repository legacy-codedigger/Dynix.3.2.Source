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
static char rcsid[] = "$Header: main.c 1.1 89/07/31 $";
#endif

/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1988 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)main.c	1.6 (Berkeley) 5/15/88";
#endif /* not lint */

#include <sys/types.h>

#include "ring.h"

#include "externs.h"
#include "defines.h"

/*
 * Initialize variables.
 */

void
tninit()
{
    init_terminal();

    init_network();
    
    init_telnet();

    init_sys();

    init_3270();
}


/*
 * main.  Parse arguments, invoke the protocol or command parser.
 */


void
main(argc, argv)
	int argc;
	char *argv[];
{
    tninit();		/* Clear out things */

    TerminalSaveState();

    prompt = argv[0];
    while ((argc > 1) && (argv[1][0] == '-')) {
	if (!strcmp(argv[1], "-d")) {
	    debug = 1;
	} else if (!strcmp(argv[1], "-n")) {
	    if ((argc > 1) && (argv[2][0] != '-')) {	/* get file name */
		NetTrace = fopen(argv[2], "w");
		argv++;
		argc--;
		if (NetTrace == NULL) {
		    NetTrace = stdout;
		}
	    }
	} else {
#if	defined(TN3270) && defined(unix)
	    if (!strcmp(argv[1], "-t")) {
		if ((argc > 1) && (argv[2][0] != '-')) { /* get file name */
		    transcom = tline;
		    (void) strcpy(transcom, argv[1]);
		    argv++;
		    argc--;
		}
	    } else if (!strcmp(argv[1], "-noasynch")) {
		noasynch = 1;
	    } else
#endif	/* defined(TN3270) && defined(unix) */
	    if (argv[1][1] != '\0') {
		fprintf(stderr, "Unknown option *%s*.\n", argv[1]);
	    }
	}
	argc--;
	argv++;
    }
    if (argc != 1) {
	if (setjmp(toplevel) != 0)
	    Exit(0);
	tn(argc, argv);
    }
    setjmp(toplevel);
    for (;;) {
#if	!defined(TN3270)
	command(1);
#else	/* !defined(TN3270) */
	if (!shell_active) {
	    command(1);
	} else {
#if	defined(TN3270)
	    shell_continue();
#endif	/* defined(TN3270) */
	}
#endif	/* !defined(TN3270) */
    }
}
