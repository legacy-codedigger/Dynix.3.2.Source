/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
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
static char *rcsid = "$Header: sh.time.c 2.4 1991/07/26 01:13:35 $";
#endif

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley Software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * static char *sccsid = "@(#)sh.time.c	5.6 (Berkeley) 5/19/88";
 */

#include "sh.h"

/*
 * C Shell - routines handling process timing and niceing
 */

settimes()
{
	struct rusage ruch;

	(void) gettimeofday(&time0, (struct timezone *)0);
	(void) getrusage(RUSAGE_SELF, &ru0);
	(void) getrusage(RUSAGE_CHILDREN, &ruch);
	ruadd(&ru0, &ruch);
}

/*
 * dotime is only called if it is truly a builtin function and not a
 * prefix to another command
 */
dotime()
{
	struct timeval timedol;
	struct rusage ru1, ruch;

	(void) getrusage(RUSAGE_SELF, &ru1);
	(void) getrusage(RUSAGE_CHILDREN, &ruch);
	ruadd(&ru1, &ruch);
	(void) gettimeofday(&timedol, (struct timezone *)0);
	prusage(&ru0, &ru1, &timedol, &time0);
}

/*
 * donice is only called when it on the line by itself or with a +- value
 */
donice(v)
	register char **v;
{
	register char *cp;
	int nval;

	v++, cp = *v++;
	if (cp == 0)
		nval = 4;
	else if (*v == 0 && any(cp[0], "+-"))
		nval = getn(cp);
	(void) setpriority(PRIO_PROCESS, 0, nval);
}

ruadd(ru, ru2)
	register struct rusage *ru, *ru2;
{
	register long *lp, *lp2;
	register int cnt;

	tvadd(&ru->ru_utime, &ru2->ru_utime);
	tvadd(&ru->ru_stime, &ru2->ru_stime);
	if (ru2->ru_maxrss > ru->ru_maxrss)
		ru->ru_maxrss = ru2->ru_maxrss;
	cnt = &ru->ru_last - &ru->ru_first + 1;
	lp = &ru->ru_first; lp2 = &ru2->ru_first;
	do
		*lp++ += *lp2++;
	while (--cnt > 0);
}

prusage(r0, r1, e, b)
	register struct rusage *r0, *r1;
	struct timeval *e, *b;
{
	register time_t t =
	    (r1->ru_utime.tv_sec-r0->ru_utime.tv_sec)*100+
	    (r1->ru_utime.tv_usec-r0->ru_utime.tv_usec)/10000+
	    (r1->ru_stime.tv_sec-r0->ru_stime.tv_sec)*100+
	    (r1->ru_stime.tv_usec-r0->ru_stime.tv_usec)/10000;
	register char *cp;
	register long i;
	register struct varent *vp = adrof("time");
	long ms =
	    (e->tv_sec-b->tv_sec)*100 + (e->tv_usec-b->tv_usec)/10000;

# if defined(sequent)
	cp = "%Uu %Ss %E %P %I+%Oio %Fpf+%Ww";
# else
	cp = "%Uu %Ss %E %P %X+%Dk %I+%Oio %Fpf+%Ww";
# endif
	if (vp && vp->vec[0] && vp->vec[1])
		cp = vp->vec[1];
	for (; *cp; cp++)
	if (*cp != '%')
		cshputchar(*cp);
	else if (cp[1]) switch(*++cp) {

	case 'U':		/* user time */
		pdeltat(&r1->ru_utime, &r0->ru_utime);
		break;

	case 'S':		/* system time */
		pdeltat(&r1->ru_stime, &r0->ru_stime);
		break;

	case 'E':		/* elasped time */
		psecs(ms / 100);
		break;

	case 'P':		/* percent of elasped time */
		printf("%d%%", (int) ( ( (double) t )*100. / ((ms ? (double) ms : 1.)) ) );
		break;

	case 'W':		/* swaps */
		i = r1->ru_nswap - r0->ru_nswap;
		printf("%ld", i);
		break;

	case 'X':		/* integral shared memory size */
		printf("%ld", t == 0 ? 0L : (r1->ru_ixrss-r0->ru_ixrss)/t);
		break;

	case 'D':		/* integral data+stack size */
		printf("%ld", t == 0 ? 0L :
		    (r1->ru_idrss+r1->ru_isrss-(r0->ru_idrss+r0->ru_isrss))/t);
		break;

	case 'K':		/* total shared memory size */
		printf("%ld", t == 0 ? 0L :
		    ((r1->ru_ixrss+r1->ru_isrss+r1->ru_idrss) -
		    (r0->ru_ixrss+r0->ru_idrss+r0->ru_isrss))/t);
		break;

	case 'M':		/* maximum resident set size */
		printf("%ld", (getpagesize() / 1024 * r1->ru_maxrss));
		break;

	case 'F':		/* page faults */
		printf("%ld", r1->ru_majflt-r0->ru_majflt);
		break;

	case 'R':		/* page reclaims */
		printf("%ld", r1->ru_minflt-r0->ru_minflt);
		break;

	case 'I':		/* block input operations */
		printf("%ld", r1->ru_inblock-r0->ru_inblock);
		break;

	case 'O':		/* block output operations */
		printf("%ld", r1->ru_oublock-r0->ru_oublock);
		break;
# if defined(sequent)
	case 'A':		/* messages sent */
		printf("%ld", r1->ru_msgsnd - r0->ru_msgsnd);
		break;

	case 'B':		/* messages received */
		printf("%ld", r1->ru_msgrcv - r0->ru_msgrcv);
		break;

	case 'G':		/* signals received */
		printf("%ld", r1->ru_nsignals - r0->ru_nsignals);
		break;

	case 'C':		/* voluntary context switches */
		printf("%ld", r1->ru_nvcsw - r0->ru_nvcsw);
		break;

	case 'N':		/* involuntary context switches */
		printf("%ld", r1->ru_nivcsw - r0->ru_nivcsw);
		break;
# endif
	}
	cshputchar('\n');
}

pdeltat(t1, t0)
	struct timeval *t1, *t0;
{
	struct timeval td;

	tvsub(&td, t1, t0);
	printf("%d.%01d", td.tv_sec, td.tv_usec/100000);
}

tvadd(tsum, t0)
	struct timeval *tsum, *t0;
{

	tsum->tv_sec += t0->tv_sec;
	tsum->tv_usec += t0->tv_usec;
	if (tsum->tv_usec > 1000000)
		tsum->tv_sec++, tsum->tv_usec -= 1000000;
}

tvsub(tdiff, t1, t0)
	struct timeval *tdiff, *t1, *t0;
{

	tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
	tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
	if (tdiff->tv_usec < 0)
		tdiff->tv_sec--, tdiff->tv_usec += 1000000;
}
