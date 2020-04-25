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
char	rcsid[] = "$Header: prfld.c 2.1 86/04/07 $";
#endif	lint

/*
 * prfld.c
 *	kernel profiling: load profiler symbol-table.
 */

#include "stdio.h"
#include "errno.h"
#include "a.out.h"
#include "sys/types.h"
#include "sys/stat.h"

#define PRFMAX	2048		/* maximum number of symbols */

char *namelist = "/dynix";	/* namelist file */
extern	int	errno;

int taddr[PRFMAX];
int ntaddr;

unsigned long prfsymp;
unsigned long prfmaxp;

main(argc, argv)
int argc;
char **argv;
{
	register int prf;
	int	compar();
	struct nlist nl[3];

	if(argc == 2)
		namelist = argv[1];
	else if(argc != 1) {
		printf("usage: prfld [/dynix]\n");
		exit(1);
	}

	if((prf = open("/dev/mem", 1)) < 0) {
		printf("Cannot open /dev/mem\n");
		exit(1);
	}

	nl[0].n_un.n_name = "_prfsym";
	nl[1].n_un.n_name = "_prfmax";
	bzero((char *)&nl[2], sizeof(struct nlist));
	nlist(namelist, nl);

	prfsymp = nl[0].n_value;
	prfmaxp = nl[1].n_value;

	if (prfmaxp == 0) {
		printf("Profiler not installed\n");
		exit(1);
	}

	ntaddr = rdsymtab();

	qsort((char *)taddr, ntaddr, sizeof (int), compar);

	(void)lseek(prf, (long)prfmaxp, 0);
	(void)write(prf, (char *)&ntaddr, sizeof ntaddr);

	(void)lseek(prf, (long)prfsymp, 0);

	if(write(prf, (char *)taddr, ntaddr*sizeof(int)) != ntaddr*sizeof(int))
		switch(errno) {
		case ENOSPC:
			printf("insufficient space in system for addresses\n");
			exit(1);
		case E2BIG:
			printf("unaligned data or insufficient addresses\n");
			exit(1);
		case EBUSY:
			printf("profiler is enabled\n");
			exit(1);
		case EINVAL:
			printf("text addresses not sorted properly\n");
			exit(1);
		default:
			printf("cannot load profiler addresses\n");
			exit(1);
		}
	(void)close(prf);
}

compar(x, y)
	register  unsigned  *x, *y;
{
	if(*x > *y)
		return(1);
	else if(*x == *y)
		return(0);
	return(-1);
}

int
rdsymtab()
{

	struct exec exec;
	struct nlist nlist;
	register int i;
	register int *ip;
	FILE	*fp;
	int stringtblsize;
	char *stringtable;
	char *malloc();
	extern errno;
	unsigned long offset;
	struct stat sbuf;
	int fd;
	int len;

	if ((fd = open(namelist, 0)) == NULL) {
		printf("cannot open namelist\n");
		exit(1);
	}

	if ((fstat(fd, &sbuf)) == -1) {
		printf("cannot stat namelist\n");
		exit(1);
	}

	(void)close(fd);
	
	fp = fopen(namelist, "r");

	if (fread((char *)&exec, sizeof exec, 1, fp) != 1) {
		printf("read error in namelist file\n");
		exit(1);
	}

	if (N_BADMAG(exec)) {
		printf("bad magic\n");
		exit(1);
	}

	stringtblsize = sbuf.st_size - N_STROFF(exec);
	stringtable = malloc ( (unsigned)stringtblsize );
	if (stringtable == NULL) {
		printf("malloc of string table failed\n");
		exit(1);
	}

	(void)fseek(fp, (long)N_STROFF(exec), 0);
	(void)fread(stringtable, sizeof(char), stringtblsize, fp);

	ip = taddr;

	(void)fseek(fp, (long)N_SYMOFF(exec), 0);

	for (i = 0; i < (exec.a_syms/sizeof nlist); i++) {
		if (ip == &taddr[PRFMAX]) {
			printf("too many test symbols\n");
			exit(1);
		}

		if (fread((char *)&nlist, sizeof nlist, 1, fp) != 1) {
			printf("read error of nlist\n");
			exit(1);
		}
	
		if ((nlist.n_type & N_TYPE) == N_TEXT) {
			offset = nlist.n_un.n_strx + (unsigned long)stringtable;
			len = strlen((char *)offset);
			if (*(char *)offset == 'L')
				continue;
			if (((char *)offset)[len-2] == '.'
					&& ((char *)offset)[len-1] == 'o')
				continue;
			*ip++ = nlist.n_value;
		}
	}

	(void)fclose(fp);
	return(ip - taddr);
	
}

