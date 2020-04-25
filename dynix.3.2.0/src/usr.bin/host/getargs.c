/* @(#)$Copyright:	$
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

/* @(#)$Header: getargs.c 1.3 84/12/18 $ */

#include <stdio.h>
#include <sgtty.h>
#include "host.h"

getargs(ac, av)
int ac;
char *av[];
{
	unsigned br;

	while (ac-- > 0) {
		switch (av[0][0]) {
		case '-':
			switch (av[0][1]) {
			case 'b':		/* set port baud rate */
				av++;
				ac--;
				sscanf(av[0], "%d", &br);
				switch (br) {
				case 300:   baud = B300;  break;
				case 1200:  baud = B1200; break;
				case 2400:  baud = B2400; break;
				case 9600:  baud = B9600; break;
				case 19200: baud = EXTA;  break;
				default:
					fprintf(stderr, "%s: %d baud not supported\n",
						myname, br);
					exit(1);
				}
				break;
			case 'e':		/* change escape char */
				escape = av[0][2];
				break;
			case 'f':
				av++;
				ac--;
				if ((runfile = fopen(av[0], "r")) == NULL) {
					fprintf(stderr, "%s: can't open %s\n",
						myname, av[0]);
					exit(1);
				}
				fileonly++;
				break;
			case 'm':
				mflag++;
				break;
			case 'n':
				nflag++;
				break;
			case 'r':
				process = 0;
				break;
			case 's':
				av++;
				ac--;
				if ((scriptfp = fopen(av[0], "a")) == NULL) {
					fprintf(stderr, "%s: can't open %s\n",							 myname, av[0]);
					exit(1);
				}
				break;
			default:
				usage();
			}
			break;
		default:
			portname = *av;
			break;
		}
		av++;
	}
}
