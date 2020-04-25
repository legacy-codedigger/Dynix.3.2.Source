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
static char rcsid[] = "$Header: split.c 2.0 86/01/28 $";
#endif

#include <stdio.h>

unsigned count = 1000;
int	fnumber;
char	fname[100];
char	*ifil;
char	*ofil;
FILE	*is;
FILE	*os;

main(argc, argv)
char *argv[];
{
	register i, c, f;
	int iflg = 0;

	for(i=1; i<argc; i++)
		if(argv[i][0] == '-')
			switch(argv[i][1]) {
		
			case '\0':
				iflg = 1;
				continue;
		
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				count = atoi(argv[i]+1);
				continue;
			}
		else if(iflg)
			ofil = argv[i];
		else {
			ifil = argv[i];
			iflg = 2;
		}
	if(iflg != 2)
		is = stdin;
	else
		if((is=fopen(ifil,"r")) == NULL) {
			perror(ifil);
			exit(1);
		}
	if(ofil == 0)
		ofil = "x";

loop:
	f = 1;
	for(i=0; i<count; i++)
	do {
		c = getc(is);
		if(c == EOF) {
			if(f == 0)
				fclose(os);
			exit(0);
		}
		if(f) {
			for(f=0; ofil[f]; f++)
				fname[f] = ofil[f];
			fname[f++] = fnumber/26 + 'a';
			fname[f++] = fnumber%26 + 'a';
			fname[f] = '\0';
			fnumber++;
			if((os=fopen(fname,"w")) == NULL) {
				fprintf(stderr,"Cannot create output\n");
				exit(1);
			}
			f = 0;
		}
		putc(c, os);
	} while(c != '\n');
	fclose(os);
	goto loop;
}
