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
static	char	rcsid[] = "$Header: kpstat.c 1.1 86/10/07 $";
#endif

/* $Log:	kpstat.c,v $
 */

/*
 *	kpstat - change and/or report profiler status
 *
 *	Usage:	kpstat [-p on|off] [-i interval] [-n] [-r] [-D on|off]
 */
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sec/kp.h>
#include <stdio.h>

main(argc, argv)
int argc;
char	*argv[];
{
	int  kp, c, err = 0;
	int	onoff = 0,		/* > 0 => enable it; < 0 => disable */
		interval = 0,
		reset_cntrs = 0,
		debug = 0;
	struct kp_status status;
	extern char *optarg;

	while ((c = getopt(argc, argv, "i:p:rD:")) != EOF) {
	    switch (c) {
	    case 'i':
		interval = atoi(optarg);
		if (interval <= 0) {
		    fprintf(stderr, "Invalid interval: %d\n", interval);
		    err++;
		}
		break;
	    case 'p':
		if (!strcmp(optarg, "on"))
			onoff++;
		else if (!strcmp(optarg, "off"))
			onoff--;
		else {
		    err++;
		    fprintf(stderr, "-p: bad arg [%s]\n", optarg);
		}
		break;
	    case 'r':
		reset_cntrs++;
		break;
	    case 'D':
		if (!strcmp(optarg, "on"))
			debug++;
		else if (!strcmp(optarg, "off"))
			debug--;
		else {
		    err++;
		    fprintf(stderr, "-D: bad arg [%s]\n", optarg);
		}
		break;
	    case '?':
		err++;
		break;
	    }
	}
	if (err) {
	    fprintf(stderr, 
 	"Usage:	kpstat [-p on|off] [-i interval] [-r] [-D on|off]\n", argv[0]);
		exit(1);
	}

	if ((kp = open("/dev/kp", 2)) < 0) {
		fprintf(stderr, "%s: ", argv[0]);
		perror("can't open /dev/kp\n");
		exit(1);
	}
	if (ioctl(kp, KP_GETSTATE, (char *) &status) < 0) {
		fprintf(stderr, "%s: GETSTATE failed ", argv[0]);
		perror("");
		exit(1);
	}
	if (argc > 1) {		/* Want to change something */
		if (onoff < 0)
			status.kps_state &= ~KP_ENABLED;
		else if (onoff > 0)
			status.kps_state |= KP_ENABLED;
		if (interval)
			set_interval(&status, interval);
		if (debug > 0)
			status.kps_state |= KP_DEBUG;
		else if (debug < 0)
			status.kps_state &= ~KP_DEBUG;
		if (ioctl(kp, KP_SETSTATE, (char *) &status) < 0) {
			fprintf(stderr, "%s: SETSTATE failed ", argv[0]);
			perror("");
			exit(1);
		}
		if (ioctl(kp, KP_GETSTATE, (char *) &status) < 0) {
			fprintf(stderr, "%s: GETSTATE after SET failed ",
				argv[0]);
			perror("");
			exit(1);
		}
	}

	if (reset_cntrs) {
		if (ioctl(kp, KP_RESET, (char *) 0) < 0) {
			fprintf(stderr, "%s: Reset counters failed> ",
				argv[0]);
			perror("");
			exit(1);
		} else
			printf("Profiling counters reset.\n\n");
	}
	printf("Profiling %s.\n",
			status.kps_state & KP_ENABLED ? "enabled" : "disabled");
	printf("SCED interrupt is every %d milliseconds\n", status.kps_interval);
	if (status.kps_reload == 0)
		printf("NMI sent every SCED interrupt.\n");
	else
		printf("NMI sent every %d SCED interrupts\n",
			status.kps_reload);
	printf("processors: %d  bins: %d  bin shift: %d\n",
		status.kps_engines, status.kps_bins, status.kps_binshift);
	printf("SCED sent %d nmi's\n", status.kps_sced_nmis);
#ifdef	NOTYET
	printf("Processor\t");
	for (i = 0; i < status.engines; i++)
		printf("\t%d", i);
	putchar('\n');
	printf("Nmi's recvd\t");
	for (i = 0; i < status.engines; i++)
		printf("\t%d", status.p_nmi_cnt[i]);
	putchar('\n');
#endif
	if (status.kps_state & KP_BINERROR)
		printf("SCED detected a bin out of range error!\n");
	printf("debug is %s\n", status.kps_state & KP_DEBUG ? "On" : "Off");
}

set_interval(sp, interval)
struct kp_status *sp;
int interval;
{
	if (interval <= 10) {
		sp->kps_interval = interval;
		sp->kps_reload = 0;
	} else {
		calc_tm(sp, interval);
	}
}

/*
 * The following has borrowed and modified code from factor.c
 */

int nf;
int fa[20];
int ind[20];

calc_tm(sp, interval)
struct kp_status *sp;
int interval;
{

	get_factors(interval);
	sp->kps_interval = findintv();
	sp->kps_reload = interval / sp->kps_interval;
}

int
findintv()
{
	register int c, i, j, intv;
	int mi;
	int max_intv = -1;

	for (c = 1; c <= nf; c++) {
		for (i = 0; i < c; i++)
			ind[i] = i;
		while (1) {
			intv = fa[ ind[0] ];
			for (i = 1; i < c; i++)
				intv *= fa[ ind[i] ];
			if (intv <= 10 && intv > max_intv)
				max_intv = intv;
			i = c - 1;
			mi = nf -1;
			while (i >= 0 && ind[i] >= mi) {
				i--;
				mi--;
			}
			if (i < 0)
				break;
			ind[i]++;
			for (j = i + 1; j < c; j++)
				ind[j] = ind[j-1] + 1;
		}
	}
	return(max_intv);
}

/*
 * Print all prime factors of integer n > 0, smallest first, one to a line
 */
get_factors(n)
	register int	n;
{
	register int	prime;

	if (n == 1) {
		fa[nf++] = 1;
/*
		printf("\t1\n");
 */
	} else {
		while (n != 1) {
			prime = factor(n);
			fa[nf++] = prime;
/*
			printf("\t%d\n", prime);
 */
			n /= prime;
		}
	}
}

/*
 * Return smallest prime factor of integer N > 0
 *
 * Algorithm from E.W. Dijkstra (A Discipline of Programming, Chapter 20)
 */

int
factor(N)
	int	N;
{
	int		p;
	register int	f;
	static struct {
		int	hib;
		int	val[24];
	} ar;

	{	register int	x, y;

		ar.hib = -1;
		x = N; y = 2;
		while (x != 0) {
			ar.val[++ar.hib] = x % y;
			x /= y;
			y += 1;
		}
	}

	f = 2;

	while (ar.val[0] != 0 && ar.hib > 1) {
		register int	i;

		f += 1;
		i = 0;
		while (i != ar.hib) {
			register int	j;

			j = i + 1;
			ar.val[i] -= j * ar.val[j];
			while (ar.val[i] < 0) {
				ar.val[i] += f + i;
				ar.val[j] -= 1;
			}
			i = j;
		}
		while (ar.val[ar.hib] == 0)
			ar.hib--;
	}

	if (ar.val[0] == 0)
		p = f;
	else
		p = N;

	return(p);
}
