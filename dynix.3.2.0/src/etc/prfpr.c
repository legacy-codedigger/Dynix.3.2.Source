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
char	rcsid[] = "$Header: prfpr.c 2.1 86/04/07 $";
#endif	lint

/*
 * prfpr.c
 *	kernel profiling: print log file.
 */

#include "stdio.h"
#include "time.h"
#include "errno.h"
#include "a.out.h"
#include "sys/types.h"
#include "sys/stat.h"

#define PRFMAX  2048		/* max number of text symbols */

struct	profile	{
	long	p_date;		/* time stamp of record */
	long	p_ctr[PRFMAX+2];	/* profiler counter values */
} p[2];

struct	taddr {
	long addr;
	char name[16];
} taddr[PRFMAX + 2];

int 	t[PRFMAX + 2];
	
int	prfmax;

char	*namelist = "/dynix";
char	*logfile;

long	sum, osum;

double	pc;
double	cutoff = 1e-2;
main(argc, argv)
	int argc;
	char **argv;
{
	register int ff, log, i;
	double	atof();
	struct taddr *tp;
	struct taddr *search();

	switch(argc) {
		case 4:
			namelist = argv[3];
		case 3:
			cutoff = atof(argv[2]) / 1e2;
		case 2:
			logfile = argv[1];
			break;
		default:
			printf("usage: prfpr file [ cutoff [ namelist ] ]\n");
			exit(1);
	}

	if ((log = open(logfile, 0)) < 0) {
		printf("cannot open data file\n");
		exit(1);
	}

	if (cutoff >= 1e0 || cutoff < 0e0) {
		printf("invalid cutoff percentage\n");
		exit(1);
	}

	if (read(log, (char *)&prfmax, sizeof prfmax) != sizeof prfmax || prfmax == 0) {
		printf("bad data file\n");
		exit(1);
	}

	if (read(log, (char *)t, prfmax * sizeof (int)) != prfmax * sizeof (int)) {
		printf("cannot read profile addresses\n");
		exit(1);
	}

	osum = sum = ff = 0;

	(void)read(log, (char *)&p[!ff], (prfmax + 2) * sizeof (int));

	for(i = 0; i <= prfmax; i++)
		osum += p[!ff].p_ctr[i];

	rdsymtab();

	for(;;) {
		sum = 0;
		if(read(log, (char *)&p[ff], (prfmax + 2) * sizeof (int)) !=
		    (prfmax + 2) * sizeof (int))
			exit(0);
		shtime(&p[!ff].p_date);
		shtime(&p[ff].p_date);
		printf("\n");
		for(i = 0; i <= prfmax; i++)
			sum += p[ff].p_ctr[i];
		if(sum == osum)
			printf("no samples\n\n");
		else for(i = 0; i <= prfmax; i++) {
			pc = (double) (p[ff].p_ctr[i] - p[!ff].p_ctr[i]) /
				(double) (sum - osum);
			if(pc > cutoff)
				if(i == prfmax)
					printf("user     %5.2f\n",
					 pc * 1e2);
				else {
					tp = search(t[i]);
					if(tp == 0)
						printf("unknown  %5.2f\n",
							pc * 1e2);
					else if(tp->name[0] == '_')
						printf("%-14.14s  %5.2f\n",
						 &tp->name[1], pc * 1e2);
					else
						printf("%-15.15 %5.2f\n",
						 tp->name, pc * 1e2);
				}
		}
		ff = !ff;
		osum = sum;
		printf("\n");
	}
}

struct taddr *
search(addr)
unsigned addr;
{
	register int i;

	for (i = 0; i <= prfmax; i++) {
		if (addr == taddr[i].addr)
			return(&taddr[i]);
	}
	return((struct taddr *) 0);

}

int
rdsymtab()
{

	struct exec exec;
	struct nlist nlist;
	register int i;
	register struct taddr *ip;
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
	stringtable = malloc( (unsigned)stringtblsize );
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
			(void)strcpy((char *)ip->name, (char *)offset);
			ip->addr = nlist.n_value;
	
			*ip++;
		}
	}

	(void)fclose(fp);
	return;
	
}

shtime(l)
register long *l;
{
	register  struct  tm  *t;
	struct  tm  *localtime();

	if(*l == (long) 0) {
		printf("initialization\n");
		return;
	}
	t = localtime(l);
	printf("%02.2d/%02.2d/%02.2d %02.2d:%02.2d\n", t->tm_mon + 1,
		t->tm_mday, t->tm_year, t->tm_hour, t->tm_min);
}
