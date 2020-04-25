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
static	char	rcsid[] = "$Header: kppr.c 1.1 86/10/07 $";
#endif

/* $Log:	kppr.c,v $
 */

#define DEBUG

/*
 *	kppr - print profiler log files
 *
 *	Usage: kppr [-a] [-c cutoff] file [namelist]
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sec/kp.h>
#include <stdio.h>
#include "kp_util.h"
#include "kp_sym.h"

#define binlo(i)	((unsigned)(i << binshift) + p_hdr.b_text)
#define binhi(i)	(binlo(i) + ((unsigned)(1 << binshift) - 1))
#define ADDRHI(j)	((unsigned)(j < glb_symbols-1 ? next_addr : p_hdr.e_text))

int objfd;
extern int glb_symbols;
extern struct sym **symtablepv;
unsigned next_addr;

char	*namelist = "/dynix";
char	*file;				/* for sym.c :-( */
char	*logfile;
char	*whoiam;

double	pc;
double	cutoff = 1e-2;
int	all_text = 0;
struct kp_hdr p_hdr;
int	rec_size;

main(argc, argv)
	int argc;
	char *argv[];
{
	struct  tm  *t;
	int log, c, err = 0;
	unsigned *rec;
	extern double	atof();
	extern char *malloc();
	extern char *optarg;
	extern int optind;
	extern struct  tm  *localtime();
	double secs;

	whoiam = argv[0];
	while ((c = getopt(argc, argv, "ac:p")) != EOF) {
	    switch (c) {

	    case 'a':
		all_text++;
		break;

	    case 'c':
		cutoff = atof(optarg) / 1e2;
		if (cutoff >= 1e0 || cutoff < 0e0) {
			fprintf(stderr, "%s: Bad cutoff [%s]\n",
				argv[0], optarg);
			err++;
		}
		break;

	    case '?':
		err++;
		break;
	    }
	}
	if (err || optind >= argc) {
		fprintf(stderr,
			"Usage: %s [-a] [-c cutoff] file [namelist]\n",
			argv[0]);
		exit(1);
	}

	logfile = argv[optind++];
	if (optind < argc)
		namelist = argv[optind];
	file = namelist;

	if ((log = open(logfile, 0)) < 0)
		error("cannot open data file");

	if ((objfd = open(namelist, 0)) < 0) {
		fprintf(stderr, "Can't open %s\n", namelist);
		exit(1);
	}

	getsymboltable(objfd);

	if (read(log, (char *)&p_hdr, sizeof (p_hdr)) != sizeof(p_hdr))
		error("Can't read profiling header");

	if (p_hdr.engines <= 0 || p_hdr.bins <= 0)
		error("Profiling header invalid");

	printf("engines = %d  bins = %d binshift = %d\n",
		p_hdr.engines, p_hdr.bins, p_hdr.binshift);
	printf("b_text = 0x%x  e_text = 0x%x\n", p_hdr.b_text, p_hdr.e_text);
	rec_size = p_hdr.bins * sizeof(unsigned);
	rec = (unsigned *) malloc((unsigned)rec_size);

	if (read(log, (char *)rec, rec_size) != rec_size)
		error("Error on read of record");

	if (p_hdr.tod_flag == TIMESTAMP) {
		t = localtime((time_t *)&p_hdr.tod.tv_sec);
		printf("%02.2d/%02.2d/%02.2d %02.2d:%02.2d:%02.2d\n",
			t->tm_mon + 1, t->tm_mday, t->tm_year, t->tm_hour,
			t->tm_min, t->tm_sec);
	} else {
		secs = p_hdr.tod.tv_sec + p_hdr.tod.tv_usec / 1000000.;
		printf("\n\nElapsed time: %8.2f\n\n", secs);
	}
	putchar('\n');

	pr_level(rec);
}


pr_level(recp)
	unsigned *recp;
{
	register int i, j;
	register unsigned *dp;
	register int binshift = p_hdr.binshift;
	unsigned symsum, lo, addition, samples = 0;

	printf("\nKernel profile data\n");

	dp = recp;
	for (i = 0; i < p_hdr.bins; i++) {
		samples += *dp++;
	}
#ifdef DEBUG
	    printf("samples = %u\n", samples);
#endif
	if (samples != 0) {
		dp = recp;
		i = 0;
		for (j = 0; j < glb_symbols; j++) {
		    symsum = 0;
		    lo = symtablepv[j]->sym_value;
		    next_addr = symtablepv[j+1]->sym_value;
		    while ((i < p_hdr.bins-1) && (binhi(i) < ADDRHI(j))) {
			addition = (dp[i]*(binhi(i)-lo+1)) >> binshift;
			symsum += addition;
			i++;
			lo = binlo(i);
		    }
		    addition = (dp[i] * (ADDRHI(j) - lo)) >> binshift;
		    symsum += addition;

		    pc = (double) symsum / (double) samples;
		    if (pc < cutoff)
			continue;
		    pr_kern_sym(j, pc);
		}
		pc = (double)dp[p_hdr.bins-1] / (double) samples;
		if (pc >= cutoff)
		    printf("user                  %5.2f\n\n", pc * 1e2);
	    } else {
		printf("no samples\n");
	    }
}


pr_kern_sym(symi, percent)
	int symi;
	double percent;
{
	register char *name = symtablepv[symi]->sym_name;

	if (*name == '_')
		printf("%-20.20s  %5.2f\n", name+1, percent * 1e2);
	else
		printf("%-21.21s %5.2f\n", name, percent * 1e2);
}

error(s)
	char *s;
{
	printf("error: %s\n", s);
	exit(1);
}
