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
static char rcsid[] = "$Header: fold.c 2.0 86/01/28 $";
#endif

#include <stdio.h>
/*
 * fold - fold long lines for finite output devices
 *
 * Bill Joy UCB June 28, 1977
 */

int	fold =  80;

main(argc, argv)
	int argc;
	char *argv[];
{
	register c;
	FILE *f;
	char obuf[BUFSIZ];

	argc--, argv++;
	setbuf(stdout, obuf);
	if (argc > 0 && argv[0][0] == '-') {
		fold = 0;
		argv[0]++;
		while (*argv[0] >= '0' && *argv[0] <= '9')
			fold *= 10, fold += *argv[0]++ - '0';
		if (*argv[0]) {
			printf("Bad number for fold\n");
			exit(1);
		}
		argc--, argv++;
	}
	do {
		if (argc > 0) {
			if (freopen(argv[0], "r", stdin) == NULL) {
				perror(argv[0]);
				exit(1);
			}
			argc--, argv++;
		}
		for (;;) {
			c = getc(stdin);
			if (c == -1)
				break;
			putch(c);
		}
	} while (argc > 0);
	exit(0);
}

int	col;

putch(c)
	register c;
{
	register ncol;

	switch (c) {
		case '\n':
			ncol = 0;
			break;
		case '\t':
			ncol = (col + 8) &~ 7;
			break;
		case '\b':
			ncol = col ? col - 1 : 0;
			break;
		case '\r':
			ncol = 0;
			break;
		default:
			ncol = col + 1;
	}
	if (ncol > fold)
		putchar('\n'), col = 0;
	putchar(c);
	switch (c) {
		case '\n':
			col = 0;
			break;
		case '\t':
			col += 8;
			col &= ~7;
			break;
		case '\b':
			if (col)
				col--;
			break;
		case '\r':
			col = 0;
			break;
		default:
			col++;
			break;
	}
}
