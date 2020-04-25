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
static char rcsid[] = "$Header: uuencode.c 2.1 87/05/27 $";
#endif

#ifndef lint
static char sccsid[] = "@(#)uuencode.c	5.3 (Berkeley) 1/22/85";
#endif

/*
 * uuencode [input] output
 *
 * Encode a file so it can be mailed to a remote system.
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

/* ENC is the basic 1 character encoding function to make a char printing */
#define ENC(c) ((c) ? ((c) & 077) + ' ': '`')

main(argc, argv)
char **argv;
{
	FILE *in;
	struct stat sbuf;
	int mode;

	/* optional 1st argument */
	if (argc > 2) {
		if ((in = fopen(argv[1], "r")) == NULL) {
			perror(argv[1]);
			exit(1);
		}
		argv++; argc--;
	} else
		in = stdin;

	if (argc != 2) {
		printf("Usage: uuencode [infile] remotefile\n");
		exit(2);
	}

	/* figure out the input file mode */
	fstat(fileno(in), &sbuf);
	mode = sbuf.st_mode & 0777;
	printf("begin %o %s\n", mode, argv[1]);

	encode(in, stdout);

	printf("end\n");
	exit(0);
}

/*
 * copy from in to out, encoding as you go along.
 */
encode(in, out)
FILE *in;
FILE *out;
{
	char buf[80];
	int i, n;

	for (;;) {
		/* 1 (up to) 45 character line */
		n = fr(in, buf, 45);
		putc(ENC(n), out);

		for (i=0; i<n; i += 3)
			outdec(&buf[i], out);

		putc('\n', out);
		if (n <= 0)
			break;
	}
}

/*
 * output one group of 3 bytes, pointed at by p, on file f.
 */
outdec(p, f)
char *p;
FILE *f;
{
	int c1, c2, c3, c4;

	c1 = *p >> 2;
	c2 = (*p << 4) & 060 | (p[1] >> 4) & 017;
	c3 = (p[1] << 2) & 074 | (p[2] >> 6) & 03;
	c4 = p[2] & 077;
	putc(ENC(c1), f);
	putc(ENC(c2), f);
	putc(ENC(c3), f);
	putc(ENC(c4), f);
}

/* fr: like read but stdio */
int
fr(fd, buf, cnt)
FILE *fd;
char *buf;
int cnt;
{
	int c, i;

	for (i=0; i<cnt; i++) {
		c = getc(fd);
		if (c == EOF)
			return(i);
		buf[i] = c;
	}
	return (cnt);
}
