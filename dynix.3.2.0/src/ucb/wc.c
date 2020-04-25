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
static char rcsid[] = "$Header: wc.c 2.1 90/02/23 $";
#endif

/* wc line and word count */

#include <stdio.h>
long	linect, wordct, charct, pagect;
long	tlinect, twordct, tcharct, tpagect;
char	*wd = "lwc";

main(argc, argv)
char **argv;
{
	int i, token;
	register FILE *fp;
	register int c;
	char *p;

	while (argc > 1 && *argv[1] == '-') {
		switch (argv[1][1]) {
		case 'l': case 'w': case 'c': 
			wd = argv[1]+1;
			break;
		default:
		usage:
			fprintf(stderr, "Usage: wc [-lwc] [files]\n");
			exit(1);
		}
		argc--;
		argv++;
	}

	i = 1;
	fp = stdin;
	do {
		if(argc>1 && (fp=fopen(argv[i], "r")) == NULL) {
			perror(argv[i]);
			continue;
		}
		linect = 0;
		wordct = 0;
		charct = 0;
		token = 0;
		for(;;) {
			c = getc(fp);
			if (c == EOF)
				break;
			charct++;
			if(' '<c&&c<0177) {
				if(!token) {
					wordct++;
					token++;
				}
				continue;
			}
			if(c=='\n') {
				linect++;
			}
			else if(c!=' '&&c!='\t')
				continue;
			token = 0;
		}
		if (!feof(fp))
			perror(argv[i]);
		/* print lines, words, chars */
		wcp(wd, charct, wordct, linect);
		if(argc>1) {
			printf(" %s\n", argv[i]);
		} else
			printf("\n");
		fclose(fp);
		tlinect += linect;
		twordct += wordct;
		tcharct += charct;
	} while(++i<argc);
	if(argc > 2) {
		wcp(wd, tcharct, twordct, tlinect);
		printf(" total\n");
	}
	exit(0);
}

wcp(wd, charct, wordct, linect)
register char *wd;
long charct; long wordct; long linect;
{
	while (*wd) switch (*wd++) {
	case 'l':
		ipr(linect);
		break;

	case 'w':
		ipr(wordct);
		break;

	case 'c':
		ipr(charct);
		break;

	}
}

ipr(num)
long num;
{
	printf(" %7ld", num);
}

