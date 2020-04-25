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
static char rcsid[] = "$Header: cat.c 2.0 86/01/28 $";
#endif

/*
 * Concatenate files.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

char	stdbuf[BUFSIZ];
char	buf[8*BUFSIZ];
int	bflg, eflg, nflg, sflg, tflg, vflg, uflg;
int	spaced, col, lno, inline;

main(argc, argv)
char **argv;
{
	int fflg = 0;
	register FILE *fi;
	register c;
	int dev, ino = -1;
	struct stat statb;

	lno = 1;
	if (isatty(fileno(stdout)))	/* defeat line buffering */
		setbuf(stdout, stdbuf);
	for( ; argc>1 && argv[1][0]=='-'; argc--,argv++) {
		switch(argv[1][1]) {
		case 0:
			break;
		case 'u':
			setbuf(stdout, (char *)NULL);
			uflg++;
			continue;
		case 'n':
			nflg++;
			continue;
		case 'b':
			bflg++;
			nflg++;
			continue;
		case 'v':
			vflg++;
			continue;
		case 's':
			sflg++;
			continue;
		case 'e':
			eflg++;
			vflg++;
			continue;
		case 't':
			tflg++;
			vflg++;
			continue;
		}
		break;
	}
	if (fstat(fileno(stdout), &statb) == 0) {
		statb.st_mode &= S_IFMT;
		if (statb.st_mode!=S_IFCHR && statb.st_mode!=S_IFBLK) {
			dev = statb.st_dev;
			ino = statb.st_ino;
		}
	}
	if (argc < 2) {
		argc = 2;
		fflg++;
	}
	while (--argc > 0) {
		if (fflg || (*++argv)[0]=='-' && (*argv)[1]=='\0')
			fi = stdin;
		else {
			if ((fi = fopen(*argv, "r")) == NULL) {
				perror(*argv);
				continue;
			}
		}
		if (fstat(fileno(fi), &statb) == 0) {
			if ((statb.st_mode & S_IFMT) == S_IFREG &&
			    statb.st_dev==dev && statb.st_ino==ino) {
				fprintf(stderr, "cat: input %s is output\n",
				   fflg?"-": *argv);
				fclose(fi);
				continue;
			}
		}
		if (nflg||sflg||vflg)
			copyopt(fi);
		else {
			if (uflg) /* for 100% backward compatibility */
				while ((c = getc(fi)) != EOF)
					putchar(c);
			else
				while ((c = fread(buf, 1, sizeof buf, fi)) != 0)
					fwrite(buf, 1, c, stdout);
		}
		if (fi!=stdin)
			fclose(fi);
	}
	if (ferror(stdout))
		fprintf(stderr, "cat: output write error\n");
	return(0);
}

copyopt(f)
	register FILE *f;
{
	register int c;

top:
	c = getc(f);
	if (c == EOF)
		return;
	if (c == '\n') {
		if (inline == 0) {
			if (sflg && spaced)
				goto top;
			spaced = 1;
		}
		if (nflg && bflg==0 && inline == 0)
			printf("%6d\t", lno++);
		if (eflg)
			putchar('$');
		putchar('\n');
		inline = 0;
		goto top;
	}
	if (nflg && inline == 0)
		printf("%6d\t", lno++);
	inline = 1;
	if (vflg) {
		if (tflg==0 && c == '\t')
			putchar(c);
		else {
			if (c > 0177) {
				printf("M-");
				c &= 0177;
			}
			if (c < ' ')
				printf("^%c", c+'@');
			else if (c == 0177)
				printf("^?");
			else
				putchar(c);
		}
	} else
		putchar(c);
	spaced = 0;
	goto top;
}
