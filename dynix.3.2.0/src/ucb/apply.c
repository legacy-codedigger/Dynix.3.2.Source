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
static char rcsid[] = "$Header: apply.c 2.2 86/05/29 $";
#endif

/*%cc -s -O %
 * apply - apply a command to a set of arguments
 *
 *	apply echo * == ls
 *	apply -2 cmp A1 B1 A2 B2   compares A's with B's
 *	apply "ln %1 /usr/fred/dir" *  duplicates a directory
 */
#include <stdio.h>
char	*cmdp;
#define	NCHARS 512
char	cmd[512];
char	defargs=1;
#define	DEFARGCHAR	'%'
char	argchar=DEFARGCHAR;
int	nchars;
extern	char *getenv();
int	ndesired = 1;
int	verbose = 0;

main(argc, argv)
	char *argv[];
{
	int nrunning;
	register n;
	int pflag = 0;
	while(argc>2 && argv[1][0]=='-'){
		if(argv[1][1]=='a'){
			argchar=argv[1][2];
			if(argchar=='\0')
				argchar=DEFARGCHAR;
		} else if (argv[1][1]=='P'){
			ndesired = atoi(&argv[1][2]);
			pflag++;
		} else if (argv[1][1]=='v'){
			verbose = 1;
		} else {
			defargs = atoi(&argv[1][1]);
			if(defargs < 0)
				defargs = 1;
		}
		--argc; ++argv;
	}
	if(argc<2){
		fprintf(stderr, "usage: apply [-v] [-Pn] [-a%] [-n] cmd arglist\n");
		exit(1);
	}
	argc -= 2;
	cmdp = argv[1];
	argv += 2;
	if (pflag == 0) {
		char *p = getenv("PARALLEL");
		if ( p )
			ndesired = atoi(p);
	}
	if (ndesired < 1)
		ndesired = 1;
	nrunning = 0;
	do {
		while(nrunning < ndesired && (n = docmd(argc, argv))) {
			nrunning++;
			argc -= n;
			argv += n;
		} 
		(void) wait(0);
	} while (nrunning--);
	exit(0);
}
char
addc(c)
	char c;
{
	if(nchars++>=NCHARS){
		fprintf(stderr, "apply: command too long\n");
		exit(1);
	}
	return(c);
}
char *
addarg(s, t)
	register char *s, *t;
{
	while(*t = addc(*s++))
		*t++;
	return(t);
}
docmd(argc, argv)
	char *argv[];
{
	register char *p, *q;
	register max, i;
	char gotit;
	if(argc<=0)
		return(0);
	nchars = 0;
	max = 0;
	gotit = 0;
	p = cmdp;
	q = cmd;
	while(*q = addc(*p++)){
		if(*q++!=argchar || *p<'1' || '9'<*p)
			continue;
		if((i= *p++-'1') > max)
			max = i;
		if(i>=argc){
	Toofew:
			fprintf(stderr, "apply: expecting argument(s) after `%s'\n", argv[argc-1]);
			exit(1);
		}
		q = addarg(argv[i], q-1);
		gotit++;
	}
	if(defargs!=0 && gotit==0){
		if(defargs>argc)
			goto Toofew;
		for(i=0; i<defargs; i++){
			*q++ = addc(' ');
			q = addarg(argv[i], q);
		}
	}
	i = system(cmd);
	return(max==0? (defargs==0? 1 : defargs) : max+1);
}
system(s)
char *s;
{
	int pid;
	char *shell = getenv("SHELL");

	if (verbose)
		fprintf(stderr, "apply: (%s) sh -c \"%s\"\n", shell ? shell : "/bin/sh", s);
	if ((pid = fork()) == 0) {
		execl(shell ? shell : "/bin/sh", "sh", "-c", s, 0);
		_exit(127);
	}
	if(pid == -1){
		fprintf(stderr, "apply: can't fork\n");
		exit(1);
	}
	return(0);
}
