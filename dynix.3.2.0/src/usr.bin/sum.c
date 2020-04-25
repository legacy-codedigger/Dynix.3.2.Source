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
static char rcsid[] = "$Header: sum.c 2.0 86/01/28 $";
#endif

/*
 * Sum bytes in file mod 2^16
 */

#include <stdio.h>

main(argc,argv)
char **argv;
{
	register unsigned sum;
	register i, c;
	register FILE *f;
	register long nbytes;
	int errflg = 0;

	i = 1;
	do {
		if(i < argc) {
			if ((f = fopen(argv[i], "r")) == NULL) {
				fprintf(stderr, "sum: Can't open %s\n", argv[i]);
				errflg += 10;
				continue;
			}
		} else
			f = stdin;
		sum = 0;
		nbytes = 0;
		while ((c = getc(f)) != EOF) {
			nbytes++;
			if (sum&01)
				sum = (sum>>1) + 0x8000;
			else
				sum >>= 1;
			sum += c;
			sum &= 0xFFFF;
		}
		if (ferror(f)) {
			errflg++;
			fprintf(stderr, "sum: read error on %s\n", argc>1?argv[i]:"-");
		}
		printf("%05u%6ld", sum, (nbytes+BUFSIZ-1)/BUFSIZ);
		if(argc > 2)
			printf(" %s", argv[i]);
		printf("\n");
		fclose(f);
	} while(++i < argc);
	exit(errflg);
}
