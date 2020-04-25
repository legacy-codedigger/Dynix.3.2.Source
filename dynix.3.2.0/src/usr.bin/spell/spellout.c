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
static char rcsid[] = "$Header: spellout.c 2.0 86/01/28 $";
#endif

#include "spell.h"

main(argc, argv)
char **argv;
{
	register i, j;
	long h;
	register long *lp;
	char word[NW];
	int dflag = 0;
	int indict;
	register char *wp;

	if (argc>1 && argv[1][0]=='-' && argv[1][1]=='d') {
		dflag = 1;
		argc--;
		argv++;
	}
	if(argc<=1) {
		fprintf(stderr,"spellout: arg count\n");
		exit(1);
	}
	if(!prime(argc,argv)) {
		fprintf(stderr,
		    "spellout: cannot initialize hash table\n");
		exit(1);
	}
	while (fgets(word, sizeof(word), stdin)) {
		indict = 1;
		for (i=0; i<NP; i++) {
			for (wp = word, h = 0, lp = pow2[i];
				(j = *wp) != '\0'; ++wp, ++lp)
				h += j * *lp;
			h %= p[i];
			if (get(h)==0) {
				indict = 0;
				break;
			}
		}
		if (dflag == indict)
			fputs(word, stdout);
	}
}
