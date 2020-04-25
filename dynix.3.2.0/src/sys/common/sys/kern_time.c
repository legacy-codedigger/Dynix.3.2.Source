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

#ifndef	lint
static	char	rcsid[] = "$Header: kern_time.c 2.8 1991/08/07 20:43:06 $";
#endif

/*
 * kern_time.c
 *	Time of day and interval timer support.
 *
 * These routines provide the kernel entry points to get and set
 * the time-of-day and per-process interval timers.  Subroutines
 * here provide support for adding and subtracting timeval structures
 * and decrementing interval timers, optionally reloading the interval
 * timers when they expire.
 */

/* $Log: kern_time.c,v $
 *
 */

#include "../machine/reg.h"

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/cmn_err.h"

#include "../machine/intctl.h"
#include "../machine/gate.h"

gettimeofday()
{
	register struct a {
		struct	timeval *tp;
		struct	timezone *tzp;
	} *uap = (struct a *)u.u_ap;
	struct timeval atv;
	extern unsigned sec0eaddr;
	GATESPL(s_ipl);

	/*
	 * Return number of users to programs that limit # of users.
	 * (value in R1 on return from gettimeofday()).
	 */
	u.u_r.r_val2 = sec0eaddr;

	/*
	 * Get time vector. Gate used to avoid possible 1 second error.
	 * Is this really necessary? User may be context switched and
	 * thus may be seconds later before user code sees the time.
	 * For consistency and to eliminate the slight probability that
	 * a process calls gettimeofday twice and the second call returning
	 * what appears to be an earlier time, the gate will remain.
	 */
	P_GATE(G_TIME, s_ipl); atv = time; V_GATE(G_TIME, s_ipl);

	u.u_error = copyout((caddr_t)&atv, (caddr_t)uap->tp, sizeof (atv));
	if (u.u_error)
		return;
	if (uap->tzp == 0)
		return;
	/* SHOULD HAVE PER-PROCESS TIMEZONE */
	u.u_error = copyout((caddr_t)&tz, (caddr_t)uap->tzp, sizeof (tz));
}

settimeofday()
{
	register struct a {
		struct	timeval *tv;
		struct	timezone *tzp;
	} *uap = (struct a *)u.u_ap;
	struct timeval atv;
	struct timezone atz;
	GATESPL(s);

	u.u_error = copyin((caddr_t)uap->tv, (caddr_t)&atv,
		sizeof (struct timeval));
	if (u.u_error)
		return;
	setthetime(&atv);
	if (uap->tzp && suser()) {
		u.u_error = copyin((caddr_t)uap->tzp, (caddr_t)&atz,
			sizeof (atz));
		if (u.u_error == 0) {
			P_GATE(G_TIME, s); tz = atz; V_GATE(G_TIME, s);
		}
	}
}

static
setthetime(tv)
	struct timeval *tv;
{
	GATESPL(s);

	if (!suser())
		return;
/* WHAT DO WE DO ABOUT PENDING REAL-TIME TIMEOUTS??? */
	boottime.tv_sec += tv->tv_sec - time.tv_sec;
	P_GATE(G_TIME, s); time = *tv; V_GATE(G_TIME, s);
	resettodr();
}

/*
 * Get value of an interval timer.  The process virtual and
 * profiling virtual time timers are kept in the u. area, since
 * they can be swapped out.  These are kept internally in the
 * way they are specified externally: in time until they expire.
 *
 * The real time interval timer is kept in the process table slot
 * for the process, and its value (it_value) is kept as an
 * absolute time rather than as a delta, so that it is easy to keep
 * periodic real-time signals from drifting.
 *
 * Virtual time timers are processed in the hardclock() routine of
 * kern_clock.c.  The real time timer is processed by a timeout
 * routine, called from the softclock() routine.  Since a callout
 * may be delayed in real time due to interrupt processing in the system,
 * it is possible for the real time timeout routine (realitexpire, given below),
 * to be delayed in real time past when it is supposed to occur.  It
 * does not suffice, therefore, to reload the real timer .it_value from the
 * real time timers .it_interval.  Rather, we compute the next time in
 * absolute time the timer should go off.
 *
 * The gate G_TIME synchronizes with the todclock.
 * The splhi synchronizes with the local hardclock in the 
 * virtual timer and profiling timer.
 */
getitimer()
{
	register struct a {
		u_int	which;
		struct	itimerval *itv;
	} *uap = (struct a *)u.u_ap;
	struct itimerval aitv;
	spl_t s;

	if (uap->which > 2) {
		u.u_error = EINVAL;
		return;
	}
	if (uap->which == ITIMER_REAL) {
		/*
		 * Convert from absolute to relative time in .it_value
		 * part of real time timer.  If time for real time timer
		 * has passed return 0, else return difference between
		 * current time and time for the timer to go off.
		 */
		P_GATE(G_TIME, s);
		aitv = u.u_procp->p_realtimer;
		if (timerisset(&aitv.it_value)) {
			if (timercmp(&aitv.it_value, &time, <))
				timerclear(&aitv.it_value);
			else
				timevalsub(&aitv.it_value, &time);
		}
		V_GATE(G_TIME, s);
	} else {
		s = splhi();
		aitv = u.u_timer[uap->which];
		splx(s);
	}
	u.u_error = copyout((caddr_t)&aitv, (caddr_t)uap->itv,
	    sizeof (struct itimerval));
}

setitimer()
{
	register struct a {
		u_int	which;
		struct	itimerval *itv, *oitv;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p = u.u_procp;
	struct	itimerval aitv, aotv;
	bool_t	sigalrm = 0;			/* should post a SIGALRM? */
	int	s;
	extern	lock_t	time_lck;		/* see untimeout */

	if (uap->which > 2) {
		u.u_error = EINVAL;
		return;
	}
	u.u_error = copyin((caddr_t)uap->itv, (caddr_t)&aitv,
						sizeof (struct itimerval));
	if (u.u_error)
		return;

	if (itimerfix(&aitv.it_value) || itimerfix(&aitv.it_interval)) {
		u.u_error = EINVAL;
		return;
	}

	if (uap->which == ITIMER_REAL) {
#ifdef	ns32000
		s = drp_lock(&time_lck);
#endif	ns32000
#ifdef	i386
		s = p_lock(&time_lck, SPLHI);
		VOID_P_GATE(G_TIME);
#endif	i386
		/*
		 * Get current value and save away.
		 */
		aotv = p->p_realtimer;
		if (timerisset(&aotv.it_value)) {
			if (timercmp(&aotv.it_value, &time, <))
				timerclear(&aotv.it_value);
			else
				timevalsub(&aotv.it_value, &time);
		}

		/*
		 * Handle race where setitimer occurs between todclock()
		 * and softclock(). That is, if the timer expires and
		 * the untimeout occurs before the timeout event sends
		 * the SIGALRM.
		 */
		if (luntimeout(realitexpire, (caddr_t)p)) {
			if (aotv.it_value.tv_sec==0 && aotv.it_value.tv_usec==0)
				++sigalrm;
		}
		if (timerisset(&aitv.it_value)) {
			timevaladd(&aitv.it_value, &time);
			ltimeout(realitexpire,(caddr_t)p, hzto(&aitv.it_value));
		}
		p->p_realtimer = aitv;
		V_GATE(G_TIME, SPLHI);
		v_lock(&time_lck, s);
	} else {
		s = splhi();
		if (uap->oitv) {
			aotv = u.u_timer[uap->which];
		}
		u.u_timer[uap->which] = aitv;
		splx(s);
	}

	/*
	 * Now copy out previous value.
	 */
	if (uap->oitv) {
		u.u_error = copyout((caddr_t)&aotv, (caddr_t)uap->oitv,
						sizeof (struct itimerval));
	}

	/*
	 * If needed, post a SIGALRM.  Done here to avoid nesting gates/locks.
	 */
	if (sigalrm)
		psignal(p, SIGALRM);
}

/*
 * Real interval timer expired:
 * send process whose timer expired an alarm signal.
 * If time is not set up to reload, then just return.
 * Else compute next time timer should go off which is > current time.
 * This is where delay in processing this timeout causes multiple
 * SIGALRM calls to be compressed into one.
 */
realitexpire(p)
	register struct proc *p;
{
	int i;
	GATESPL(s);

	psignal(p, SIGALRM);
	if (!timerisset(&p->p_realtimer.it_interval)) {
		timerclear(&p->p_realtimer.it_value);
		return;
	}

	/*
	 * check if date has gone backwards.
	 */
	if (p->p_realtimer.it_value.tv_sec > time.tv_sec) {
		P_GATE(G_TIME, s);
		/*
		 * check for real. (above assumed atomic increasing time).
		 */
		if (timercmp(&p->p_realtimer.it_value, &time, >)) {
			/*
			 * maintain interval - assumes we got here
			 * promptly.
			 */
#ifdef XRT_DEBUG
			printf("realitexpire:time went backwards "); 
			print_tval(&p->p_realtimer.it_value, "it_value", " ");
			print_tval(&time, "time now", "\n");
#endif
			p->p_realtimer.it_value = time;
			timevaladd(&p->p_realtimer.it_value,
			    &p->p_realtimer.it_interval);
			ltimeout(realitexpire, (caddr_t)p,
			    (i=hzto(&p->p_realtimer.it_value)));
			V_GATE(G_TIME, s);
			cmn_err(CE_WARN, "realitexpire:time went backwards. Timeout in %d seconds", i/hz);
			/*
			 *+ A realtime interval timer expired and has detected
			 *+ that the time of day clock has been set backwards.
			 *+ An interval may have been lost because of this
			 *+ or the inerval may not be precise.
			 */
			return;
		}
		V_GATE(G_TIME, s);
	}
	for (i=0; i<100; i++) {
		P_GATE(G_TIME, s);
		timevaladd(&p->p_realtimer.it_value,
		    &p->p_realtimer.it_interval);
		if (timercmp(&p->p_realtimer.it_value, &time, >)) {
			ltimeout(realitexpire, (caddr_t)p,
			    hzto(&p->p_realtimer.it_value));
			V_GATE(G_TIME, s);
			return;
		}
		V_GATE(G_TIME, s);

/*
 * this code is too costly for the frequency of usefulness.
 */
#ifdef REALIT_FIX
		/*
		 * use modulus arithmatic instead or we might be here all day.
		 * must hold G_TIME to prevent the *date* from changing
		 * (clock ticks would be ok).
		 */
		if (i>3) {
			P_GATE(G_TIME, s);
			timevalguess(&p->p_realtimer.it_value, 
					&p->p_realtimer.it_interval, &time); 
			timevaladd(&p->p_realtimer.it_value,
			    &p->p_realtimer.it_interval);
			if (timercmp(&p->p_realtimer.it_value, &time, >)) {
				ltimeout(realitexpire, (caddr_t)p,
				    (i=hzto(&p->p_realtimer.it_value)));
					V_GATE(G_TIME, s);
					return;
			}
			V_GATE(G_TIME, s);
		}
#endif
	}
	/*
	 * The above loop has failed to calculate to correct interval so instead
	 * assume it from now. (this would be the case if REALIT_FIX were not 
	 * defined and the date had been moved forward by relativly large 
	 * ammount.
	 */

	P_GATE(G_TIME, s);
	p->p_realtimer.it_value = time;
	timevaladd(&p->p_realtimer.it_value, &p->p_realtimer.it_interval);
	ltimeout(realitexpire, (caddr_t)p, (i=hzto(&p->p_realtimer.it_value)));
	V_GATE(G_TIME, s);
	cmn_err(CE_WARN, "realtime expire loop taking far too long giving up. Timeout in %d seconds\n", i/hz);
	/*
	 *+ A realtime timer expired and detected that the 
	 *+ time of day clock has been set forwards.
	 *+ An interval may have been lost because of this or may not be
	 *+ precise.
	 */
}

/*
 * Check that a proposed value to load into the .it_value or
 * .it_interval part of an interval timer is acceptable, and
 * fix it to have at least minimal value (i.e. if it is less
 * than the resolution of the clock, round it up.)
 */
itimerfix(tv)
	struct timeval *tv;
{

	if (tv->tv_sec < 0 || tv->tv_sec > 100000000 ||
	    tv->tv_usec < 0 || tv->tv_usec >= 1000000)
		return (EINVAL);
	if (tv->tv_sec == 0 && tv->tv_usec != 0 && tv->tv_usec < tick)
		tv->tv_usec = tick;
	return (0);
}

/*
 * Decrement an interval timer by a specified number
 * of microseconds, which must be less than a second,
 * i.e. < 1000000.  If the timer expires, then reload
 * it.  In this case, carry over (usec - old value) to
 * reducint the value reloaded into the timer so that
 * the timer does not drift.  This routine assumes
 * that it is called in a context where the timers
 * on which it is operating cannot change in value.
 */
itimerdecr(itp, usec)
	register struct itimerval *itp;
	int usec;
{

	if (itp->it_value.tv_usec < usec) {
		if (itp->it_value.tv_sec == 0) {
			/* expired, and already in next interval */
			usec -= itp->it_value.tv_usec;
			goto expire;
		}
		itp->it_value.tv_usec += 1000000;
		itp->it_value.tv_sec--;
	}
	itp->it_value.tv_usec -= usec;
	usec = 0;
	if (timerisset(&itp->it_value))
		return (1);
	/* expired, exactly at end of interval */
expire:
	if (timerisset(&itp->it_interval)) {
		itp->it_value = itp->it_interval;
		itp->it_value.tv_usec -= usec;
		if (itp->it_value.tv_usec < 0) {
			itp->it_value.tv_usec += 1000000;
			itp->it_value.tv_sec--;
		}
	} else
		itp->it_value.tv_usec = 0;		/* sec is already 0 */
	return (0);
}

/*
 * Add and subtract routines for timevals.
 * N.B.: subtract routine doesn't deal with
 * results which are before the beginning,
 * it just gets very confused in this case.
 * Caveat emptor.
 */
timevaladd(t1, t2)
	struct timeval *t1, *t2;
{

	t1->tv_sec += t2->tv_sec;
	t1->tv_usec += t2->tv_usec;
	timevalfix(t1);
}

timevalsub(t1, t2)
	struct timeval *t1, *t2;
{

	t1->tv_sec -= t2->tv_sec;
	t1->tv_usec -= t2->tv_usec;
	timevalfix(t1);
}

timevalfix(t1)
	struct timeval *t1;
{

	if (t1->tv_usec < 0) {
		t1->tv_sec--;
		t1->tv_usec += 1000000;
	}
	if (t1->tv_usec >= 1000000) {
		t1->tv_sec++;
		t1->tv_usec -= 1000000;
	}
}

/*
 * adjtime(delta, olddelta)
 * struct timeval *delta, *olddelta;
 *
 * adjtime system call used to slow down or speed up the time
 * of day clock.  The time of day clock is slowly adjusted
 * to the new timedelta by tickdelta at each todclock() call.
 *
 * tickdelta is set to tickadj if the amount of change is
 * < |1 sec.| or to 10 X tickadj if the change is > |1 sec.|
 *
 * timedelta is set to be the closest multiple of tickdelta
 * for the time the user asked for.
 *
 */

int tickdelta;			/* current clock skew, us. per tick */
long timedelta;			/* unapplied time correction, us. */
static long bigadj = 1000000;	/* use 10x skew above bigadj us. */

adjtime()
{
	register struct a {
		struct timeval *delta;
		struct timeval *olddelta;
	} *uap = (struct a *)u.u_ap;
	register long new_timed;
	register int new_tickd;
	struct timeval atv, oatv;
	extern int tickadj;		/* "standard" clock skew, us./tick */
	GATESPL(s);

	if (!suser()) {
		return;
	}
	u.u_error = copyin((caddr_t)uap->delta, (caddr_t)&atv,
			sizeof (struct timeval));
	if (u.u_error) {
		return;
	}
	new_timed = atv.tv_sec * 1000000 + atv.tv_usec;
	if ((new_timed > bigadj) || (new_timed < -bigadj)) {
		new_tickd = 10 * tickadj;
	} else {
		new_tickd = tickadj;
	}
	if (new_timed % new_tickd) {
		new_timed = new_timed / new_tickd * new_tickd;
	}

	P_GATE(G_TIME, s);
	if (uap->olddelta) {
		oatv.tv_sec = timedelta / 1000000;
		oatv.tv_usec = timedelta % 1000000;
	}
	timedelta = new_timed;
	tickdelta = new_tickd;
	V_GATE(G_TIME, s);

	if (uap->olddelta) {
		u.u_error = copyout((caddr_t)&oatv, (caddr_t)uap->olddelta,
			sizeof (struct timeval));
	}
}

/*
 * this code is too costly for the frequency of usefulnes.
 */
#ifdef REALIT_FIX

/*
 * timevalguess.
 * return value = value + guess*interval is just less than now or
 * equal to now interval.
 * If the difference between value and time is great enough and
 * and the time interval small enough we will overflow.
 * In that case return value=now.
 */
int
timevalguess(value, interval, now)
	struct timeval *value;
	struct timeval *interval;
	struct timeval *now;
{
	unsigned long	guess;
	struct	timeval p1;
	unsigned long	i_secs;
	unsigned long	i1,i2,i3;
	int 	i;
	int	loop;

#ifdef  RT_DEBUG
	printf("timevalguess ");
	print_tval(value, "value", " ");
	print_tval(interval, "interval", " ");
	print_tval(now, "now", "\n");
#endif

	if (timercmp(value, now, > )) {
		*value = *now;
		timevaladd(value, interval);
		return;
	}
	for(loop=0; loop<MAXBCONV; loop++) { /* binary converg is worst case */
		p1 = *now;
		timevalsub(&p1, value);
#ifdef RT_DEBUG
		print_tval(&p1, "(now - value)", " ");
		printf("-loop %d\n",loop);
#endif
		if (timercmp(interval, &p1, > )) {
			/*
			 * got there.
			 */
			return;
		}
		i_secs = p1.tv_sec;

		/*
		 * now find the next higher interval that execeds "now".
		 * guess.
		 */

		/*
		 * check if we can do in 32 bit arithmatic.
		 * printf("32 at %d seconds\n", MAX32PROD);
		 */
		if (((ulong)p1.tv_sec < MAX32PROD) && ((ulong)interval->tv_sec < MAX32PROD)) {
#ifdef RT_DEBUG
			printf("using 32 bit arithmatic\n");
#endif
			/*
			 * convert to 32 bit.
			 */
			i1 = p1.tv_sec*USEC + p1.tv_usec;
			i2 = interval->tv_sec*USEC + interval->tv_usec;
			if (i2 == 0) {
#ifdef RT_DEBUG
				printf("/0\n");
#endif
				return;
			}
			i3 = i1/i2;
			i3 = i3 * i2;
			/*
			 * convert back
			 */
			p1.tv_sec =  i3 / USEC;
			p1.tv_usec = i3 % USEC;
#ifdef RT_DEBUG
			print_tval(&p1, "nearest interval multiple", "\n");
#endif
			timevaladd(value, &p1);
#ifdef RT_DEBUG
			print_tval(value, "starting value (exact)", "\n");
#endif
			return;
		} 
		/*
		 * A small divisor can produce large errors so use 
		 * better guesses since all we need is 32/16 arithmatic.
		 */
		if (interval->tv_sec == 0) {
			if ((ulong)interval->tv_usec < 3) {
				/*
				 * If the interval is extremely small 
				 * then special case it.
				 */
				i1 = interval->tv_usec + p1.tv_usec;
				i1 = i1 & 0x3;
#ifdef RT_DEBUG
				printf("tiny p1.tv_usec 0x%x interval 0x%x i1=%d\n", 
						p1.tv_usec, interval->tv_usec, i1);
#endif
				value->tv_sec = now->tv_sec;
				value->tv_usec = now->tv_usec - i1;
				return;
			}
			if ((ulong)p1.tv_sec > (ulong)interval->tv_usec) {
				/*
				 * take advantage of a quick multiply by USEC
				 * so use the usecs as USEC*usces which is secs.
				 */
				guess = (ulong)p1.tv_sec/(ulong)interval->tv_usec;
				guess *= interval->tv_usec;
#ifdef RT_DEBUG
				printf("quick %u/%u*%u guess %u\n", p1.tv_sec, 
					interval->tv_usec, interval->tv_usec, guess);
#endif
				value->tv_sec = value->tv_sec + guess;
#ifdef RT_DEBUG
				print_tval(value, "quick guess(now - value)", "\n");
#endif
				continue;
			}
			/*
			 * use (ai+b)/(0i+d) is about ai/d.
			 * a/d is therefore i too big
			 */

			guess = (ulong)(p1.tv_sec*USECSQRT)/(ulong)(interval->tv_usec+1);
			guess = guess * USECSQRT;
#ifdef RT_DEBUG
			printf("small guess %u <%u\n", guess, MAXFPROD);
#endif
		} else {
			/*
			 * interval is large so guess using seconds.
			 * then iterate on binary divisors of micro seconds.
			 */
			guess = (ulong)p1.tv_sec/(ulong)(interval->tv_sec+1); /* guess small */
#ifdef RT_DEBUG
			printf("large guess %u\n", guess);
#endif
		}
		/*
		 * multiply the interval by guess
		 */
		if ((guess > 1) && timevalimul(interval, guess, &p1)) {
			*value = *now;
			return;
		}
		/*
		 * now add the starting value
		 */
#ifdef RT_DEBUG
		print_tval(&p1, "nearest interval multiple", "\n");
#endif
		timevaladd(value, &p1);
#ifdef RT_DEBUG
		print_tval(value, "new starting value", "\n");
#endif
		/*
		 * binary converge if we were too far off.
		 */
		if (p1.tv_sec)
			i1 = i_secs/(ulong)p1.tv_sec;
		else
			i1 = 1;

#ifdef RT_DEBUG
		printf("i_secs %u p1.tv_sec %u off by %d\n",i_secs,p1.tv_sec, i1);
#endif
		/*
		 * limit loop to 9 times round.
		 */
		i1 = i1/2;
		if (i1>512)
			i1 = 512;
		for (i=1; i<=i1; i = i*2){
			timevaladd(value, &p1);
#ifdef RT_DEBUG
			print_tval(value, "starting value - interval doubled", "\n");
#endif
			timevaladd(&p1, &p1);
		}
	}
}


/*
 *  scalar multiply.
 *  t1 = sf+u     
 *  t2 <-- t1*'m = m*sf + u*d
 *               = f(m*s + m*u/f) + (m*u%f)
 *  return 1 = failed
 */

int
timevalimul(t1,mult,t2)
	struct timeval *t1;
	unsigned long	mult;
	struct timeval *t2;
{
	unsigned long	p1,p2,p3,p4,p5,p6,p7;
	extern	unsigned long timevalmuldiv();

#ifdef RT_DEBUG
	printf("timevalimul (m)%u * %u,%d\n", mult, t1->tv_sec, t1->tv_usec);
#endif
	p1 = t1->tv_sec * mult;		
#ifdef RT_DEBUG
	printf("timevalimul p1:%u (m*sec)\n",p1);
#endif
	/*
	 * now calucalate usec*mult/USEC and usec*mult%USEC
	 * we cannot let the partial product get too big so
	 * pre-divide by the sqrt of the divisor to avoid loss of precision.
	 * (assume USECSQRT*USEC < MAXUINT)
	 * s*s=f
	 * (us+v)m  =  (mus + mv)
	 * (mus+mv)/f = (mus/f+mv/f)+d      
	 *             = (mu/s+mv/f)+d    0 < d < ((mu%s)s+(mv%f))/f < 2
	 */
						/* s*s=f            */
						/* u = us+v         */
	p2 = t1->tv_usec % USECSQRT;		/* v  		    */
	p3 = t1->tv_usec / USECSQRT;		/* u 		    */
#ifdef RT_DEBUG
	p4 = mult * p2;				/* m*v 	    	    */
	p5 = timevalmuldiv(mult, p3, USECSQRT);	/* m*u/s 	    */
	p6 = p5 + (p4 / USEC);			/* m*u/s + m*v/f    */
	printf("timevalimul p2:%u v\n",p2);
	printf("timevalimul p3:%u u\n",p3);
	printf("timevalimul p4:%u m*v\n",p4);
	printf("timevalimul p5:%u high m*u/s\n",p5);
	printf("timevalimul p6:\n",p6);
#endif
	p6 = ((mult * p2) / USEC) + timevalmuldiv(mult, p3, (unsigned long)USECSQRT);
	/*
	 * now the error from the integer divides. (this is exact over the
	 * legal range of values of sec).
	 */
	p7 = ((((mult%USECSQRT)*(p3%USECSQRT))%USECSQRT)*USECSQRT +
		((mult%USEC)*(p2))%USEC) > USEC;

	t2->tv_sec = p1 + p6 + p7;

	/*
	 * split into 2 parts for double precision.
	 * (ab)%c = (a%c*b%c)%c
	 * and (a+b)%c = (a%c+b%c)%c
	 */
	p2 = mult % USEC;	
	p3 = p2 / USECSQRT;
	p4 = p2 % USECSQRT;

	p5 = (p3 * t1->tv_usec) % USEC;
	p6 = (p4 * t1->tv_usec) % USEC;

	p7 = (p5*USECSQRT + p6) % USEC;

	t2->tv_usec = p7;
#ifdef RT_DEBUG
	print_tval(t2, "timevalimul", "\n");
#endif
	return (0);
}


/*
 * scale.
 * return a*b/f without loss of precision.
 */
unsigned long
timevalmuldiv(a,b,f)
	unsigned long a;
	unsigned long b;
	unsigned long f;
{
	unsigned long i1,i2,i3,i4;

	/*
	 * use (a+b)c = ac+bc
	 * use (a+b)/f ~= a/f + b/f
	 */
	/*
	 * split a int two parts. i1*f + i2
	 */
	i1 = a / f;
	i2 = a % f;
	/*
	 * now multiply by b so get i1*f*b + i2*b
	 */
	i3 = b * i1;
	i4 = b * i2;
	/*
	 * now divide by f to get i1*f*b/f + i2*b/f
	 *                     =  i1*b + i2*b/f
	 */
	return(i3 + (i4/f));
}

#endif /* REALIT_FIX */

#ifdef XRT_DEBUG
static
print_tval(t, s, s2)
	struct timeval	*t;
	char	*s;
	char	*s2;
{
	printf("time %s is %u.(%u) %s", s, t->tv_sec, t->tv_usec, s2);
}
#endif
